// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include <ylplugin/ylplugin.hpp>
#include <ylplugin/ylplugin.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <fstream>
#include <iostream>
#include <future>
#include <random>
#include <cstring>

using namespace ylp;
namespace {
int checks=0;
void expect(bool value,const std::string& name) {++checks;if(!value)throw std::runtime_error("FAILED: "+name);}
template<class Fn> void rejected(Fn fn,const std::string& name) {
    bool failed=false;try{fn();}catch(const Error&){failed=true;}expect(failed,name);
}
Bytes load(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return Bytes(std::istreambuf_iterator<char>(f),{});}
void save(const std::filesystem::path& p,std::span<const uint8_t> b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));if(!f)throw std::runtime_error("test write failed");}
void save(const std::filesystem::path& p,const std::string& s){save(p,{reinterpret_cast<const uint8_t*>(s.data()),s.size()});}
uint64_t u64(const Bytes& b,size_t off){uint64_t n=0;for(size_t i=0;i<8;++i)n|=uint64_t(b.at(off+i))<<(i*8);return n;}
void field(Bytes& b,size_t off,uint64_t n,size_t count=8){for(size_t i=0;i<count;++i)b.at(off+i)=static_cast<uint8_t>(n>>(i*8));}
Package memory(const Bytes& b,Limits l={}){return Package(b.size(),[&b](uint64_t off,std::span<uint8_t> out){if(off>b.size()||out.size()>b.size()-off)throw Error(Code::io,"short memory read");std::copy_n(b.begin()+static_cast<ptrdiff_t>(off),out.size(),out.begin());},l);}
std::string manifest(const std::string& permissions="[]",const std::string& runtime="data",const std::string& native="") {
    return "[_ylspec]\nversion=\"0.1\"\nprofile=\"xyz.younglion.plugin\"\nprofile_version=\"1.0\"\n"
        "[data]\nid=\"xyz.test.plugin\"\nversion=\"1.0\"\n"
        "[[data.entrypoints]]\nid=\"main\"\nruntime=\""+runtime+"\"\nruntime_min=\"1.0\"\nruntime_max=\"1.0\"\nresource=\"data.bin\"\n"+native+
        "[[data.targets]]\nid=\"primary\"\nhost=\"xyz.test.host\"\napi_min=\"1.0\"\napi_max=\"2.0\"\nentrypoint=\"main\"\nrequired_permissions="+permissions+"\n";
}
Host host(){return host_from_json({{"id","xyz.test.host"},{"api","1.0"},{"os","linux"},{"arch","x86_64"},{"abi","ylp-c1"},
    {"runtimes",{{"data","1.0"}}},{"permission_formats",{"strings","bytes","bitmask"}},{"memory",1024*1024*128},{"storage",1024*1024*128}});}
struct Temp {
    std::filesystem::path path;
    Temp(){std::array<uint8_t,16> random{};if(RAND_bytes(random.data(),16)!=1)throw std::runtime_error("RNG");path=std::filesystem::canonical(std::filesystem::temp_directory_path())/("ylplugin-test-"+hex(random));std::filesystem::create_directory(path);}
    ~Temp(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
struct Signing {
    Bytes pem;std::array<uint8_t,32> pub{};
    Signing(){
        std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)> key(EVP_PKEY_Q_keygen(nullptr,nullptr,"ED25519"),EVP_PKEY_free);
        expect(bool(key),"generate Ed25519 key");
        std::unique_ptr<BIO,decltype(&BIO_free)> bio(BIO_new(BIO_s_mem()),BIO_free);
        expect(PEM_write_bio_PrivateKey(bio.get(),key.get(),nullptr,nullptr,0,nullptr,nullptr)==1,"serialize private key");
        char* data=nullptr;auto n=BIO_get_mem_data(bio.get(),&data);pem.assign(data,data+n);size_t size=32;
        expect(EVP_PKEY_get_raw_public_key(key.get(),pub.data(),&size)==1&&size==32,"public key export");
    }
    TrustProvider trust(){return [this](const std::string& id,std::array<uint8_t,32>& out){out=pub;return id=="test-key";};}
};
void native_test(const std::filesystem::path& library){
#if defined(__linux__)
#if defined(__aarch64__)
    std::string architecture="arm64";
#else
    std::string architecture="x86_64";
#endif
    Temp tmp;auto manifest_text=manifest("[]","native","os=\"linux\"\narch=\""+architecture+"\"\nabi=\"ylp-c1\"\n");
    Input in;in.path="data.bin";in.file=library;in.extraction=2;create(tmp.path/"native.ylplugin",manifest_text,{in});
    Package p(tmp.path/"native.ylplugin");auto h=host();h.arch=architecture;h.runtimes["native"]="1.0";h.allow_extraction=true;
    expect(preflight(p,h).at("status")=="UNSUPPORTED","native default deny");h.allow_native=true;
    expect(preflight(p,h).at("status")=="SUPPORTED","native opt-in preflight");
    int approvals=0;Session session(p,h,native_adapter(tmp.path/"native-cache",[&](const std::string& id,const std::string& path,const auto& hash){
        ++approvals;return id==p.metadata().id&&path=="data.bin"&&hash==p.resource(path).hash;
    }));
    rejected([&]{session.start();},"runtime start out of order");session.prepare();expect(approvals==1&&session.stage()==Stage::prepared,"native prepare approval");
    session.activate();session.start();session.stop();session.cleanup();expect(!std::filesystem::exists(tmp.path/"native-cache"),"native cleanup cache");
    Session denied(p,h,native_adapter(tmp.path/"denied",[](const auto&,const auto&,const auto&){return false;}));
    rejected([&]{denied.prepare();},"native host approval denied");expect(!std::filesystem::exists(tmp.path/"denied"),"denied native creates no cache");
    h.os="ios";expect(preflight(p,h).at("status")=="UNSUPPORTED","iOS native prohibited");
#else
    (void)library;std::cout<<"Native runtime execution not exercised on this platform\n";
#endif
}
void core(){
    Temp tmp;auto source=tmp.path/"source";Bytes content(8192);for(size_t i=0;i<content.size();++i)content[i]=static_cast<uint8_t>(i%251);save(source,content);
    Input in;in.path="data.bin";in.file=source;
    auto path=tmp.path/"base.ylplugin";create(path,manifest(),{in});Package p(path);p.validate();expect(p.read("data.bin")==content,"store roundtrip");
    expect(p.read_slice("data.bin",250,8)==Bytes(content.begin()+250,content.begin()+258),"verified slice");
    expect(p.read_slice("data.bin",8192,0).empty(),"empty slice");rejected([&]{p.read_slice("data.bin",8193,0);},"slice bounds");
    create(tmp.path/"repeat.ylplugin",manifest(),{in});expect(load(path)==load(tmp.path/"repeat.ylplugin"),"deterministic build");
    rejected([&]{create(path,manifest(),{in});},"no overwrite output");
    auto bytes=load(path);auto toc=u64(bytes,48),payload=u64(bytes,72);expect(bytes[128]=='Y'&&bytes[129]=='L',"embedded genuine YLSpec frame");
    std::vector<size_t> corrupt_fields={0,8,10,12,16,24,32,40,48,56,64,72,80,88,96,104};
    for(auto off:corrupt_fields){auto bad=bytes;bad[off]^=0x80;rejected([&]{memory(bad);},"corrupt header "+std::to_string(off));}
    for(size_t n:{size_t(0),size_t(1),size_t(127),size_t(128),bytes.size()-1}){auto bad=bytes;bad.resize(n);rejected([&]{memory(bad);},"truncated file "+std::to_string(n));}
    {auto bad=bytes;field(bad,48,UINT64_MAX-7);field(bad,56,100);rejected([&]{memory(bad);},"offset overflow");}
    {auto bad=bytes;field(bad,128+16,UINT64_MAX);rejected([&]{memory(bad);},"truncated/overflow metadata frame");}
    for(size_t off:{size_t(0),size_t(4),size_t(5),size_t(6),size_t(7),size_t(8),size_t(16),size_t(24),size_t(32),size_t(36),size_t(38),size_t(44),size_t(108)}) {
        auto bad=bytes;bad.at(static_cast<size_t>(toc)+off)^=0x80;rejected([&]{memory(bad);},"TOC corruption "+std::to_string(off));
    }
    {auto bad=bytes;bad[payload]^=1;auto q=memory(bad);rejected([&]{q.read("data.bin");},"payload tampering");}
    {auto bad=bytes;field(bad,toc+24,UINT64_MAX);rejected([&]{memory(bad);},"oversized declaration");}
    {Limits l;l.record=1024;rejected([&]{Package q(path,l);},"host record limit");}
    {auto bad=bytes;std::string needle="xyz.younglion.plugin";auto pos=std::search(bad.begin(),bad.end(),needle.begin(),needle.end());expect(pos!=bad.end(),"find profile vector");*(pos+static_cast<ptrdiff_t>(needle.size()-1))='x';auto q=memory(bad);expect(q.inspect().at("profile_status")=="UNSUPPORTED_PROFILE","inspect unknown profile");expect(preflight(q,host()).at("status")=="UNSUPPORTED_PROFILE","preflight unsupported profile");rejected([&]{q.validate();},"reject unsupported profile validation");}
    for(const auto& name:{"../x","/x","a/../b","a\\b","a//b","CON","nul.txt","LPT1.qml","trailing.","a:b","a/./b","","bad\nname"})expect(!safe_path(name),"malicious filename");
    expect(safe_path("assets/data-v1.bin"),"portable path accepted");
    {auto duplicate=in;rejected([&]{create(tmp.path/"dup",manifest(),{in,duplicate});},"duplicate path");duplicate.path="DATA.bin";rejected([&]{create(tmp.path/"alias",manifest(),{in,duplicate});},"case alias");duplicate.path="data.bin/child";rejected([&]{create(tmp.path/"prefix",manifest(),{in,duplicate});},"prefix collision");}
    {auto second=in;second.path="other.bin";create(tmp.path/"two",manifest(),{in,second});auto bad=load(tmp.path/"two");auto t=u64(bad,48);auto record_size=uint64_t(bad[t])|(uint64_t(bad[t+1])<<8);field(bad,t+record_size+8,u64(bad,t+8));rejected([&]{memory(bad);},"overlapping payload records");
        if(sizeof(size_t)>=8) {
            // Virtual seekable file proves offsets beyond 4 GiB without allocating its payload.
            auto big=load(tmp.path/"two");auto first=u64(big,t+8);uint64_t large=(uint64_t(1)<<32)+8;
            auto second_offset=first+large;field(big,t+16,large);field(big,t+24,large);field(big,t+record_size+8,second_offset);
            auto end=second_offset+content.size();field(big,24,end);field(big,80,end-u64(big,72));
            Limits l;l.record=large;l.total=large+content.size();uint64_t highest_read=0;
            Package q(end,[&](uint64_t off,std::span<uint8_t> out){highest_read=std::max(highest_read,off);for(size_t j=0;j<out.size();++j)out[j]=off+j<big.size()?big[static_cast<size_t>(off+j)]:0;},l);
            expect(q.resource("other.bin").offset>(uint64_t(1)<<32)&&highest_read<1024*1024,"64-bit random-access index without payload reads");
        }
    }
    in.compression=1;create(tmp.path/"zstd",manifest(),{in});Package z(tmp.path/"zstd");z.validate();expect(z.read("data.bin")==content,"Zstd roundtrip");rejected([&]{z.read_slice("data.bin",0,1);},"compressed slice explicitly unsupported");
    {auto bad=load(tmp.path/"zstd");bad[z.resource("data.bin").offset]=0;auto q=memory(bad);rejected([&]{q.read("data.bin");},"invalid Zstd stream");}
    {auto bad=load(tmp.path/"zstd");field(bad,u64(bad,48)+24,1024*1024*32);rejected([&]{memory(bad);},"archive bomb ratio");}
    {Limits l;l.ratio=0;l.record=1024*1024*4;Bytes bomb(1024*1024*2);save(tmp.path/"bomb",bomb);Input b=in;b.file=tmp.path/"bomb";BuildOptions options;options.limits=l;rejected([&]{create(tmp.path/"bomb-pkg",manifest(),{b},options);},"builder rejects excessive expansion");}
    std::array<uint8_t,32> key{};for(size_t i=0;i<key.size();++i)key[i]=static_cast<uint8_t>(i);
    KeyProvider provider=[&](const std::string& id,auto& out){out=key;return id=="license-v1";};
    BuildOptions encrypted;encrypted.keys=provider;in.key_id="license-v1";
    create(tmp.path/"encrypted",manifest(),{in},encrypted);Package enc(tmp.path/"encrypted");
    expect(enc.metadata().id==p.metadata().id,"public encrypted metadata");expect(enc.read("data.bin",provider)==content,"encrypted Zstd roundtrip");
    rejected([&]{enc.read("data.bin");},"missing key");KeyProvider wrong=[](const auto&,auto& out){out.fill(99);return true;};
    rejected([&]{enc.read("data.bin",wrong);},"wrong encryption key");
    {auto bad=load(tmp.path/"encrypted");bad[u64(bad,48)+92]^=1;auto q=memory(bad);rejected([&]{q.read("data.bin",provider);},"corrupt auth tag");}
    {auto bad=load(tmp.path/"encrypted");bad[u64(bad,48)+128]='c';auto q=memory(bad);rejected([&]{q.read("cata.bin",provider);},"AEAD binds resource path");}
    create(tmp.path/"encrypted2",manifest(),{in},encrypted);expect(load(tmp.path/"encrypted")!=load(tmp.path/"encrypted2"),"fresh random nonce");
    in.key_id.clear();in.compression=0;
    Signing signing;sign(path,tmp.path/"signed","test-key",signing.pem);Package signed_p(tmp.path/"signed");signed_p.verify(signing.trust());expect(signed_p.signed_package(),"signature roundtrip");
    sign(path,tmp.path/"signed2","test-key",signing.pem);expect(load(tmp.path/"signed")==load(tmp.path/"signed2"),"reproducible signature");
    sign(tmp.path/"encrypted",tmp.path/"encrypted-signed","test-key",signing.pem);Package(tmp.path/"encrypted-signed").verify(signing.trust());expect(true,"sign encrypted without license");
    rejected([&]{p.verify(signing.trust());},"unsigned is not valid signature");
    rejected([&]{signed_p.verify([](const auto&,auto&){return false;});},"revoked signing key");
    {auto bad=load(tmp.path/"signed");bad[payload]^=1;auto q=memory(bad);rejected([&]{q.verify(signing.trust());},"signed payload tampering");}
    {auto bad=load(tmp.path/"signed");bad.back()^=1;auto q=memory(bad);rejected([&]{q.verify(signing.trust());},"signature tampering");}
    {auto bad=load(tmp.path/"signed");std::string id="xyz.test.plugin";auto at=std::search(bad.begin(),bad.end(),id.begin(),id.end());*(at+static_cast<ptrdiff_t>(id.size()-1))='X';auto q=memory(bad);rejected([&]{q.verify(signing.trust());},"signed metadata tampering");}
    {auto bad=load(tmp.path/"signed");auto where=u64(bad,88);bad[where+16]='x';auto q=memory(bad);rejected([&]{q.verify(signing.trust());},"signature key ID bound");}
    auto h=host();expect(preflight(p,h).at("status")=="SUPPORTED","preflight supported");h.require_signature=true;expect(preflight(p,h).at("status")=="INVALID_SIGNATURE","signature policy fails closed");h.trust=signing.trust();expect(preflight(signed_p,h).at("status")=="SUPPORTED","verified signature preflight");
    h=host();h.memory=1;expect(preflight(p,h).at("status")=="RESOURCE_LIMIT","preflight memory budget");h=host();h.memory=0;expect(preflight(p,h).at("status")=="UNKNOWN","unknown memory budget");
    h=host();h.runtimes=Json::object();expect(preflight(p,h).at("status")=="NEEDS_RUNTIME","runtime missing");h=host();h.api="3.0";expect(preflight(p,h).at("status")=="UNSUPPORTED","API bounds");h=host();expect(preflight(enc,h).at("status")=="NEEDS_LICENSE","preflight missing license");h.keys=provider;expect(preflight(enc,h).at("status")=="SUPPORTED","preflight available license");
    for(const auto& permissions:{"{format=\"bitmask\",registry=\"host.bits\",registry_version=\"1\",value=37}",
            "{format=\"bytes\",registry=\"host.bits\",registry_version=\"1\",values=[37,8,255,0]}"}) {
        auto target=tmp.path/(std::string(permissions).find("bitmask")!=std::string::npos?"mask":"bytes");create(target,manifest(permissions),{in});Package q(target);auto hh=host();
        expect(preflight(q,hh).at("status")=="UNSUPPORTED","permission registry mismatch");hh.permission_registries["host.bits"]="1";
        expect(preflight(q,hh).at("status")=="NEEDS_PERMISSION","permission grant absent");hh.grants["primary"]=permission_json(q.metadata().targets[0].required_permissions);
        expect(preflight(q,hh).at("status")=="SUPPORTED","explicit permission grant");
        auto repr=q.metadata().targets[0].required_permissions;
        if(repr.format==Permission::Format::bytes)expect(repr.bytes==Bytes({37,8,255,0}),"actual byte buffer");else expect(repr.mask==37,"actual numeric mask");
    }
    {create(tmp.path/"hexmask",manifest("{format=\"bitmask\",registry=\"host.bits\",registry_version=\"1\",value=0x25}"),{in});Package q(tmp.path/"hexmask");auto s=q.metadata_text();expect(s.find("0x25")==std::string::npos&&s.find("37")!=std::string::npos,"writer decimal permission integers");}
    {create(tmp.path/"maxmask",manifest("{format=\"bitmask\",registry=\"host.bits\",registry_version=\"1\",value=9223372036854775807}"),{in});expect(Package(tmp.path/"maxmask").metadata().targets[0].required_permissions.mask==INT64_MAX,"signed int64 mask boundary");}
    for(const auto& bad:{"{format=\"bitmask\",registry=\"r\",registry_version=\"1\",value=-1}","{format=\"bitmask\",registry=\"r\",registry_version=\"1\",value=9223372036854775808}",
        "{format=\"bitmask\",registry=\"r\",registry_version=\"1\",value=\"37\"}","{format=\"bytes\",registry=\"r\",registry_version=\"1\",values=[256]}",
        "{format=\"bytes\",registry=\"r\",registry_version=\"1\",values=[-1]}","{format=\"bytes\",registry=\"r\",registry_version=\"1\",values=[\"37\"]}"})
        rejected([&]{create(tmp.path/"badperm",manifest(bad),{in});},"invalid permission value");
    {create(tmp.path/"strings",manifest("[\"save\",\"custom.feature\"]"),{in});Package q(tmp.path/"strings");auto hh=host();expect(preflight(q,hh).at("status")=="NEEDS_PERMISSION","string permission missing");hh.grants["primary"]={"save","custom.feature"};expect(preflight(q,hh).at("status")=="SUPPORTED","host string permission grants");}
    {auto m=manifest();m+="[[data.targets]]\nid=\"alpha\"\nhost=\"xyz.test.host\"\napi_min=\"1.0\"\napi_max=\"2.0\"\nentrypoint=\"main\"\n";
        create(tmp.path/"tie",m,{in});expect(preflight(Package(tmp.path/"tie"),host()).at("target")=="alpha","equal priority lexicographic tie break");
        m+="[[data.targets]]\nid=\"other-host\"\nhost=\"xyz.other.host\"\napi_min=\"1.0\"\napi_max=\"2.0\"\nentrypoint=\"main\"\n";
        create(tmp.path/"multi",m,{in});auto hh=host();hh.id="xyz.other.host";expect(preflight(Package(tmp.path/"multi"),hh).at("target")=="other-host","multi-host target independent");}
    {auto m=manifest();m+="[[data.targets]]\nid=\"primary\"\nhost=\"xyz.other.host\"\napi_min=\"1.0\"\napi_max=\"2.0\"\nentrypoint=\"main\"\n";rejected([&]{create(tmp.path/"duplicate-target",m,{in});},"duplicate target invalid");}
    {auto m=manifest();m+="priority=10\n[[data.entrypoints]]\nid=\"wasm\"\nruntime=\"wasm\"\nruntime_min=\"1.0\"\nruntime_max=\"1.0\"\nresource=\"data.bin\"\n[[data.targets]]\nid=\"unavailable\"\nhost=\"xyz.test.host\"\napi_min=\"1.0\"\napi_max=\"2.0\"\nentrypoint=\"wasm\"\npriority=20\n";
        create(tmp.path/"fallback",m,{in});expect(preflight(Package(tmp.path/"fallback"),host()).at("target")=="primary","runtime fallback selects supported target");
        m+="fallback=false\n";create(tmp.path/"nofallback",m,{in});expect(preflight(Package(tmp.path/"nofallback"),host()).at("status")=="NEEDS_RUNTIME","fallback false blocks lower target");}
    {auto m=manifest();auto start=m.find("host=\"xyz.test.host\"");m.replace(start,std::string("host=\"xyz.test.host\"").size(),"host=\"*\"");create(tmp.path/"generic",m,{in});auto hh=host();expect(preflight(Package(tmp.path/"generic"),hh).at("status")=="UNSUPPORTED","generic opt-in required");hh.allow_generic=true;expect(preflight(Package(tmp.path/"generic"),hh).at("status")=="SUPPORTED","generic still checks compatibility");}
    std::vector<std::future<bool>> futures;for(int i=0;i<8;++i)futures.push_back(std::async(std::launch::async,[&]{for(int j=0;j<25;++j)if(p.read("data.bin")!=content)return false;return true;}));for(auto& f:futures)expect(f.get(),"concurrent reads");
    rejected([&]{p.read("data.bin",{},[]{return true;});},"read cancellation");
    BuildOptions interrupted;interrupted.cancel=[]{return true;};rejected([&]{create(tmp.path/"cancelled",manifest(),{in},interrupted);},"builder cancellation");expect(!std::filesystem::exists(tmp.path/"cancelled"),"cancelled output not published");
#if defined(__linux__) || defined(__APPLE__)
    p.extract(tmp.path/"extracted");expect(load(tmp.path/"extracted/data.bin")==content,"transactional extraction");rejected([&]{p.extract(tmp.path/"extracted");},"existing extraction rejected");
    std::filesystem::create_directory(tmp.path/"outside");std::filesystem::create_directory_symlink(tmp.path/"outside",tmp.path/"link");rejected([&]{p.extract(tmp.path/"link/escaped");},"symlink ancestor rejected");expect(!std::filesystem::exists(tmp.path/"outside/escaped"),"no symlink escape");
    std::filesystem::permissions(tmp.path/"outside",std::filesystem::perms::owner_all|std::filesystem::perms::group_write);
    rejected([&]{p.extract(tmp.path/"outside/shared-parent");},"writable shared extraction parent rejected");
    std::filesystem::permissions(tmp.path/"outside",std::filesystem::perms::owner_all);
    int cancellation=0;rejected([&]{p.extract(tmp.path/"interrupted",{}, {},[&]{return ++cancellation>=4;});},"extraction interruption");expect(!std::filesystem::exists(tmp.path/"interrupted"),"interrupted extraction not published");
    in.extraction=1;create(tmp.path/"direct",manifest(),{in});rejected([&]{Package(tmp.path/"direct").extract(tmp.path/"forbidden");},"direct-required forbids extraction");in.extraction=2;create(tmp.path/"extract-required",manifest(),{in});expect(preflight(Package(tmp.path/"extract-required"),host()).at("status")=="NEEDS_EXTRACTION","extract-required negotiation");in.extraction=0;
#endif
    Session session(p,host(),data_adapter());session.prepare();session.activate();session.start();expect(session.stage()==Stage::started,"data runtime lifecycle");session.stop();session.cleanup();expect(session.stage()==Stage::cleaned,"data session cleanup");
    ylp_package* c=nullptr;ylp_error error{};expect(ylp_open(path.string().c_str(),nullptr,&c,&error)==YLP_OK,"C ABI open");
    ylp_resource view{};view.struct_size=sizeof(view);view.abi_version=1;expect(ylp_resource_at(c,0,&view,&error)==YLP_OK&&view.size==content.size(),"C resource view");
    ylp_buffer out{};expect(ylp_read(c,"data.bin",nullptr,nullptr,nullptr,nullptr,&out,&error)==YLP_OK&&out.size==content.size(),"C buffer read");ylp_buffer_free(&out);expect(!out.data&&!out.size,"C buffer release");
    expect(ylp_read(c,"missing",nullptr,nullptr,nullptr,nullptr,&out,&error)!=YLP_OK&&!out.data,"C failure empties output");ylp_close(c);
    std::mt19937 rng(1);for(int i=0;i<3000;++i){auto bad=bytes;auto mutations=1+rng()%8;for(unsigned j=0;j<mutations;++j)bad[rng()%bad.size()]^=static_cast<uint8_t>(1+rng()%255);if(i%5==0)bad.resize(rng()%bad.size());try{auto q=memory(bad);q.validate();}catch(const Error&){} }expect(true,"3000 deterministic binary fuzz mutations");
    for(const auto& entry:std::filesystem::directory_iterator(tmp.path))expect(!entry.path().filename().string().starts_with(".ylplugin"),"no leaked staging directories");
}
}
int main(int argc,char** argv){try{if(argc==3&&std::string(argv[1])=="--native"){
#if !defined(__linux__)
    std::cout<<"Native execution test is Linux-only in this snapshot\n";return 77;
#else
    native_test(argv[2]);
#endif
}else core();std::cout<<checks<<" checks passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
