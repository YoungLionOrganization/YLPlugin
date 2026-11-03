#include <ylplugin/ylplugin.h>
#include "internal.hpp"
#include <cstdlib>
#include <cstring>

struct ylp_package { ylp::Package value; explicit ylp_package(ylp::Package p):value(std::move(p)) {} };
struct ylp_session { ylp::Session value; ylp_session(ylp::Package p,ylp::Host h):value(std::move(p),std::move(h),ylp::data_adapter()) {} };
namespace {
template<class Fn> ylp_status guard(ylp_error* error,Fn fn) noexcept {
    if(error) *error={};
    try { fn(); return YLP_OK; }
    catch(const ylp::Error& e) {
        auto status=static_cast<ylp_status>(e.code);
        if(error) { error->code=status; std::snprintf(error->message,sizeof(error->message),"%s",e.what()); } return status;
    } catch(const std::bad_alloc&) {
        if(error) { error->code=YLP_MEMORY; std::snprintf(error->message,sizeof(error->message),"allocation failed"); } return YLP_MEMORY;
    } catch(const std::exception& e) {
        if(error) { error->code=YLP_INVALID_METADATA; std::snprintf(error->message,sizeof(error->message),"%s",e.what()); } return YLP_INVALID_METADATA;
    } catch(...) {
        if(error) { error->code=YLP_RUNTIME; std::snprintf(error->message,sizeof(error->message),"unexpected exception"); } return YLP_RUNTIME;
    }
}
std::filesystem::path utf8_path(const std::string& s) { ylp::require(s.find('\0')==std::string::npos,ylp::Code::invalid_metadata,"NUL in filesystem path"); return std::filesystem::path(reinterpret_cast<const char8_t*>(s.c_str())); }
void check(bool b) { ylp::require(b,ylp::Code::invalid_metadata,"invalid API argument or ABI structure"); }
ylp::Limits limits(const ylp_limits* v) {
    if(!v) return {};
    check(v->struct_size==sizeof(*v)&&v->abi_version==YLP_ABI_VERSION);
    return {v->metadata,v->toc,v->records,v->record,v->total,v->ratio};
}
ylp::KeyProvider keys(ylp_key_fn fn,void* c) {
    if(!fn) return {};
    return [fn,c](const std::string& id,std::array<uint8_t,32>& out){return fn(c,id.c_str(),out.data())==0;};
}
ylp::Cancel cancel(ylp_cancel_fn fn,void* c) { if(!fn) return {}; return [fn,c]{return fn(c)!=0;}; }
void buffer(ylp_buffer* out,std::span<const uint8_t> data) {
    check(out&&data.size()<SIZE_MAX);
    auto ptr=static_cast<unsigned char*>(std::malloc(data.size()+1));
    ylp::require(ptr,ylp::Code::memory,"allocation failed");
    if(!data.empty()) std::memcpy(ptr,data.data(),data.size());
    ptr[data.size()]=0; out->data=ptr; out->size=data.size();
}
void buffer(ylp_buffer* out,const std::string& text) { buffer(out,{reinterpret_cast<const uint8_t*>(text.data()),text.size()}); }
void empty(ylp_buffer* out) { if(out) *out={}; }
const ylp::Permission& permission(const ylp_package* p,size_t i,int optional) {
    check(p); const auto& t=p->value.metadata().targets.at(i); return optional?t.optional_permissions:t.required_permissions;
}
}
extern "C" {
const char* ylp_version(void) { return YLPLUGIN_VERSION; }
ylp_limits ylp_default_limits(void) { ylp::Limits l; return {sizeof(ylp_limits),YLP_ABI_VERSION,l.metadata,l.toc,l.records,l.record,l.total,l.ratio}; }
void ylp_buffer_free(ylp_buffer* out) { if(out) { std::free(out->data); *out={}; } }
ylp_status ylp_open(const char* path,const ylp_limits* l,ylp_package** out,ylp_error* e) {
    if(out) *out=nullptr;
    return guard(e,[&]{check(path&&out); *out=new ylp_package(ylp::Package(std::filesystem::path(reinterpret_cast<const char8_t*>(path)),limits(l)));});
}
ylp_status ylp_open_reader(uint64_t size,ylp_read_at_fn reader,void* ctx,const ylp_limits* l,ylp_package** out,ylp_error* e) {
    if(out) *out=nullptr;
    return guard(e,[&]{check(reader&&out); *out=new ylp_package(ylp::Package(size,[reader,ctx](uint64_t off,std::span<uint8_t> data){
        ylp::require(reader(ctx,off,data.data(),data.size())==0,ylp::Code::io,"read_at callback failed");
    },limits(l)));});
}
void ylp_close(ylp_package* p) { delete p; }
ylp_status ylp_inspect(const ylp_package* p,ylp_buffer* out,ylp_error* e) { empty(out); return guard(e,[&]{check(p&&out);buffer(out,p->value.inspect().dump());}); }
ylp_status ylp_metadata(const ylp_package* p,ylp_buffer* out,ylp_error* e) { empty(out); return guard(e,[&]{check(p&&out);buffer(out,p->value.metadata_text());}); }
ylp_status ylp_validate_profile(const yl_document* doc,ylp_error* e) { return guard(e,[&]{check(doc);ylp::validate_profile(doc);}); }
yl_profile ylp_plugin_profile(void) { return {"xyz.younglion.plugin","1.0",ylp::profile_validator,nullptr}; }
size_t ylp_resource_count(const ylp_package* p) { return p?p->value.resources().size():0; }
ylp_status ylp_resource_at(const ylp_package* p,size_t i,ylp_resource* out,ylp_error* e) {
    return guard(e,[&]{check(p&&out&&out->struct_size==sizeof(*out)&&out->abi_version==YLP_ABI_VERSION);
        const auto& r=p->value.resources().at(i); *out={}; out->struct_size=sizeof(*out); out->abi_version=YLP_ABI_VERSION;
        out->path=r.path.c_str(); out->media=r.media.c_str(); out->variant=r.variant.c_str(); out->key_id=r.key_id.c_str();
        out->offset=r.offset; out->stored_size=r.stored; out->size=r.size; out->alignment=r.alignment;
        out->compression=r.compression; out->encryption=r.encryption; out->extraction=r.extraction; std::copy(r.hash.begin(),r.hash.end(),out->sha256);
    });
}
size_t ylp_target_count(const ylp_package* p) { try{return p?p->value.metadata().targets.size():0;}catch(...){return 0;} }
size_t ylp_entrypoint_count(const ylp_package* p) { try{return p?p->value.metadata().entrypoints.size():0;}catch(...){return 0;} }
ylp_status ylp_target_at(const ylp_package* p,size_t i,ylp_buffer* out,ylp_error* e) {
    empty(out);return guard(e,[&]{check(p&&out);const auto& t=p->value.metadata().targets.at(i);
        buffer(out,ylp::Json({{"id",t.id},{"host",t.host},{"api_min",t.api_min},{"api_max",t.api_max},{"entrypoint",t.entrypoint},
            {"priority",t.priority},{"fallback",t.fallback},{"required_capabilities",t.required_capabilities},{"optional_capabilities",t.optional_capabilities},
            {"required_permissions",ylp::permission_json(t.required_permissions)},{"optional_permissions",ylp::permission_json(t.optional_permissions)},
            {"memory",t.memory},{"storage",t.storage}}).dump());});
}
ylp_status ylp_entrypoint_at(const ylp_package* p,size_t i,ylp_buffer* out,ylp_error* e) {
    empty(out);return guard(e,[&]{check(p&&out);const auto& t=p->value.metadata().entrypoints.at(i);
        buffer(out,ylp::Json({{"id",t.id},{"runtime",t.runtime},{"runtime_min",t.runtime_min},{"runtime_max",t.runtime_max},
            {"resource",t.resource},{"os",t.os},{"arch",t.arch},{"abi",t.abi}}).dump());});
}
ylp_status ylp_permission_at(const ylp_package* p,size_t i,int optional,ylp_permission* out,ylp_error* e) {
    return guard(e,[&]{check(out&&out->struct_size==sizeof(*out)&&out->abi_version==YLP_ABI_VERSION);const auto& v=permission(p,i,optional);
        *out={};out->struct_size=sizeof(*out);out->abi_version=YLP_ABI_VERSION;out->format=static_cast<int>(v.format);
        out->registry=v.registry.c_str();out->registry_version=v.registry_version.c_str();out->mask=v.mask;
        out->bytes=v.bytes.data();out->byte_count=v.bytes.size();});
}
size_t ylp_permission_string_count(const ylp_package* p,size_t i,int optional) {try{return permission(p,i,optional).strings.size();}catch(...){return 0;} }
const char* ylp_permission_string_at(const ylp_package* p,size_t i,int optional,size_t index) {try{return permission(p,i,optional).strings.at(index).c_str();}catch(...){return nullptr;} }
ylp_status ylp_read(const ylp_package* p,const char* path,ylp_key_fn k,void* kc,ylp_cancel_fn c,void* cc,ylp_buffer* out,ylp_error* e) {
    empty(out);return guard(e,[&]{check(p&&path&&out);buffer(out,p->value.read(path,keys(k,kc),cancel(c,cc)));});
}
ylp_status ylp_read_slice(const ylp_package* p,const char* path,uint64_t off,size_t n,ylp_buffer* out,ylp_error* e) {
    empty(out);return guard(e,[&]{check(p&&path&&out);buffer(out,p->value.read_slice(path,off,n));});
}
ylp_status ylp_validate(const ylp_package* p,ylp_key_fn k,void* kc,ylp_cancel_fn c,void* cc,ylp_error* e) {return guard(e,[&]{check(p);p->value.validate(keys(k,kc),cancel(c,cc));});}
ylp_status ylp_verify(const ylp_package* p,ylp_key_fn k,void* kc,ylp_cancel_fn c,void* cc,ylp_error* e) {return guard(e,[&]{check(p);p->value.verify(keys(k,kc),cancel(c,cc));});}
ylp_status ylp_check(const ylp_package* p,const char* host,ylp_key_fn k,void* kc,ylp_key_fn t,void* tc,ylp_buffer* out,ylp_error* e) {
    empty(out);return guard(e,[&]{check(p&&host&&out);auto h=ylp::host_from_json(ylp::Json::parse(host));h.keys=keys(k,kc);h.trust=keys(t,tc);buffer(out,ylp::preflight(p->value,h).dump());});
}
ylp_status ylp_extract(const ylp_package* p,const char* dest,const char* selection,ylp_key_fn k,void* kc,ylp_cancel_fn c,void* cc,ylp_error* e) {
    return guard(e,[&]{check(p&&dest&&selection);auto paths=ylp::Json::parse(selection).get<std::vector<std::string>>();p->value.extract(std::filesystem::path(reinterpret_cast<const char8_t*>(dest)),paths,keys(k,kc),cancel(c,cc));});
}
ylp_status ylp_create(const char* output,const char* manifest,const char* build,ylp_key_fn k,void* kc,ylp_cancel_fn c,void* cc,ylp_error* e) {
    return guard(e,[&]{check(output&&manifest&&build);std::vector<ylp::Input> inputs;
        auto j=ylp::Json::parse(build);for(const auto& r:j.at("resources")) {
            ylp::Input in;in.path=r.at("path").get<std::string>();in.file=utf8_path(r.at("file").get<std::string>());
            in.media=r.value("media",std::string("application/octet-stream"));in.variant=r.value("variant",std::string());
            in.key_id=r.value("key_id",std::string());
            auto compression=r.value("compression",0),extraction=r.value("extraction",0);check(compression>=0&&compression<=1&&extraction>=0&&extraction<=2);
            in.compression=static_cast<uint8_t>(compression);in.extraction=static_cast<uint8_t>(extraction);
            if(r.contains("alignment"))check(r.at("alignment").is_number_integer()&&
                (r.at("alignment").is_number_unsigned()||r.at("alignment").get<int64_t>()>=0)&&r.at("alignment").get<uint64_t>()<=UINT32_MAX);
            in.alignment=r.value("alignment",uint32_t(8));inputs.push_back(std::move(in));
        }
        ylp::BuildOptions options;options.keys=keys(k,kc);options.cancel=cancel(c,cc);ylp::create(std::filesystem::path(reinterpret_cast<const char8_t*>(output)),manifest,std::move(inputs),options);});
}
ylp_status ylp_sign(const char* input,const char* output,const char* id,const uint8_t* pem,size_t n,ylp_error* e) {
    return guard(e,[&]{check(input&&output&&id&&pem);ylp::sign(std::filesystem::path(reinterpret_cast<const char8_t*>(input)),std::filesystem::path(reinterpret_cast<const char8_t*>(output)),id,{pem,n});});
}
ylp_status ylp_data_session(const ylp_package* p,const char* h,ylp_session** out,ylp_error* e) {
    if(out)*out=nullptr;
    return guard(e,[&]{check(p&&h&&out);*out=new ylp_session(p->value,ylp::host_from_json(ylp::Json::parse(h)));});
}
ylp_status ylp_session_prepare(ylp_session* s,ylp_cancel_fn c,void* cc,ylp_error* e) {return guard(e,[&]{check(s);s->value.prepare(cancel(c,cc));});}
ylp_status ylp_session_activate(ylp_session* s,ylp_error* e) {return guard(e,[&]{check(s);s->value.activate();});}
ylp_status ylp_session_start(ylp_session* s,ylp_error* e) {return guard(e,[&]{check(s);s->value.start();});}
void ylp_session_stop(ylp_session* s) {if(s)s->value.stop();}
void ylp_session_free(ylp_session* s) {delete s;}
}
