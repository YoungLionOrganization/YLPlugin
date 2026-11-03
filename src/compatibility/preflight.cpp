// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include "../internal.hpp"
#include <algorithm>
#include <charconv>
#include <tuple>
#include <openssl/crypto.h>

namespace ylp {
namespace {
std::array<uint32_t,3> version(const std::string& value) {
    std::array<uint32_t,3> out{}; size_t start=0,part=0;
    require(!value.empty(),Code::invalid_metadata,"version must be numeric");
    while(start<value.size()) {
        require(part<3,Code::invalid_metadata,"version has too many components");
        size_t end=value.find('.',start); if(end==std::string::npos) end=value.size();
        auto [last,ec]=std::from_chars(value.data()+start,value.data()+end,out[part]);
        require(ec==std::errc()&&last==value.data()+end&&end>start,Code::invalid_metadata,"invalid numeric version");
        ++part; start=end+1;
    }
    require(value.back()!='.',Code::invalid_metadata,"trailing version separator"); return out;
}
bool in_range(const std::string& actual,const std::string& minimum,const std::string& maximum) {
    auto lo=version(minimum),hi=version(maximum);
    require(lo<=hi,Code::invalid_metadata,"inverted version range");
    auto a=version(actual); return lo<=a&&a<=hi;
}
bool has(const std::vector<std::string>& list,const std::string& s) { return std::find(list.begin(),list.end(),s)!=list.end(); }
bool empty(const Permission& p) {
    if(p.format==Permission::Format::strings) return p.strings.empty();
    if(p.format==Permission::Format::bitmask) return p.mask==0;
    return std::all_of(p.bytes.begin(),p.bytes.end(),[](uint8_t b){return b==0;});
}
std::string format(const Permission& p) {
    return p.format==Permission::Format::strings?"strings":p.format==Permission::Format::bitmask?"bitmask":"bytes";
}
bool supported(const Permission& p,const Host& host) {
    if(p.format==Permission::Format::strings&&p.strings.empty()) return true;
    if(!has(host.permission_formats,format(p))) return false;
    if(p.format!=Permission::Format::strings)
        return host.permission_registries.contains(p.registry)&&host.permission_registries.at(p.registry).is_string()&&
            host.permission_registries.at(p.registry).get<std::string>()==p.registry_version;
    return true;
}
bool granted(const Permission& p,const Json& grants) {
    if(empty(p)) return true;
    if(p.format==Permission::Format::strings) {
        if(!grants.is_array()) return false;
        for(const auto& s:p.strings) if(std::find(grants.begin(),grants.end(),Json(s))==grants.end()) return false;
        return true;
    }
    if(!grants.is_object()||grants.value("format",std::string())!=format(p)||
       grants.value("registry",std::string())!=p.registry||grants.value("registry_version",std::string())!=p.registry_version) return false;
    if(p.format==Permission::Format::bitmask) {
        if(!grants.contains("value")||!grants.at("value").is_number_integer()) return false;
        if(grants.at("value").is_number_unsigned() && grants.at("value").get<uint64_t>()>INT64_MAX) return false;
        auto mask=grants.at("value").get<int64_t>(); return mask>=0&&(static_cast<uint64_t>(mask)&p.mask)==p.mask;
    }
    if(!grants.contains("values")||!grants.at("values").is_array()) return false;
    const auto& values=grants.at("values");
    for(const auto& b:values) if(!b.is_number_integer()||b.get<int64_t>()<0||b.get<int64_t>()>255) return false;
    for(size_t i=0;i<p.bytes.size();++i) {
        auto b=i<values.size()?values.at(i).get<unsigned>():0;
        if((b&p.bytes[i])!=p.bytes[i]) return false;
    }
    return true;
}
Json candidate(const Package& package,const Host& host,const Target& t,const Entrypoint& e) {
    Json j={{"target",t.id},{"entrypoint",e.id},{"runtime",e.runtime},{"status","SUPPORTED"},
        {"reasons",Json::array()},{"missing_capabilities",Json::array()},{"required_permissions",permission_json(t.required_permissions)},
        {"optional_permissions",permission_json(t.optional_permissions)},{"optional_permissions_supported",supported(t.optional_permissions,host)},
        {"strategy","direct"},{"strategy_reason","resource supports indexed access"}};
    auto reject=[&](const char* status,const std::string& why) { if(j["status"].get<std::string>()=="SUPPORTED") j["status"]=status; j["reasons"].push_back(why); };
    if(!in_range(host.api,t.api_min,t.api_max)) reject("UNSUPPORTED","host API outside target range");
    if((e.os!="*"&&e.os!=host.os)||(e.arch!="*"&&e.arch!=host.arch)||(e.abi!="*"&&e.abi!=host.abi)) reject("UNSUPPORTED","OS, architecture or ABI does not match");
    if(!host.runtimes.contains(e.runtime)) reject("NEEDS_RUNTIME","runtime is not registered by host");
    else if(!in_range(host.runtimes.at(e.runtime).get<std::string>(),e.runtime_min,e.runtime_max)) reject("NEEDS_RUNTIME","runtime version outside entrypoint range");
    if(e.runtime=="native" && (!host.allow_native||host.os=="ios"||host.os=="android")) reject("UNSUPPORTED","host/platform prohibits external native code");
    for(const auto& c:t.required_capabilities) if(!has(host.capabilities,c)) {
        j["missing_capabilities"].push_back(c); reject("UNSUPPORTED","missing capability: "+c);
    }
    if(!supported(t.required_permissions,host)) reject("UNSUPPORTED","permission representation or exact registry version unsupported");
    else {
        Json none=Json::array(); const Json& grants=host.grants.contains(t.id)?host.grants.at(t.id):none;
        if(!granted(t.required_permissions,grants)) reject("NEEDS_PERMISSION","required host-scoped permission grant missing");
    }
    const auto& r=package.resource(e.resource);
    bool extraction=r.extraction==2||e.runtime=="native";
    if(extraction && r.extraction==1) reject("UNSUPPORTED","runtime requires extraction but resource is direct-required");
    if(extraction) {
        j["strategy"]="extracted"; j["strategy_reason"]="runtime or resource requires a filesystem path";
        if(!host.allow_extraction) reject("NEEDS_EXTRACTION","host must explicitly allow extraction");
        if(host.os=="windows") reject("UNSUPPORTED","this SDK has no secure Windows extraction backend yet");
    }
    uint64_t memory=std::max(t.memory,r.size),storage=std::max(t.storage,extraction?r.size:0);
    j["memory_required"]=memory; j["storage_required"]=storage;
    if((memory&&!host.memory)||(storage&&!host.storage)) reject("UNKNOWN","host memory/storage budget is unknown");
    if((host.memory&&memory>host.memory)||(host.storage&&storage>host.storage)) reject("RESOURCE_LIMIT","host resource budget exceeded");
    if(r.encryption) {
        struct ScopedKey {
            std::array<uint8_t,32> value{};
            ~ScopedKey() { OPENSSL_cleanse(value.data(),value.size()); }
        } key;
        bool available=host.keys&&host.keys(r.key_id,key.value);
        if(!available) reject("NEEDS_LICENSE","payload key/license unavailable");
    }
    return j;
}
}
Host host_from_json(const Json& j) {
    require(j.is_object(),Code::invalid_metadata,"host description must be an object"); Host h;
    h.id=j.at("id").get<std::string>(); h.api=j.at("api").get<std::string>();
    h.os=j.at("os").get<std::string>(); h.arch=j.at("arch").get<std::string>(); h.abi=j.value("abi",std::string("*"));
    h.target=j.value("target",std::string());
    version(h.api); require(!h.id.empty()&&h.id!="*",Code::invalid_metadata,"concrete host ID required");
    h.capabilities=j.value("capabilities",std::vector<std::string>{});
    h.runtimes=j.value("runtimes",Json::object()); h.permission_formats=j.value("permission_formats",std::vector<std::string>{});
    h.permission_registries=j.value("permission_registries",Json::object()); h.grants=j.value("grants",Json::object());
    require(h.runtimes.is_object()&&h.permission_registries.is_object()&&h.grants.is_object(),Code::invalid_metadata,"host registries/grants must be objects");
    for(auto it=h.runtimes.begin();it!=h.runtimes.end();++it) version(it.value().get<std::string>());
    h.allow_generic=j.value("allow_generic",false); h.allow_extraction=j.value("allow_extraction",false);
    h.allow_native=j.value("allow_native",false); h.require_signature=j.value("require_signature",false);
    for(const char* key:{"memory","storage"}) if(j.contains(key)) require(j.at(key).is_number_integer()&&
        (j.at(key).is_number_unsigned()||j.at(key).get<int64_t>()>=0),Code::invalid_metadata,"resource budget must be nonnegative integer");
    h.memory=j.value("memory",uint64_t(0)); h.storage=j.value("storage",uint64_t(0)); return h;
}
Json preflight(const Package& package,const Host& host) {
    Json out={{"status","UNSUPPORTED"},{"candidates",Json::array()},{"reasons",Json::array()},
        {"signature_status",package.signed_package()?"UNVERIFIED":"UNSIGNED"}};
    try {
        cancelled(host.cancel);
        if(package.signed_package() && host.trust) { package.verify(host.trust,host.cancel); out["signature_status"]="VALID"; }
        else if(host.require_signature) fail(Code::invalid_signature,"host requires an authenticated signature and trust provider");
        const auto& m=package.metadata(); std::vector<const Target*> targets;
        for(const auto& t:m.targets) if((host.target.empty()||t.id==host.target)&&(t.host==host.id||(t.host=="*"&&host.allow_generic))) targets.push_back(&t);
        std::sort(targets.begin(),targets.end(),[&](const Target* a,const Target* b) {
            if((a->host==host.id)!=(b->host==host.id)) return a->host==host.id;
            if(a->priority!=b->priority) return a->priority>b->priority;
            return a->id<b->id;
        });
        for(const auto* t:targets) {
            cancelled(host.cancel);
            const auto& e=*std::find_if(m.entrypoints.begin(),m.entrypoints.end(),[&](const auto& v){return v.id==t->entrypoint;});
            auto c=candidate(package,host,*t,e); out["candidates"].push_back(c);
            if(c["status"].get<std::string>()=="SUPPORTED") { for(auto it=c.begin();it!=c.end();++it) out[it.key()]=it.value(); return out; }
            if(out["candidates"].size()==1) for(auto it=c.begin();it!=c.end();++it) out[it.key()]=it.value();
            if(!t->fallback) break;
        }
        if(targets.empty()) out["reasons"].push_back("no host target matches; generic targets require explicit opt-in");
    } catch(const Error& e) {
        if(e.code==Code::cancelled) throw;
        const char* status=e.code==Code::unsupported_profile?"UNSUPPORTED_PROFILE":e.code==Code::invalid_signature?"INVALID_SIGNATURE":
            e.code==Code::resource_limit?"RESOURCE_LIMIT":e.code==Code::invalid_metadata?"INVALID_CONTAINER":"UNKNOWN";
        out["status"]=status; out["reasons"].push_back(e.what());
        if(e.code==Code::invalid_signature) out["signature_status"]="INVALID";
    }
    return out;
}
} // namespace ylp
