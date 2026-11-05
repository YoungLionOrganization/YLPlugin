// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include "../internal.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <cstdio>
#include <limits>
#include <set>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <zstd.h>

namespace ylp {
namespace {
constexpr std::array<uint8_t,8> magic{'Y','L','P','L','U','G',0,0};
constexpr std::array<uint8_t,8> sigmagic{'Y','L','S','I','G',0,0,0};
std::string path_text(const std::filesystem::path& path) {
    const auto utf8=path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()),utf8.size()};
}
std::string lower(std::string s) { for(auto& c:s) if(c>='A'&&c<='Z') c=static_cast<char>(c-'A'+'a'); return s; }
void paths(const std::vector<Resource>& resources) {
    std::set<std::string> names;
    for(const auto& r:resources) {
        require(safe_path(r.path),Code::invalid_container,"unsafe resource path: "+r.path);
        require(names.insert(lower(r.path)).second,Code::invalid_container,"duplicate or case-aliased path");
    }
    for(const auto& name:names) {
        for(size_t i=name.find('/');i!=std::string::npos;i=name.find('/',i+1))
            require(!names.contains(name.substr(0,i)),Code::invalid_container,"file/directory path collision");
    }
}
void references(const Metadata& m,const std::vector<Resource>& records) {
    for(const auto& e:m.entrypoints) {
        auto it=std::find_if(records.begin(),records.end(),[&](const auto& r){return r.path==e.resource;});
        require(it!=records.end(),Code::invalid_metadata,"entrypoint resource missing: "+e.resource);
        require(it->variant.empty()||it->variant==e.id,Code::invalid_metadata,"resource variant does not match entrypoint");
    }
    for(const auto& r:records) if(!r.variant.empty())
        require(std::any_of(m.entrypoints.begin(),m.entrypoints.end(),[&](const auto& e){return e.id==r.variant;}),
                Code::invalid_metadata,"unknown resource variant association");
}
Bytes make_header(uint64_t flags,uint64_t size,uint64_t meta,uint64_t toc_offset,uint64_t toc_length,
                  uint64_t count,uint64_t payload_offset,uint64_t payload_length,uint64_t sig_offset) {
    Bytes b(128); std::copy(magic.begin(),magic.end(),b.begin());
    put(b,8,1,2); put(b,12,128,4); put(b,16,flags,8); put(b,24,size,8);
    put(b,32,128,8); put(b,40,meta,8); put(b,48,toc_offset,8); put(b,56,toc_length,8);
    put(b,64,count,8); put(b,72,payload_offset,8); put(b,80,payload_length,8);
    put(b,88,sig_offset,8); put(b,96,sig_offset?128:0,8); return b;
}
struct KeyGuard {
    std::array<uint8_t,32> value{};
    ~KeyGuard() { OPENSSL_cleanse(value.data(),value.size()); }
};
struct FileSource {
    std::ifstream stream;
    std::mutex mutex;
    uint64_t size;
    explicit FileSource(const std::filesystem::path& path):stream(path,std::ios::binary|std::ios::ate) {
        if(!stream) fail(Code::io,"cannot open package: "+path_text(path));
        const auto end=stream.tellg();require(end>=0,Code::io,"package size unavailable");
        size=static_cast<uint64_t>(end);
    }
    void read(uint64_t offset,std::span<uint8_t> out) {
        require(offset<=size&&out.size()<=size-offset,Code::invalid_container,"read outside container");
        require(offset<=static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max())&&out.size()<=static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max()),Code::resource_limit,"file position exceeds I/O implementation");
        std::lock_guard lock(mutex);stream.clear();stream.seekg(static_cast<std::streamoff>(offset));
        stream.read(reinterpret_cast<char*>(out.data()),static_cast<std::streamsize>(out.size()));
        require(bool(stream),Code::io,"short package read");
    }
};
}
uint64_t get(std::span<const uint8_t> b,size_t off,size_t count) {
    require(count<=8&&off<=b.size()&&count<=b.size()-off,Code::invalid_container,"truncated binary field");
    uint64_t n=0; for(size_t i=0;i<count;++i) n|=uint64_t(b[off+i])<<(8*i); return n;
}
void put(Bytes& b,size_t off,uint64_t value,size_t count) {
    require(count<=8&&off<=b.size()&&count<=b.size()-off,Code::invalid_container,"binary write overflow");
    for(size_t i=0;i<count;++i) b[off+i]=static_cast<uint8_t>(value>>(8*i));
}
uint64_t align(uint64_t n,uint64_t a) {
    require(a&&std::has_single_bit(a)&&n<=UINT64_MAX-(a-1),Code::invalid_container,"alignment overflow");
    return (n+a-1)&~(a-1);
}
void zero(std::span<const uint8_t> b) {
    require(std::all_of(b.begin(),b.end(),[](uint8_t n){return n==0;}),Code::invalid_container,"nonzero reserved/padding bytes");
}
Bytes encode_record(const Resource& r) {
    require(r.path.size()<=1024&&!r.media.empty()&&r.media.size()<=255&&r.variant.size()<=255&&r.key_id.size()<=255,
            Code::invalid_container,"record string length limit");
    Bytes b(static_cast<size_t>(align(128+r.path.size()+r.media.size()+r.variant.size()+r.key_id.size(),8)));
    put(b,0,b.size(),4); b[4]=r.compression; b[5]=r.encryption; b[6]=r.extraction;
    put(b,8,r.offset,8); put(b,16,r.stored,8); put(b,24,r.size,8); put(b,32,r.alignment,4);
    put(b,36,r.path.size(),2); put(b,38,r.media.size(),2); put(b,40,r.variant.size(),2); put(b,42,r.key_id.size(),2);
    std::copy(r.hash.begin(),r.hash.end(),b.begin()+48); std::copy(r.nonce.begin(),r.nonce.end(),b.begin()+80);
    std::copy(r.tag.begin(),r.tag.end(),b.begin()+92);
    size_t pos=128;
    for(const auto* s:{&r.path,&r.media,&r.variant,&r.key_id}) { std::copy(s->begin(),s->end(),b.begin()+static_cast<ptrdiff_t>(pos)); pos+=s->size(); }
    return b;
}
Bytes file_bytes(const std::filesystem::path& p,uint64_t maximum) {
    std::ifstream f(p,std::ios::binary|std::ios::ate);
    if(!f) fail(Code::io,"cannot open input file: "+path_text(p));
    auto end=f.tellg(); require(end>=0,Code::io,"input size unavailable");
    auto n=static_cast<uint64_t>(end);
    require(n<=maximum&&n<=SIZE_MAX&&n<=static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max()),Code::resource_limit,"input exceeds limit");
    Bytes b(static_cast<size_t>(n)); f.seekg(0);
    if(n) f.read(reinterpret_cast<char*>(b.data()),static_cast<std::streamsize>(n));
    require(bool(f),Code::io,"short input read"); return b;
}
Output::Output(const std::filesystem::path& p):destination(p) {
    require(!p.filename().empty(),Code::io,"output filename required");
    std::array<uint8_t,16> random{};
    require(RAND_bytes(random.data(),static_cast<int>(random.size()))==1,Code::io,"secure random generator unavailable");
    temporary=p.parent_path()/(".ylplugin-"+hex(random));
    std::error_code ec;
    require(std::filesystem::create_directory(temporary,ec)&&!ec,Code::io,"cannot create output staging directory");
    std::filesystem::permissions(temporary,std::filesystem::perms::owner_all,std::filesystem::perm_options::replace,ec);
    if(ec) { std::filesystem::remove(temporary); fail(Code::io,"cannot secure staging directory"); }
    stream.open(temporary/"package",std::ios::binary|std::ios::in|std::ios::out|std::ios::trunc);
    if(!stream) { std::filesystem::remove_all(temporary); fail(Code::io,"cannot open staging output"); }
}
Output::~Output() { stream.close(); std::error_code ec; std::filesystem::remove_all(temporary,ec); }
void Output::write(std::span<const uint8_t> bytes) {
    require(bytes.size()<=static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max()),Code::resource_limit,"write size overflow");
    if(!bytes.empty()) stream.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    require(bool(stream),Code::io,"output write failed");
}
uint64_t Output::position() { auto p=stream.tellp(); require(p>=0,Code::io,"output position unavailable"); return static_cast<uint64_t>(p); }
void Output::pad(uint64_t boundary) { auto p=position(); auto end=align(p,boundary); write(Bytes(static_cast<size_t>(end-p))); }
void Output::publish() {
    stream.flush(); require(bool(stream),Code::io,"output flush failed"); stream.close();
    require(bool(stream),Code::io,"output close failed");
    std::error_code ec; std::filesystem::create_hard_link(temporary/"package",destination,ec);
    require(!ec,Code::io,"cannot publish without replacement: "+ec.message());
}
void Package::Impl::read_at(uint64_t offset,std::span<uint8_t> out) const {
    require(offset<=size&&out.size()<=size-offset,Code::invalid_container,"read outside container");
    if(!out.empty()) reader(offset,out);
}
Bytes Package::Impl::bytes(uint64_t offset,uint64_t count) const {
    require(count<=SIZE_MAX,Code::resource_limit,"region exceeds address space");
    Bytes b(static_cast<size_t>(count)); read_at(offset,b); return b;
}
Package::Package(const std::filesystem::path& path,Limits limits) {
    auto file=std::make_shared<FileSource>(path);
    impl_=std::make_shared<Impl>(); impl_->size=file->size; impl_->limits=limits;
    impl_->reader=[file](uint64_t offset,std::span<uint8_t> out) {
        file->read(offset,out);
    };
    impl_->parse();
}
Package::Package(uint64_t size,std::function<void(uint64_t,std::span<uint8_t>)> reader,Limits limits) {
    require(bool(reader),Code::io,"reader callback required");
    impl_=std::make_shared<Impl>(); impl_->size=size; impl_->reader=std::move(reader); impl_->limits=limits; impl_->parse();
}
std::string read_metadata(uint64_t size,std::function<void(uint64_t,std::span<uint8_t>)> reader,Limits limits) {
    require(bool(reader),Code::io,"reader callback required");
    require(size>=128,Code::invalid_container,"truncated container header");
    Bytes header(128);reader(0,header);
    require(std::equal(magic.begin(),magic.end(),header.begin()),Code::invalid_container,"invalid container magic");
    require(get(header,8,2)==1&&get(header,10,2)==0&&get(header,12,4)==128,Code::invalid_container,"unsupported container version/header");
    require((get(header,16,8)&~uint64_t(7))==0,Code::invalid_container,"unknown mandatory container feature");
    require(get(header,24,8)==size,Code::invalid_container,"physical/declarative length mismatch");
    zero(std::span(header).subspan(104));
    const auto offset=get(header,32,8),length=get(header,40,8);
    require(offset==128&&length>=32&&offset<=size&&length<=size-offset,Code::invalid_container,"invalid metadata region");
    require(length-32<=limits.metadata&&limits.metadata<=SIZE_MAX,Code::resource_limit,"metadata limit exceeded");
    struct Context {
        const std::function<void(uint64_t,std::span<uint8_t>)>* reader;
        uint64_t start,end;
        std::exception_ptr error;
    } context{&reader,offset,offset+length,{}};
    auto callback=[](void* user,uint64_t off,void* data,size_t count)->int {
        auto& c=*static_cast<Context*>(user);
        try {
            require(off>=c.start&&off<=c.end&&count<=c.end-off,Code::invalid_container,"read outside metadata frame");
            if(count)(*c.reader)(off,{static_cast<uint8_t*>(data),count});
            return 0;
        } catch(...) { c.error=std::current_exception();return -1; }
    };
    auto yl_limits=yl_default_limits();yl_limits.max_document_bytes=static_cast<size_t>(limits.metadata);
    yl_document* raw=nullptr;yl_error error{};uint64_t consumed=0;
    const auto status=yl_read_at(callback,&context,offset,length,&yl_limits,&raw,&consumed,&error);
    Document document(raw);
    if(context.error)std::rethrow_exception(context.error);
    require(status==YL_OK,status==YL_LIMIT?Code::resource_limit:Code::invalid_metadata,error.message);
    require(consumed==length,Code::invalid_container,"metadata locator must describe exactly one frame");
    YlBuffer text;require(yl_dump(document.get(),&text.value,&error)==YL_OK,Code::invalid_metadata,error.message);
    return {reinterpret_cast<const char*>(text.value.data),text.value.size};
}
std::string read_metadata(const std::filesystem::path& path,Limits limits) {
    // Disable stdio buffering: even a small header read must not prefetch the TOC/payload.
#ifdef _WIN32
    auto* raw=_wfopen(path.c_str(),L"rb");
    auto seek=[](FILE* file,int64_t offset,int origin){return _fseeki64(file,offset,origin);};
    auto tell=[](FILE* file){return _ftelli64(file);};
#else
    auto* raw=std::fopen(path.c_str(),"rb");
    auto seek=[](FILE* file,int64_t offset,int origin){return ::fseeko(file,static_cast<off_t>(offset),origin);};
    auto tell=[](FILE* file){return ::ftello(file);};
#endif
    require(raw!=nullptr,Code::io,"cannot open package: "+path_text(path));
    auto close=[](FILE* value){std::fclose(value);};
    std::unique_ptr<FILE,decltype(close)> file(raw,close);
    require(std::setvbuf(raw,nullptr,_IONBF,0)==0,Code::io,"cannot disable metadata reader buffering");
    require(seek(raw,0,SEEK_END)==0,Code::io,"package size unavailable");
    auto end=tell(raw);require(end>=0,Code::io,"package size unavailable");
    return read_metadata(static_cast<uint64_t>(end),[&](uint64_t offset,std::span<uint8_t> out){
        require(offset<=INT64_MAX,Code::resource_limit,"file position exceeds I/O implementation");
        require(seek(raw,static_cast<int64_t>(offset),SEEK_SET)==0,Code::io,"metadata seek failed");
        require(std::fread(out.data(),1,out.size(),raw)==out.size(),Code::io,"short metadata read");
    },limits);
}
void Package::Impl::parse() {
    require(size>=128,Code::invalid_container,"truncated container header"); header=bytes(0,128);
    require(std::equal(magic.begin(),magic.end(),header.begin()),Code::invalid_container,"invalid container magic");
    require(get(header,8,2)==1&&get(header,10,2)==0&&get(header,12,4)==128,Code::invalid_container,"unsupported container version/header");
    flags=get(header,16,8); require((flags&~uint64_t(7))==0,Code::invalid_container,"unknown mandatory container feature");
    require(get(header,24,8)==size,Code::invalid_container,"physical/declarative length mismatch"); zero(std::span(header).subspan(104));
    const auto mo=get(header,32,8), ml=get(header,40,8), to=get(header,48,8), tl=get(header,56,8), count=get(header,64,8);
    const auto po=get(header,72,8), pl=get(header,80,8), sl=get(header,96,8); sig_offset=get(header,88,8);
    auto fits=[&](uint64_t off,uint64_t len) { require(off<=size&&len<=size-off,Code::invalid_container,"region outside container/overflow"); };
    fits(mo,ml); fits(to,tl); fits(po,pl);
    require(mo==128&&ml>=32&&to==align(mo+ml,8)&&po==align(to+tl,8),Code::invalid_container,"noncanonical/overlapping regions");
    require(ml-32<=limits.metadata&&tl<=limits.toc&&count<=limits.records&&count<=tl/128,
            Code::resource_limit,"metadata/TOC/count limit exceeded");
    require(tl<=SIZE_MAX&&limits.metadata<=SIZE_MAX,Code::resource_limit,"address space limit");
    if(flags&4) {
        fits(sig_offset,sl);
        require(sl==128&&sig_offset==po+pl&&sl==size-sig_offset,Code::invalid_container,"invalid signature region");
        signature=bytes(sig_offset,sl);
        require(std::equal(sigmagic.begin(),sigmagic.end(),signature.begin())&&get(signature,8,4)==1&&get(signature,48,8)==sig_offset+64,
                Code::invalid_signature,"invalid signature descriptor");
        zero(std::span(signature).subspan(12,4)); zero(std::span(signature).subspan(56,8));
        auto key=std::span(signature).subspan(16,32); auto end=std::find(key.begin(),key.end(),0);
        require(end!=key.begin()&&end!=key.end(),Code::invalid_signature,"invalid signing key ID");
        for(auto i=key.begin();i!=end;++i) require((*i>='a'&&*i<='z')||(*i>='A'&&*i<='Z')||(*i>='0'&&*i<='9')||*i=='_'||*i=='-'||*i=='.',Code::invalid_signature,"invalid key ID character");
        zero(std::span<const uint8_t>(end,key.end()));
    } else require(sig_offset==0&&sl==0&&pl==size-po,Code::invalid_container,"unexpected trailing/signature bytes");
    struct Context { const Impl* impl; std::exception_ptr error; } context{this,{}};
    auto callback=[](void* c,uint64_t off,void* data,size_t n)->int {
        auto& x=*static_cast<Context*>(c);
        try { x.impl->read_at(off,{static_cast<uint8_t*>(data),n}); return 0; }
        catch(...) { x.error=std::current_exception(); return -1; }
    };
    yl_document* d=nullptr; yl_error error{}; uint64_t consumed=0; auto yl_limits=yl_default_limits();
    yl_limits.max_document_bytes=static_cast<size_t>(limits.metadata);
    auto status=yl_read_at(callback,&context,mo,ml,&yl_limits,&d,&consumed,&error);
    if(context.error) std::rethrow_exception(context.error);
    require(status==YL_OK,status==YL_LIMIT?Code::resource_limit:Code::invalid_metadata,error.message);
    document.reset(d); require(consumed==ml,Code::invalid_container,"metadata locator must describe exactly one frame");
    zero(bytes(mo+ml,to-mo-ml)); zero(bytes(to+tl,po-to-tl));
    auto toc=bytes(to,tl); size_t pos=0; uint64_t end=po,total=0,used_features=(flags&4);
    for(uint64_t i=0;i<count;++i) {
        require(pos<=toc.size()&&toc.size()-pos>=128,Code::invalid_container,"truncated TOC record");
        auto b=std::span<const uint8_t>(toc).subspan(pos); auto n=get(b,0,4);
        auto pathlen=get(b,36,2),medialen=get(b,38,2),variantlen=get(b,40,2),keylen=get(b,42,2);
        require(pathlen>=1&&pathlen<=1024&&medialen>=1&&medialen<=255&&variantlen<=255&&keylen<=255&&
                n==align(128+pathlen+medialen+variantlen+keylen,8)&&n<=b.size(),Code::invalid_container,"invalid TOC string/record lengths");
        b=b.first(static_cast<size_t>(n)); Resource r;
        r.compression=b[4]; r.encryption=b[5]; r.extraction=b[6];
        require(r.compression<=1&&r.encryption<=1&&r.extraction<=2,Code::unsupported,"unsupported record codec/encryption/extraction");
        zero(b.subspan(7,1)); zero(b.subspan(44,4)); zero(b.subspan(108,20));
        r.offset=get(b,8,8); r.stored=get(b,16,8); r.size=get(b,24,8); r.alignment=static_cast<uint32_t>(get(b,32,4));
        require(r.alignment>=8&&r.alignment<=4096&&std::has_single_bit(r.alignment),Code::invalid_container,"invalid record alignment");
        std::copy_n(b.begin()+48,32,r.hash.begin()); std::copy_n(b.begin()+80,12,r.nonce.begin()); std::copy_n(b.begin()+92,16,r.tag.begin());
        size_t sp=128;
        auto str=[&](uint64_t length) { std::string s(reinterpret_cast<const char*>(b.data()+sp),static_cast<size_t>(length)); sp+=static_cast<size_t>(length); require(s.find('\0')==std::string::npos,Code::invalid_container,"NUL in record string"); return s; };
        r.path=str(pathlen); r.media=str(medialen); r.variant=str(variantlen); r.key_id=str(keylen); zero(b.subspan(sp));
        require(std::all_of(r.media.begin(),r.media.end(),[](unsigned char c){return c>=33&&c<=126;}),Code::invalid_container,"media type must be printable ASCII");
        fits(r.offset,r.stored);
        require(r.offset==align(end,r.alignment)&&r.offset>=po&&r.offset<=po+pl&&r.stored<=po+pl-r.offset,
                Code::invalid_container,"overlapping/out-of-order/out-of-payload record");
        zero(bytes(end,r.offset-end)); end=r.offset+r.stored;
        require(r.size<=limits.record&&r.stored<=limits.record&&r.size<=SIZE_MAX&&r.size<=limits.total-total,Code::resource_limit,"record/aggregate size limit exceeded");
        total+=r.size;
        if(r.compression) {
            used_features|=1;
            require(r.size<=1024*1024 || (r.stored>0 && (r.size-1024*1024-1)/r.stored<limits.ratio),Code::resource_limit,"decompression ratio limit");
        } else require(r.stored==r.size,Code::invalid_container,"store size mismatch");
        auto identifier=[](const std::string& s) { return std::all_of(s.begin(),s.end(),[](unsigned char c){
            return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-';
        }); };
        require(identifier(r.variant)&&identifier(r.key_id),Code::invalid_container,"variant/key ID must be ASCII identifiers");
        if(r.encryption) { used_features|=2; require(!r.key_id.empty(),Code::invalid_container,"encrypted record key ID required"); }
        else { require(r.key_id.empty(),Code::invalid_container,"key ID on unencrypted record"); zero(r.nonce); zero(r.tag); }
        if(!resources.empty()) require(resources.back().path<r.path,Code::invalid_container,"TOC paths must be strictly sorted");
        resources.push_back(std::move(r)); pos+=static_cast<size_t>(n);
    }
    require(pos==toc.size()&&end==po+pl&&used_features==flags,Code::invalid_container,"TOC/payload/feature mismatch"); paths(resources);
}
const std::vector<Resource>& Package::resources() const { return impl_->resources; }
const yl_document* Package::document() const { return impl_->document.get(); }
const Metadata& Package::metadata() const {
    std::call_once(impl_->metadata_once,[&] {
        auto m=std::make_unique<Metadata>(validate_profile(document())); references(*m,resources()); impl_->metadata=std::move(m);
    }); return *impl_->metadata;
}
std::string Package::metadata_text() const {
    YlBuffer out; yl_error e{};
    require(yl_dump(document(),&out.value,&e)==YL_OK,Code::invalid_metadata,e.message);
    return {reinterpret_cast<const char*>(out.value.data),out.value.size};
}
const Resource& Package::resource(const std::string& path) const {
    auto it=std::lower_bound(resources().begin(),resources().end(),path,[](const Resource& r,const std::string& s){return r.path<s;});
    require(it!=resources().end()&&it->path==path,Code::invalid_metadata,"resource not found: "+path); return *it;
}
Bytes Package::read(const std::string& path,const KeyProvider& keys,const Cancel& cancel) const {
    cancelled(cancel); auto r=resource(path); auto b=impl_->bytes(r.offset,r.stored); cancelled(cancel);
    if(r.encryption) {
        KeyGuard key; require(keys&&keys(r.key_id,key.value),Code::needs_license,"license key unavailable: "+r.key_id);
        auto aad=encode_record(r); std::fill(aad.begin()+92,aad.begin()+108,0);
        b=crypt(false,b,aad,key.value,r);
    }
    if(r.compression) {
        require(ZSTD_findFrameCompressedSize(b.data(),b.size())==b.size(),Code::integrity,"invalid/trailing Zstd stream");
        auto content=ZSTD_getFrameContentSize(b.data(),b.size());
        require(content==r.size,Code::integrity,"Zstd content size mismatch/unknown");
        Bytes decoded(static_cast<size_t>(r.size));
        std::unique_ptr<ZSTD_DCtx,decltype(&ZSTD_freeDCtx)> ctx(ZSTD_createDCtx(),ZSTD_freeDCtx);
        require(bool(ctx),Code::memory,"Zstd context allocation");
        unsigned log=10; while(log<31&&(uint64_t(1)<<log)<std::max<uint64_t>(1024,impl_->limits.record)) ++log;
        require(!ZSTD_isError(ZSTD_DCtx_setParameter(ctx.get(),ZSTD_d_windowLogMax,static_cast<int>(log))),Code::integrity,"Zstd window limit");
        require(ZSTD_getDictID_fromFrame(b.data(),b.size())==0,Code::unsupported,"Zstd dictionaries are not supported");
        ZSTD_inBuffer input{b.data(),b.size(),0}; ZSTD_outBuffer output{decoded.data(),decoded.size(),0};
        size_t remaining=1;
        while(remaining) {
            cancelled(cancel); auto old_input=input.pos,old_output=output.pos;
            remaining=ZSTD_decompressStream(ctx.get(),&output,&input);
            require(!ZSTD_isError(remaining),Code::integrity,"invalid/oversized Zstd stream or window");
            require(!remaining||input.pos!=old_input||output.pos!=old_output,Code::integrity,"truncated/oversized Zstd stream");
        }
        require(input.pos==b.size()&&output.pos==r.size,Code::integrity,"Zstd decoded length mismatch"); b=std::move(decoded);
    }
    cancelled(cancel);
    require(b.size()==r.size&&CRYPTO_memcmp(sha256(b).data(),r.hash.data(),32)==0,Code::integrity,"resource SHA256 mismatch: "+path);
    return b;
}
Bytes Package::read_slice(const std::string& path,uint64_t offset,size_t count,const Cancel& cancel) const {
    cancelled(cancel);
    const auto& r=resource(path); require(!r.compression&&!r.encryption,Code::unsupported,"slices require plaintext store records");
    require(offset<=r.size&&count<=r.size-offset,Code::invalid_metadata,"slice outside resource");
    Bytes out(count),chunk(static_cast<size_t>(std::min<uint64_t>(1024*1024,r.size))); std::array<uint8_t,32> digest{};
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    require(ctx&&EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)==1,Code::integrity,"hash init failed");
    for(uint64_t pos=0;pos<r.size;) {
        cancelled(cancel); auto n=static_cast<size_t>(std::min<uint64_t>(chunk.size(),r.size-pos));
        impl_->read_at(r.offset+pos,std::span(chunk).first(n));
        require(EVP_DigestUpdate(ctx.get(),chunk.data(),n)==1,Code::integrity,"hash update failed");
        auto begin=std::max(pos,offset),end=std::min(pos+n,offset+count);
        if(begin<end) std::copy_n(chunk.begin()+static_cast<ptrdiff_t>(begin-pos),static_cast<size_t>(end-begin),out.begin()+static_cast<ptrdiff_t>(begin-offset));
        pos+=n;
    }
    cancelled(cancel);
    unsigned n=0; require(EVP_DigestFinal_ex(ctx.get(),digest.data(),&n)==1&&n==32&&CRYPTO_memcmp(digest.data(),r.hash.data(),32)==0,Code::integrity,"slice resource SHA256 mismatch"); return out;
}
void Package::validate(const KeyProvider& keys,const Cancel& cancel) const {
    cancelled(cancel); metadata();
    for(const auto& r:resources()) {
        cancelled(cancel);
        // A zero-length result still authenticates every source byte. Avoid
        // allocating the entire plaintext resource solely to discard it.
        if(!r.compression&&!r.encryption) read_slice(r.path,0,0,cancel);
        else read(r.path,keys,cancel);
    }
}
bool Package::signed_package() const { return !impl_->signature.empty(); }
std::string Package::signing_key_id() const {
    if(!signed_package()) return {};
    const char* p=reinterpret_cast<const char*>(impl_->signature.data()+16); return std::string(p);
}
Bytes Package::Impl::digest(const Cancel& cancel) const {
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    require(ctx&&EVP_DigestInit_ex(ctx.get(),EVP_sha512(),nullptr)==1,Code::invalid_signature,"signature hash init");
    Bytes chunk(1024*1024); const auto prefix=sig_offset+64;
    for(uint64_t off=0;off<prefix;) {
        cancelled(cancel); auto n=static_cast<size_t>(std::min<uint64_t>(chunk.size(),prefix-off));
        read_at(off,std::span(chunk).first(n));
        require(EVP_DigestUpdate(ctx.get(),chunk.data(),n)==1,Code::invalid_signature,"signature hash update"); off+=n;
    }
    static constexpr char domain[]="YLPlugin-SHA512-Ed25519-v1";
    Bytes out(reinterpret_cast<const uint8_t*>(domain),reinterpret_cast<const uint8_t*>(domain)+sizeof(domain));
    size_t start=out.size(); out.resize(start+64); unsigned n=0;
    require(EVP_DigestFinal_ex(ctx.get(),out.data()+start,&n)==1&&n==64,Code::invalid_signature,"signature hash final"); return out;
}
void Package::verify(const TrustProvider& trust,const Cancel& cancel) const {
    require(signed_package(),Code::invalid_signature,"package is unsigned");
    std::array<uint8_t,32> pub{};
    require(trust&&trust(signing_key_id(),pub),Code::invalid_signature,"unknown/revoked/untrusted signing key");
    require(verify_digest(pub,impl_->digest(cancel),std::span(impl_->signature).subspan(64)),Code::invalid_signature,"Ed25519 signature mismatch");
}
Json resource_json(const Resource& r) {
    return {{"path",r.path},{"media",r.media},{"variant",r.variant},{"offset",r.offset},{"stored",r.stored},
        {"size",r.size},{"compression",r.compression},{"encryption",r.encryption},{"key_id",r.key_id},
        {"extraction",r.extraction},{"alignment",r.alignment},{"sha256",hex(r.hash)}};
}
Json Package::inspect() const {
    Json j={{"container","1.0"},{"file_size",impl_->size},{"signed",signed_package()},
            {"signature_status",signed_package()?"UNVERIFIED":"UNSIGNED"},{"signing_key_id",signing_key_id()},
            {"metadata",metadata_text()},{"resources",Json::array()}};
    for(const auto& r:resources()) j["resources"].push_back(resource_json(r));
    try { const auto& m=metadata(); j["id"]=m.id; j["profile_status"]="VALID"; }
    catch(const Error& e) { j["profile_status"]=e.code==Code::unsupported_profile?"UNSUPPORTED_PROFILE":"INVALID"; j["profile_error"]=e.what(); }
    return j;
}
void create(const std::filesystem::path& output,const std::string& manifest,std::vector<Input> inputs,const BuildOptions& options) {
    auto doc=parse_manifest(manifest,options.limits.metadata); auto meta=validate_profile(doc.get());
    YlBuffer frame; yl_error e{};
    require(yl_pack(doc.get(),&frame.value,&e)==YL_OK,Code::invalid_metadata,e.message);
    Bytes metadata(frame.value.data,frame.value.data+frame.value.size);
    std::sort(inputs.begin(),inputs.end(),[](const Input& a,const Input& b){return a.path<b.path;});
    require(inputs.size()<=options.limits.records,Code::resource_limit,"too many resources");
    std::vector<Resource> records; uint64_t toc_length=0;
    for(const auto& in:inputs) {
        Resource r; r.path=in.path; r.media=in.media; r.variant=in.variant; r.key_id=in.key_id;
        r.compression=in.compression; r.encryption=in.key_id.empty()?0:1; r.extraction=in.extraction; r.alignment=in.alignment;
        require(r.compression<=1&&r.extraction<=2&&r.alignment>=8&&r.alignment<=4096&&std::has_single_bit(r.alignment),Code::invalid_metadata,"unsupported build record options");
        toc_length+=encode_record(r).size(); require(toc_length<=options.limits.toc,Code::resource_limit,"TOC limit"); records.push_back(std::move(r));
    }
    paths(records); references(meta,records);
    auto toc_offset=align(128+metadata.size(),8),payload_offset=align(toc_offset+toc_length,8);
    Output out(output); out.write(Bytes(128)); out.write(metadata); out.pad(8); out.write(Bytes(static_cast<size_t>(toc_length))); out.pad(8);
    uint64_t flags=0,total=0;
    for(size_t i=0;i<inputs.size();++i) {
        cancelled(options.cancel); auto& r=records[i]; auto raw=file_bytes(inputs[i].file,options.limits.record);
        r.size=raw.size(); require(r.size<=options.limits.total-total,Code::resource_limit,"aggregate plaintext limit"); total+=r.size;
        auto hash=sha256(raw); std::copy(hash.begin(),hash.end(),r.hash.begin()); Bytes stored;
        if(r.compression) {
            flags|=1; stored.resize(ZSTD_compressBound(raw.size()));
            auto n=ZSTD_compress(stored.data(),stored.size(),raw.data(),raw.size(),3);
            require(!ZSTD_isError(n),Code::integrity,"Zstd compression failed"); stored.resize(n);
            require(r.size<=1024*1024 || (stored.size()>0&&(r.size-1024*1024-1)/stored.size()<options.limits.ratio),Code::resource_limit,"build exceeds decompression ratio policy");
        } else stored=std::move(raw);
        require(stored.size()<=options.limits.record,Code::resource_limit,"stored record limit");
        out.pad(r.alignment); r.offset=out.position(); r.stored=stored.size();
        if(r.encryption) {
            flags|=2; KeyGuard key;
            require(options.keys&&options.keys(r.key_id,key.value),Code::needs_license,"encryption key unavailable");
            require(RAND_bytes(r.nonce.data(),static_cast<int>(r.nonce.size()))==1,Code::integrity,"nonce generation failed");
            stored=crypt(true,stored,encode_record(r),key.value,r);
        }
        out.write(stored);
    }
    const auto size=out.position(); auto header=make_header(flags,size,metadata.size(),toc_offset,toc_length,records.size(),payload_offset,size-payload_offset,0);
    out.stream.seekp(0); out.write(header); out.stream.seekp(static_cast<std::streamoff>(toc_offset));
    for(const auto& r:records) out.write(encode_record(r));
    out.stream.flush(); require(bool(out.stream),Code::io,"builder flush failed");
    // Parse the actual staged bytes before publishing; writer and reader share one contract.
    Package(out.temporary/"package",options.limits).metadata(); cancelled(options.cancel); out.publish();
}
void sign(const std::filesystem::path& input,const std::filesystem::path& output,const std::string& key_id,std::span<const uint8_t> pem,const Cancel& cancel) {
    require(!key_id.empty()&&key_id.size()<=31&&safe_path(key_id)&&key_id.find('/')==std::string::npos,Code::invalid_signature,"signing key ID must be a portable identifier of 1..31 bytes");
    Package source(input); source.metadata(); Output out(output);
    auto header=source.impl_->header; auto sig_offset=source.signed_package()?source.impl_->sig_offset:source.impl_->size;
    require(sig_offset<=UINT64_MAX-128,Code::resource_limit,"signed size overflow");
    put(header,16,get(header,16,8)|4,8); put(header,24,sig_offset+128,8); put(header,88,sig_offset,8); put(header,96,128,8);
    out.write(header); Bytes chunk(1024*1024);
    for(uint64_t pos=128;pos<sig_offset;) {
        cancelled(cancel); auto n=static_cast<size_t>(std::min<uint64_t>(chunk.size(),sig_offset-pos)); source.impl_->read_at(pos,std::span(chunk).first(n)); out.write(std::span(chunk).first(n)); pos+=n;
    }
    Bytes block(128); std::copy(sigmagic.begin(),sigmagic.end(),block.begin()); put(block,8,1,4);
    std::copy(key_id.begin(),key_id.end(),block.begin()+16); put(block,48,sig_offset+64,8); out.write(block);
    out.stream.flush(); Package staged(out.temporary/"package"); auto sig=sign_digest(pem,staged.impl_->digest(cancel));
    out.stream.seekp(static_cast<std::streamoff>(sig_offset+64)); out.write(sig); cancelled(cancel); out.publish();
}
} // namespace ylp
