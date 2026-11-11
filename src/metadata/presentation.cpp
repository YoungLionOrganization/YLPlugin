// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include "../internal.hpp"
#include <ylplugin/presentation.hpp>
#include <algorithm>
#include <cstring>
#include <set>
#include <tuple>

namespace ylp {
namespace {
const yl_value* field(const yl_value* value,const char* key) {
    return yl_get(value,key,std::strlen(key));
}
void table(const yl_value* value) {
    require(value&&yl_value_type(value)==YL_TABLE,Code::invalid_metadata,"presentation value must be a table");
}
void keys(const yl_value* value,const std::set<std::string>& allowed) {
    table(value);
    for(size_t i=0;i<yl_count(value);++i) {
        size_t length=0;const char* key=yl_key(value,i,&length);
        require(allowed.contains(std::string(key,length)),Code::invalid_metadata,"unknown presentation field");
    }
}
std::string text(const yl_value* value,size_t maximum=255) {
    require(value&&yl_value_type(value)==YL_STRING,Code::invalid_metadata,"presentation string required");
    size_t size=0;const char* bytes=yl_value_text(value,&size);
    require(size&&size<=maximum,Code::invalid_metadata,"presentation string length exceeded");
    std::string result(bytes,size);
    require(result.find('\0')==std::string::npos,Code::invalid_metadata,"NUL in presentation string");
    return result;
}
uint64_t integer(const yl_value* value) {
    require(value&&yl_value_type(value)==YL_INTEGER&&yl_value_integer(value)>=0,
            Code::invalid_metadata,"presentation offset/length must be nonnegative signed-64 integers");
    return static_cast<uint64_t>(yl_value_integer(value));
}
bool role(const std::string& value) {
    static const std::set<std::string> values={"icon","readme","license","contributing","security","code_of_conduct","changelog","notice"};
    return values.contains(value);
}
bool identifier(const std::string& value) {
    return std::all_of(value.begin(),value.end(),[](unsigned char c){
        return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-';
    });
}
bool same_resource(const PresentationAsset& a,const PresentationAsset& b) {
    return std::tie(a.resource,a.offset,a.length,a.hash,a.media_type,a.format,a.encoding)==
           std::tie(b.resource,b.offset,b.length,b.hash,b.media_type,b.format,b.encoding);
}
}
std::optional<PresentationCatalog> parse_presentation_catalog(const yl_document* document,const PresentationLimits& limits) {
    require(document,Code::invalid_metadata,"YLSpec document required");
    const auto* extensions=field(yl_root(document),"extensions");
    if(!extensions)return std::nullopt;
    table(extensions);
    const auto* value=field(extensions,"xyz.younglion.ylplugin.presentation");
    if(!value)return std::nullopt;
    table(value);
    PresentationCatalog catalog;catalog.version=text(field(value,"version"));
    require(catalog.version=="1.0",Code::unsupported,"unsupported presentation catalog version");
    keys(value,{"version","primary","assets"});
    const auto* assets=field(value,"assets");
    require(assets&&yl_value_type(assets)==YL_ARRAY,Code::invalid_metadata,"presentation assets array required");
    require(yl_count(assets)<=limits.entries,Code::resource_limit,"presentation entry limit exceeded");
    uint64_t total=0;
    std::map<std::string,size_t> ids,resources;
    std::set<std::string> portable_paths;
    std::vector<std::pair<uint64_t,uint64_t>> intervals;
    for(size_t i=0;i<yl_count(assets);++i) {
        const auto* item=yl_at(assets,i);
        keys(item,{"id","role","resource","media_type","format","encoding","offset","length","sha256"});
        PresentationAsset asset;
        asset.id=text(field(item,"id"));asset.role=text(field(item,"role"));
        require(identifier(asset.id)&&role(asset.role),Code::invalid_metadata,"invalid presentation ID or role");
        require(ids.emplace(asset.id,i).second,Code::invalid_metadata,"duplicate presentation ID");
        if(!catalog.assets.empty()) {
            const auto& previous=catalog.assets.back();
            require(std::tie(previous.role,previous.id)<std::tie(asset.role,asset.id),Code::invalid_metadata,"presentation assets must be sorted by role and ID");
        }
        asset.resource=text(field(item,"resource"),1024);
        require(safe_path(asset.resource),Code::invalid_metadata,"unsafe presentation resource path");
        asset.media_type=text(field(item,"media_type"));asset.format=text(field(item,"format"));
        asset.offset=integer(field(item,"offset"));asset.length=integer(field(item,"length"));
        require(asset.offset%8==0,Code::invalid_metadata,"presentation offset must be 8-byte aligned");
        if(asset.role=="icon") {
            require(!field(item,"encoding"),Code::invalid_metadata,"icon must not declare text encoding");
            require((asset.format=="png"&&asset.media_type=="image/png")||
                    (asset.format=="webp"&&asset.media_type=="image/webp")||
                    (asset.format=="jpeg"&&asset.media_type=="image/jpeg"),Code::invalid_metadata,"unsupported icon format/media pair");
            require(asset.length>0,Code::invalid_metadata,"empty presentation icon");
            require(asset.length<=limits.icon,Code::resource_limit,"presentation icon limit exceeded");
        } else {
            asset.encoding=text(field(item,"encoding"));
            require(asset.encoding=="utf-8"&&((asset.format=="markdown"&&asset.media_type=="text/markdown")||
                    (asset.format=="text"&&asset.media_type=="text/plain")),Code::invalid_metadata,"unsupported document encoding/format/media pair");
            require(asset.length<=limits.text,Code::resource_limit,"presentation text limit exceeded");
        }
        require(total<=limits.total&&asset.length<=limits.total-total,Code::resource_limit,"presentation aggregate limit exceeded");
        total+=asset.length;
        const auto digest=text(field(item,"sha256"),64);
        require(digest.size()==64&&std::all_of(digest.begin(),digest.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),
                Code::invalid_metadata,"presentation hash must be 64 lowercase hex characters");
        if(asset.length==0)require(digest=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",Code::invalid_metadata,"empty document hash mismatch");
        const auto bytes=unhex(digest);std::copy(bytes.begin(),bytes.end(),asset.hash.begin());
        auto [existing,inserted]=resources.emplace(asset.resource,i);
        if(!inserted)require(same_resource(catalog.assets.at(existing->second),asset),Code::invalid_metadata,"inconsistent presentation resource alias");
        else {
            auto portable=asset.resource;
            for(char& c:portable)if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');
            require(portable_paths.insert(portable).second,Code::invalid_metadata,"case-aliased presentation resource paths");
            if(asset.length)intervals.emplace_back(asset.offset,asset.offset+asset.length);
        }
        catalog.assets.push_back(std::move(asset));
    }
    for(const auto& path:portable_paths)
        for(size_t pos=path.find('/');pos!=std::string::npos;pos=path.find('/',pos+1))
            require(!portable_paths.contains(path.substr(0,pos)),Code::invalid_metadata,"presentation file/directory path collision");
    std::sort(intervals.begin(),intervals.end());
    for(size_t i=1;i<intervals.size();++i)
        require(intervals[i].first>=intervals[i-1].second,Code::invalid_metadata,"overlapping presentation resources");
    if(const auto* primary=field(value,"primary")) {
        table(primary);
        for(size_t i=0;i<yl_count(primary);++i) {
            size_t length=0;const char* key=yl_key(primary,i,&length);std::string name(key,length);
            require(role(name),Code::invalid_metadata,"invalid primary presentation role");
            auto id=text(yl_at(primary,i));
            const auto found=ids.find(id);
            require(found!=ids.end()&&catalog.assets.at(found->second).role==name,Code::invalid_metadata,"primary presentation ID missing or role mismatch");
            catalog.primary.emplace(std::move(name),std::move(id));
        }
    }
    return catalog;
}
} // namespace ylp
