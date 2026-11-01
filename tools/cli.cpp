#include <ylplugin/ylplugin.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <csignal>
#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

namespace {
using namespace ylp;
volatile std::sig_atomic_t interrupted=0;
void interrupt_handler(int) { interrupted=1; }
Cancel cancellation=[] { return interrupted!=0; };
struct Args {
    std::string command; std::vector<std::string> positional;
    std::map<std::string,std::string> options; std::set<std::string> flags;
    explicit Args(int argc,char** argv) {
        const std::set<std::string> boolean={"--json","--help","--drop-signature"};
        const std::set<std::string> valued={"--manifest","--root","--output","--plan","--compression","--host","--key-file",
            "--key-id","--public-key","--private-key","--resource","--target","--os","--arch","--abi"};
        for(int i=1;i<argc;++i) {
            std::string a=argv[i];
            if(boolean.contains(a)) { flags.insert(a); continue; }
            if(a.starts_with("--")&&a!="--version") {
                if(!valued.contains(a)||i+1>=argc) throw Error(Code::invalid_metadata,"unknown option or missing value: "+a);
                if(!options.emplace(a,argv[++i]).second) throw Error(Code::invalid_metadata,"duplicate option: "+a);
            } else if(command.empty()) command=a; else positional.push_back(a);
        }
    }
    std::string option(const std::string& key,const std::string& fallback={}) const { auto i=options.find(key);return i==options.end()?fallback:i->second; }
    std::string required(const std::string& key) const { auto s=option(key);if(s.empty()) throw Error(Code::invalid_metadata,"required option: "+key);return s; }
    std::string input() const { if(positional.size()!=1) throw Error(Code::invalid_metadata,"exactly one input package/file required");return positional[0]; }
};
Bytes load(const std::filesystem::path& path,uint64_t maximum=16*1024*1024) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)throw Error(Code::io,"cannot open: "+path.string());
    auto n=f.tellg();if(n<0||static_cast<uint64_t>(n)>maximum)throw Error(Code::resource_limit,"input exceeds CLI limit");
    Bytes out(static_cast<size_t>(n)); f.seekg(0); if(!out.empty())f.read(reinterpret_cast<char*>(out.data()),n);
    if(!f)throw Error(Code::io,"short CLI file read");return out;
}
std::string text(const std::filesystem::path& path) { auto bytes=load(path);return {bytes.begin(),bytes.end()}; }
struct Secret {
    Bytes bytes;
    ~Secret() { if(!bytes.empty()) OPENSSL_cleanse(bytes.data(),bytes.size()); }
};
KeyProvider key_provider(const Args& args,std::shared_ptr<Secret>& secret) {
    if(args.option("--key-file").empty())return {};
    auto id=args.required("--key-id");secret=std::make_shared<Secret>();secret->bytes=load(args.option("--key-file"),32);
    if(secret->bytes.size()!=32)throw Error(Code::invalid_metadata,"--key-file must contain exactly 32 raw bytes");
    return [secret,id](const std::string& wanted,std::array<uint8_t,32>& out){if(wanted!=id)return false;std::copy(secret->bytes.begin(),secret->bytes.end(),out.begin());return true;};
}
TrustProvider trust_provider(const Args& args) {
    if(args.option("--public-key").empty())return {};
    auto id=args.required("--key-id"),pem=text(args.option("--public-key"));
    std::unique_ptr<BIO,decltype(&BIO_free)> bio(BIO_new_mem_buf(pem.data(),static_cast<int>(pem.size())),BIO_free);
    std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)> key(PEM_read_bio_PUBKEY(bio.get(),nullptr,nullptr,nullptr),EVP_PKEY_free);
    std::array<uint8_t,32> pub{};size_t n=32;
    if(!key||!EVP_PKEY_is_a(key.get(),"ED25519")||EVP_PKEY_get_raw_public_key(key.get(),pub.data(),&n)!=1||n!=32)throw Error(Code::invalid_signature,"Ed25519 public PEM required");
    return [id,pub](const std::string& wanted,std::array<uint8_t,32>& out){if(wanted!=id)return false;out=pub;return true;};
}
std::vector<Input> plan(const Args& a) {
    std::vector<Input> inputs;
    if(!a.option("--plan").empty()) {
        auto path=std::filesystem::absolute(a.option("--plan"));auto j=Json::parse(text(path));
        for(const auto& r:j.at("resources")) {
            Input in;in.path=r.at("path").get<std::string>();in.file=path.parent_path()/r.at("file").get<std::string>();
            in.media=r.value("media",std::string("application/octet-stream"));in.variant=r.value("variant",std::string());
            in.key_id=r.value("key_id",std::string());in.alignment=r.value("alignment",uint32_t(8));
            auto c=r.value("compression",0),e=r.value("extraction",0);
            if(c<0||c>1||e<0||e>2)throw Error(Code::invalid_metadata,"invalid codec/extraction value");
            in.compression=static_cast<uint8_t>(c);in.extraction=static_cast<uint8_t>(e);inputs.push_back(std::move(in));
        }
    } else {
        auto root=std::filesystem::path(reinterpret_cast<const char8_t*>(a.required("--root").c_str()));
        auto codec=a.option("--compression","store");if(codec!="store"&&codec!="zstd")throw Error(Code::unsupported,"compression must be store or zstd");
        for(const auto& file:std::filesystem::recursive_directory_iterator(root)) {
            if(file.is_symlink())throw Error(Code::permission,"builder does not follow symlinks");
            if(file.is_directory())continue;
            if(!file.is_regular_file())throw Error(Code::permission,"non-regular builder input");
            Input in;in.file=file.path();in.path=std::filesystem::relative(file.path(),root).generic_string();
            in.compression=(codec=="zstd"&&file.path().extension()!=".rcc")?1:0;
            if(file.path().extension()==".rcc")in.media="application/x-qt-resource";
            if(!a.option("--key-file").empty())in.key_id=a.required("--key-id");inputs.push_back(std::move(in));
        }
    }
    return inputs;
}
void help() {
    std::cout<<"YLPlugin 0.1.0 (experimental)\n"
      "ylplugin --version\n"
      "ylplugin create --manifest plugin.ylspec (--root resources | --plan resources.json) --output package.ylplugin\n"
      "ylplugin inspect|list|validate FILE [--json]\n"
      "ylplugin check FILE --host host.json [--target ID] [--os OS --arch ARCH --abi ABI]\n"
      "ylplugin extract FILE --output NEW_DIRECTORY [--resource PATH]\n"
      "ylplugin convert legacy.rcc --manifest plugin.ylspec --output package.ylplugin [--resource game.rcc]\n"
      "ylplugin sign FILE --output SIGNED --key-id ID --private-key ed25519.pem\n"
      "ylplugin verify FILE --key-id ID --public-key ed25519-public.pem\n"
      "ylplugin encrypt FILE --output ENCRYPTED --key-id ID --key-file raw32bytes [--drop-signature]\n"
      "validate/check/extract accept --key-id ID --key-file raw32bytes. check accepts verify options.\n"
      "All outputs must be new. No command executes plugin code.\n";
}
int run(const Args& a) {
    if(a.command.empty()||a.flags.contains("--help")){help();return 0;}
    if(a.command=="--version") {std::cout<<"0.1.0\n";return 0;}
    bool machine=a.flags.contains("--json");
    auto print=[&](const Json& j){std::cout<<j.dump(machine?-1:2)<<'\n';};
    std::shared_ptr<Secret> secret;auto keys=key_provider(a,secret);
    if(a.command=="create"||a.command=="convert") {
        auto manifest=text(a.required("--manifest"));std::vector<Input> inputs;
        if(a.command=="create")inputs=plan(a);
        else {
            auto file=a.input();auto head=load(file,64*1024*1024);
            if(head.size()<20||std::string(head.begin(),head.begin()+4)!="qres")throw Error(Code::invalid_metadata,"input is not an RCC file");
            std::string resource=a.option("--resource");
            if(resource.empty()) {
                yl_document* raw=nullptr;yl_error err{};
                if(yl_parse(manifest.data(),manifest.size(),nullptr,&raw,&err)!=YL_OK)throw Error(Code::invalid_metadata,err.message);
                std::unique_ptr<yl_document,decltype(&yl_document_free)> doc(raw,yl_document_free);auto m=validate_profile(doc.get());
                std::set<std::string> names;for(const auto& e:m.entrypoints)names.insert(e.resource);
                if(names.size()!=1)throw Error(Code::invalid_metadata,"convert requires one shared resource or explicit --resource");resource=*names.begin();
            }
            Input in;in.file=file;in.path=resource;in.media="application/x-qt-resource";
            if(keys)in.key_id=a.required("--key-id");inputs.push_back(std::move(in));
        }
        BuildOptions options;options.keys=keys;options.cancel=cancellation;create(a.required("--output"),manifest,std::move(inputs),options);
        print({{"status","CREATED"},{"output",a.required("--output")}});return 0;
    }
    Package p(a.input());
    if(a.command=="inspect")print(p.inspect());
    else if(a.command=="list")print(p.inspect().at("resources"));
    else if(a.command=="validate") {p.validate(keys,cancellation);print({{"status","VALID"},{"signature_status",p.signed_package()?"UNVERIFIED":"UNSIGNED"}});}
    else if(a.command=="verify") {p.verify(trust_provider(a),cancellation);print({{"status","VALID"},{"key_id",p.signing_key_id()}});}
    else if(a.command=="check") {
        auto host=Json::parse(text(a.required("--host")));
        for(const auto* key:{"os","arch","abi"})if(!a.option(std::string("--")+key).empty())host[key]=a.option(std::string("--")+key);
        if(!a.option("--target").empty())host["target"]=a.option("--target");
        auto h=host_from_json(host);h.keys=keys;h.trust=trust_provider(a);h.cancel=cancellation;auto result=preflight(p,h);print(result);return result.at("status")=="SUPPORTED"?0:4;
    } else if(a.command=="extract") {
        std::vector<std::string> selected;if(!a.option("--resource").empty())selected.push_back(a.option("--resource"));
        p.extract(a.required("--output"),selected,keys,cancellation);print({{"status","EXTRACTED"},{"output",a.required("--output")}});
    } else if(a.command=="sign") {
        Secret pem;pem.bytes=load(a.required("--private-key"));sign(a.input(),a.required("--output"),a.required("--key-id"),pem.bytes,cancellation);
        print({{"status","SIGNED"},{"output",a.required("--output")}});
    } else if(a.command=="encrypt") {
        if(!keys)throw Error(Code::needs_license,"encrypt requires --key-file and --key-id");
        if(p.signed_package()&&!a.flags.contains("--drop-signature"))throw Error(Code::permission,"encryption changes signed bytes; explicitly pass --drop-signature and sign the result again");
        for(const auto& r:p.resources())if(r.encryption)throw Error(Code::unsupported,"CLI encrypt accepts plaintext input only; use core providers to rekey");
        std::array<uint8_t,16> random{};if(RAND_bytes(random.data(),16)!=1)throw Error(Code::io,"RNG failed");
        auto root=std::filesystem::path(a.required("--output")).parent_path()/(".ylplugin-encrypt-"+hex(random));
        if(!std::filesystem::create_directory(root))throw Error(Code::io,"cannot create encryption staging");
        struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code ec;std::filesystem::remove_all(p,ec);}} cleanup{root};
        std::filesystem::permissions(root,std::filesystem::perms::owner_all,std::filesystem::perm_options::replace);
        std::vector<Input> inputs;size_t i=0;
        for(const auto& r:p.resources()) {
            auto bytes=p.read(r.path,{},cancellation);auto file=root/std::to_string(i++);std::ofstream f(file,std::ios::binary);
            f.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));f.close();if(!f)throw Error(Code::io,"encryption staging write failed");
            Input in;in.path=r.path;in.file=file;in.media=r.media;in.variant=r.variant;in.compression=r.compression;
            in.extraction=r.extraction;in.alignment=r.alignment;in.key_id=a.required("--key-id");inputs.push_back(std::move(in));
        }
        BuildOptions options;options.keys=keys;options.cancel=cancellation;create(a.required("--output"),p.metadata_text(),std::move(inputs),options);
        print({{"status","ENCRYPTED"},{"output",a.required("--output")},{"signature_status","UNSIGNED"}});
    } else throw Error(Code::unsupported,"unknown command: "+a.command);
    return 0;
}
}
int main(int argc,char** argv) {
    std::signal(SIGINT,interrupt_handler);std::signal(SIGTERM,interrupt_handler);
    bool machine=false;for(int i=1;i<argc;++i)if(std::string(argv[i])=="--json")machine=true;
    try {return run(Args(argc,argv));}
    catch(const ylp::Error& e) {
        int exit=e.code==ylp::Code::invalid_signature||e.code==ylp::Code::integrity?5:e.code==ylp::Code::needs_license?6:
            e.code==ylp::Code::io?7:e.code==ylp::Code::cancelled?8:e.code==ylp::Code::unsupported?9:3;
        if(machine)std::cerr<<ylp::Json({{"error",e.what()},{"code",static_cast<int>(e.code)},{"exit_code",exit}}).dump()<<'\n';
        else std::cerr<<"ylplugin: "<<e.what()<<'\n';return exit;
    } catch(const std::exception& e) {
        if(machine)std::cerr<<ylp::Json({{"error",e.what()},{"exit_code",3}}).dump()<<'\n';else std::cerr<<"ylplugin: "<<e.what()<<'\n';return 3;
    }
}
