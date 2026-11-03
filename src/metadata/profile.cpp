// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include "../internal.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>

namespace ylp {
namespace {
const yl_value* field(const yl_value* v, const char* key) { return yl_get(v,key,std::strlen(key)); }
void table(const yl_value* v, const std::string& name) {
    require(v && yl_value_type(v)==YL_TABLE,Code::invalid_metadata,name+" must be a table");
}
std::string text_value(const yl_value* v, const std::string& name) {
    require(v && yl_value_type(v)==YL_STRING,Code::invalid_metadata,name+" must be a string");
    size_t n=0; const char* s=yl_value_text(v,&n);
    std::string out(s,n);
    require(!out.empty() && out.size()<=255 && out.find('\0')==std::string::npos,
            Code::invalid_metadata,name+" must contain 1..255 non-NUL bytes");
    return out;
}
std::string str(const yl_value* v, const char* key, const std::string& fallback={}) {
    auto p=field(v,key); return p?text_value(p,key):fallback;
}
int64_t integer(const yl_value* v, const char* key, int64_t fallback=0) {
    auto p=field(v,key); if(!p) return fallback;
    require(yl_value_type(p)==YL_INTEGER,Code::invalid_metadata,std::string(key)+" must be an integer");
    return yl_value_integer(p);
}
bool boolean(const yl_value* v, const char* key, bool fallback) {
    auto p=field(v,key); if(!p) return fallback;
    require(yl_value_type(p)==YL_BOOLEAN,Code::invalid_metadata,std::string(key)+" must be boolean");
    return yl_value_boolean(p)!=0;
}
void keys(const yl_value* v, const std::set<std::string>& allowed) {
    table(v,"record");
    for(size_t i=0;i<yl_count(v);++i) {
        size_t n=0; const char* k=yl_key(v,i,&n);
        require(allowed.contains(std::string(k,n)),Code::invalid_metadata,"unknown profile field: "+std::string(k,n));
    }
}
std::vector<std::string> strings(const yl_value* v) {
    if(!v) return {};
    require(yl_value_type(v)==YL_ARRAY,Code::invalid_metadata,"expected string array");
    std::vector<std::string> out; std::set<std::string> unique;
    for(size_t i=0;i<yl_count(v);++i) {
        auto s=text_value(yl_at(v,i),"array element");
        require(unique.insert(s).second,Code::invalid_metadata,"duplicate array identifier");
        out.push_back(std::move(s));
    }
    return out;
}
Permission permission(const yl_value* v) {
    Permission p;
    if(!v) return p;
    if(yl_value_type(v)==YL_ARRAY) { p.strings=strings(v); return p; }
    table(v,"permission");
    auto format=str(v,"format");
    p.registry=str(v,"registry"); p.registry_version=str(v,"registry_version");
    require(!p.registry.empty() && !p.registry_version.empty(),Code::invalid_metadata,"permission registry and version required");
    if(format=="bitmask") {
        keys(v,{"format","registry","registry_version","value"});
        require(field(v,"value"),Code::invalid_metadata,"bitmask value required");
        auto n=integer(v,"value");
        require(n>=0,Code::invalid_metadata,"bitmask must be a nonnegative signed 64-bit integer");
        p.format=Permission::Format::bitmask; p.mask=static_cast<uint64_t>(n);
    } else if(format=="bytes") {
        keys(v,{"format","registry","registry_version","values"});
        auto values=field(v,"values");
        require(values && yl_value_type(values)==YL_ARRAY && yl_count(values)<=8192,
                Code::invalid_metadata,"permission values must be a byte array of at most 8192 bytes");
        p.format=Permission::Format::bytes;
        for(size_t i=0;i<yl_count(values);++i) {
            auto n=yl_at(values,i);
            require(yl_value_type(n)==YL_INTEGER && yl_value_integer(n)>=0 && yl_value_integer(n)<=255,
                    Code::invalid_metadata,"permission byte must be integer 0..255");
            p.bytes.push_back(static_cast<uint8_t>(yl_value_integer(n)));
        }
    } else fail(Code::invalid_metadata,"unknown permission representation");
    return p;
}
bool identifier(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(),s.end(),[](unsigned char c){
        return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-';
    });
}
bool version(const std::string& s) {
    if(s.empty()) return false;
    unsigned parts=1, digits=0; uint64_t component=0;
    for(unsigned char c:s) {
        if(c=='.') { if(!digits || ++parts>3) return false; digits=0; component=0; }
        else if(c>='0'&&c<='9') { if(++digits>9) return false; component=component*10+(c-'0'); }
        else return false;
    }
    return digits>0;
}
std::array<uint32_t,3> version_order(const std::string& s) {
    std::array<uint32_t,3> parts{}; size_t start=0,index=0;
    while(start<s.size()) {
        auto end=s.find('.',start);if(end==std::string::npos)end=s.size();
        parts.at(index++)=static_cast<uint32_t>(std::stoul(s.substr(start,end-start)));start=end+1;
    }
    return parts;
}
Metadata parse(const yl_document* doc) {
    auto d=field(yl_root(doc),"data");
    keys(d,{"id","version","publisher","targets","entrypoints"});
    Metadata m; m.id=str(d,"id"); m.version=str(d,"version"); m.publisher=str(d,"publisher");
    require(identifier(m.id) && version(m.version),Code::invalid_metadata,"plugin id and numeric version required");
    auto entries=field(d,"entrypoints"); auto targets=field(d,"targets");
    require(entries && targets && yl_value_type(entries)==YL_ARRAY && yl_value_type(targets)==YL_ARRAY
            && yl_count(entries)>0 && yl_count(targets)>0,Code::invalid_metadata,"nonempty targets and entrypoints arrays required");
    std::set<std::string> ids;
    for(size_t i=0;i<yl_count(entries);++i) {
        auto e=yl_at(entries,i);
        keys(e,{"id","runtime","runtime_min","runtime_max","resource","os","arch","abi"});
        Entrypoint a;
        a.id=str(e,"id"); a.runtime=str(e,"runtime"); a.runtime_min=str(e,"runtime_min");
        a.runtime_max=str(e,"runtime_max"); a.resource=str(e,"resource");
        a.os=str(e,"os","*"); a.arch=str(e,"arch","*"); a.abi=str(e,"abi","*");
        require(identifier(a.id)&&identifier(a.runtime)&&ids.insert(a.id).second&&safe_path(a.resource),
                Code::invalid_metadata,"invalid or duplicate entrypoint / resource path");
        require(version(a.runtime_min)&&version(a.runtime_max),Code::invalid_metadata,"explicit numeric runtime bounds required");
        require(version_order(a.runtime_min)<=version_order(a.runtime_max),Code::invalid_metadata,"inverted runtime version range");
        if(a.runtime=="native") require(a.os!="*"&&a.arch!="*"&&a.abi!="*",Code::invalid_metadata,"native entrypoint needs concrete OS, architecture and ABI");
        m.entrypoints.push_back(std::move(a));
    }
    std::set<std::string> tids;
    for(size_t i=0;i<yl_count(targets);++i) {
        auto t=yl_at(targets,i);
        keys(t,{"id","host","api_min","api_max","entrypoint","priority","fallback","required_capabilities",
                "optional_capabilities","required_permissions","optional_permissions","memory","storage"});
        Target a; a.id=str(t,"id"); a.host=str(t,"host"); a.api_min=str(t,"api_min"); a.api_max=str(t,"api_max");
        a.entrypoint=str(t,"entrypoint"); a.priority=integer(t,"priority"); a.fallback=boolean(t,"fallback",true);
        a.required_capabilities=strings(field(t,"required_capabilities"));
        a.optional_capabilities=strings(field(t,"optional_capabilities"));
        a.required_permissions=permission(field(t,"required_permissions"));
        a.optional_permissions=permission(field(t,"optional_permissions"));
        auto mem=integer(t,"memory"), storage=integer(t,"storage");
        require(mem>=0&&storage>=0,Code::invalid_metadata,"memory and storage cannot be negative");
        a.memory=static_cast<uint64_t>(mem); a.storage=static_cast<uint64_t>(storage);
        require(identifier(a.id)&&(identifier(a.host)||a.host=="*")&&tids.insert(a.id).second&&ids.contains(a.entrypoint),
                Code::invalid_metadata,"invalid/duplicate target or missing entrypoint reference");
        require(version(a.api_min)&&version(a.api_max),Code::invalid_metadata,"explicit numeric host API bounds required");
        require(version_order(a.api_min)<=version_order(a.api_max),Code::invalid_metadata,"inverted host API range");
        m.targets.push_back(std::move(a));
    }
    return m;
}
}
bool safe_path(const std::string& s) {
    if(s.empty() || s.size()>1024 || s.front()=='/' || s.back()=='/') return false;
    for(unsigned char c:s) if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'||c=='/')) return false;
    size_t start=0;
    while(start<s.size()) {
        size_t end=s.find('/',start); if(end==std::string::npos) end=s.size();
        std::string part=s.substr(start,end-start);
        if(part.empty()||part=="."||part==".."||part.size()>255||part.back()=='.') return false;
        auto base=part.substr(0,part.find('.'));
        for(char& c:base) c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if(base=="CON"||base=="PRN"||base=="AUX"||base=="NUL"||base=="CONIN$"||base=="CONOUT$") return false;
        if(base.size()==4 && (base.starts_with("COM")||base.starts_with("LPT")) && base[3]>='0'&&base[3]<='9') return false;
        start=end+1;
    }
    return true;
}
yl_status profile_validator(const yl_document* d, void*, yl_error* e) noexcept {
    try { parse(d); return YL_OK; }
    catch(const std::exception& ex) {
        if(e) { *e={}; e->code=YL_PROFILE; std::snprintf(e->message,sizeof(e->message),"%s",ex.what()); }
        return YL_PROFILE;
    } catch(...) { return YL_MEMORY; }
}
Metadata validate_profile(const yl_document* d) {
    yl_profile profile{"xyz.younglion.plugin","1.0",profile_validator,nullptr};
    yl_registry registry{&profile,1,nullptr,0}; yl_error error{};
    auto status=yl_validate(d,&registry,&error);
    if(status!=YL_OK) fail(status==YL_UNSUPPORTED?Code::unsupported_profile:Code::invalid_metadata,error.message);
    return parse(d);
}
Document parse_manifest(const std::string& text, uint64_t limit) {
    auto limits=yl_default_limits(); limits.max_document_bytes=static_cast<size_t>(std::min<uint64_t>(limit,SIZE_MAX));
    yl_document* d=nullptr; yl_error e{};
    auto status=yl_parse(text.data(),text.size(),&limits,&d,&e);
    require(status==YL_OK,status==YL_LIMIT?Code::resource_limit:Code::invalid_metadata,e.message);
    return Document(d);
}
Json permission_json(const Permission& p) {
    if(p.format==Permission::Format::strings) return p.strings;
    Json j={{"format",p.format==Permission::Format::bitmask?"bitmask":"bytes"},
            {"registry",p.registry},{"registry_version",p.registry_version}};
    if(p.format==Permission::Format::bitmask) j["value"]=p.mask;
    else { j["values"]=Json::array(); for(auto b:p.bytes) j["values"].push_back(b); }
    return j;
}
} // namespace ylp
