#include "../internal.hpp"
#include <ylplugin/native.h>
#include <algorithm>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#elif !defined(__ANDROID__)
#include <dlfcn.h>
#endif

namespace ylp {
namespace {
class DataAdapter final:public Adapter {
    Bytes bytes_;
public:
    std::string id() const override { return "data"; }
    std::string version() const override { return "1.0"; }
    void validate(const Package& p,const Entrypoint& e) override {
        require(e.runtime==id(),Code::runtime,"data adapter runtime mismatch");
        require(p.resource(e.resource).extraction!=2,Code::unsupported,"memory data adapter cannot honor extract-required; host must provide a file adapter");
    }
    void prepare(const Package& p,const Entrypoint& e,const Host& h,const Cancel& c) override { bytes_=p.read(e.resource,h.keys,c); }
    void activate() override {}
    void start() override {}
    void stop() noexcept override {}
    void cleanup() noexcept override { bytes_.clear(); }
};
class NativeAdapter final:public Adapter {
    std::filesystem::path directory_,library_;
    NativeApproval approval_; bool owns_directory_=false,started_=false,queried_=false;
    void* handle_=nullptr; ylp_native_v1 api_{};
public:
    NativeAdapter(std::filesystem::path directory,NativeApproval approval):directory_(std::move(directory)),approval_(std::move(approval)) {}
    ~NativeAdapter() override { cleanup(); }
    std::string id() const override { return "native"; }
    std::string version() const override { return "1.0"; }
    void validate(const Package& p,const Entrypoint& e) override {
        require(e.runtime==id(),Code::runtime,"native adapter runtime mismatch");
        require(p.resource(e.resource).extraction!=1,Code::permission,"native code requires extraction");
    }
    void prepare(const Package& p,const Entrypoint& e,const Host& h,const Cancel& c) override {
#if defined(__ANDROID__) || (defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
        (void)p;(void)e;(void)h;(void)c; fail(Code::unsupported,"external native code disabled on mobile");
#else
        require(h.allow_native&&approval_,Code::permission,"native execution needs host approval callback");
        require(approval_(p.metadata().id,e.resource,p.resource(e.resource).hash),Code::permission,"host denied digest-bound native approval");
        p.extract(directory_,{e.resource},h.keys,c); owns_directory_=true;
        library_=std::filesystem::absolute(directory_/e.resource);
#endif
    }
    void activate() override {
        require(owns_directory_&&!handle_,Code::runtime,"native adapter not prepared/already activated");
#if defined(_WIN32)
        handle_=reinterpret_cast<void*>(LoadLibraryExW(library_.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));
        require(handle_,Code::runtime,"LoadLibraryEx failed");
        auto query=reinterpret_cast<ylp_native_query_fn>(GetProcAddress(static_cast<HMODULE>(handle_),"ylp_native_query"));
#elif !defined(__ANDROID__)
        handle_=dlopen(library_.c_str(),RTLD_NOW|RTLD_LOCAL);
        require(handle_,Code::runtime,"dlopen failed");
        auto query=reinterpret_cast<ylp_native_query_fn>(dlsym(handle_,"ylp_native_query"));
#else
        ylp_native_query_fn query=nullptr;
#endif
        require(query,Code::runtime,"native ylp_native_query symbol missing");
        api_={}; api_.struct_size=sizeof(api_); api_.abi_version=1;
        require(query(1,&api_)==0&&api_.struct_size==sizeof(api_)&&api_.abi_version==1&&api_.start&&api_.stop&&api_.destroy,
                Code::runtime,"native ABI negotiation failed"); queried_=true;
    }
    void start() override { require(queried_,Code::runtime,"native adapter inactive"); started_=true; require(api_.start(api_.context)==0,Code::runtime,"native start failed"); }
    void stop() noexcept override { if(started_&&queried_) { api_.stop(api_.context); started_=false; } }
    void cleanup() noexcept override {
        stop(); if(queried_) { api_.destroy(api_.context); queried_=false; }
        if(handle_) {
#if defined(_WIN32)
            FreeLibrary(static_cast<HMODULE>(handle_));
#elif !defined(__ANDROID__)
            dlclose(handle_);
#endif
            handle_=nullptr;
        }
        if(owns_directory_) { std::error_code ec; std::filesystem::remove_all(directory_,ec); owns_directory_=false; }
    }
};
}
Session::Session(Package p,Host h,std::unique_ptr<Adapter> a):package_(std::move(p)),host_(std::move(h)),adapter_(std::move(a)) {
    require(bool(adapter_),Code::runtime,"runtime adapter required");
}
Session::~Session() { cleanup(); }
void Session::prepare(const Cancel& cancel) {
    require(stage_==Stage::discovered,Code::runtime,"session prepare is only valid after discovery");
    try {
        cancelled(cancel);
        host_.cancel=cancel;
        // The adapter's actual version overrides an unverified host declaration.
        host_.runtimes[adapter_->id()]=adapter_->version(); result_=preflight(package_,host_);
        require(result_.at("status").get<std::string>()=="SUPPORTED",Code::runtime,"preflight blocked runtime: "+result_.dump()); stage_=Stage::compatible;
        require(result_.at("runtime").get<std::string>()==adapter_->id(),Code::runtime,"selected runtime differs from adapter");
        auto entry=result_.at("entrypoint").get<std::string>(); const auto& m=package_.metadata();
        const auto& e=*std::find_if(m.entrypoints.begin(),m.entrypoints.end(),[&](const Entrypoint& x){return x.id==entry;});
        adapter_->validate(package_,e); stage_=Stage::validated;
        adapter_->prepare(package_,e,host_,cancel); cancelled(cancel); stage_=Stage::prepared;
    } catch(...) { adapter_->cleanup(); stage_=Stage::failed; throw; }
}
void Session::activate() {
    require(stage_==Stage::prepared,Code::runtime,"activation requires prepared session");
    try { adapter_->activate(); stage_=Stage::active; }
    catch(...) { adapter_->cleanup(); stage_=Stage::failed; throw; }
}
void Session::start() {
    require(stage_==Stage::active,Code::runtime,"start requires active session");
    try { adapter_->start(); stage_=Stage::started; }
    catch(...) { adapter_->cleanup(); stage_=Stage::failed; throw; }
}
void Session::stop() noexcept { if(stage_==Stage::started) { adapter_->stop(); stage_=Stage::stopped; } }
void Session::cleanup() noexcept {
    if(stage_!=Stage::cleaned) { stop(); adapter_->cleanup(); stage_=Stage::cleaned; }
}
std::unique_ptr<Adapter> data_adapter() { return std::make_unique<DataAdapter>(); }
std::unique_ptr<Adapter> native_adapter(std::filesystem::path directory,NativeApproval approval) { return std::make_unique<NativeAdapter>(std::move(directory),std::move(approval)); }
} // namespace ylp
