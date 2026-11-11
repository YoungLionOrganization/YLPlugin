// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include <ylplugin/presentation.hpp>
#include <fstream>
#include <iostream>
#include <limits>

using namespace ylp;
namespace {
using Value=std::unique_ptr<yl_value,decltype(&yl_value_free)>;
Value value(const Json& json) {
    if(json.is_string()){const auto s=json.get<std::string>();return {yl_string(s.data(),s.size()),yl_value_free};}
    if(json.is_boolean())return {yl_boolean(json.get<bool>()),yl_value_free};
    if(json.is_number_float())return {yl_float(json.get<double>()),yl_value_free};
    if(json.is_number_integer())return {yl_integer(json.get<int64_t>()),yl_value_free};
    Value result(yl_new(json.is_array()?YL_ARRAY:YL_TABLE),yl_value_free);
    for(auto it=json.begin();it!=json.end();++it) {
        auto child=value(it.value());yl_error error{};
        const auto status=json.is_array()?yl_append(result.get(),child.get(),&error):
            yl_set(result.get(),it.key().data(),it.key().size(),child.get(),&error);
        if(status!=YL_OK)throw std::runtime_error(error.message);
        child.release();
    }
    return result;
}
std::optional<PresentationCatalog> parse(const Json& catalog,const PresentationLimits& limits={},bool present=true) {
    Json json={{"_ylspec",{{"version","0.1"},{"profile","core.data"},{"profile_version","0.1"}}},{"data",Json::object()}};
    if(present)json["extensions"]={{"xyz.younglion.ylplugin.presentation",catalog}};
    auto root=value(json);yl_document* raw=nullptr;yl_error error{};
    if(yl_document_create(root.get(),nullptr,&raw,&error)!=YL_OK)throw std::runtime_error(error.message);
    root.release();std::unique_ptr<yl_document,decltype(&yl_document_free)> doc(raw,yl_document_free);
    return parse_presentation_catalog(doc.get(),limits);
}
}
int main() {
    try {
        std::ifstream file(std::filesystem::path(YLP_SOURCE_DIR)/"tests/presentation/catalog-cases.json");
        const auto vectors=Json::parse(file);size_t checks=0;
        for(const auto& test:vectors.at("cases")) {
            const auto expected=test.at("native_code").get<int>();int actual=0;
            try {
                auto parsed=parse(test.at("catalog"));if(!parsed)throw std::runtime_error("catalog disappeared");
                for(size_t i=0;i<parsed->assets.size();++i) {
                    const auto& declared=test.at("catalog").at("assets").at(i);const auto& asset=parsed->assets[i];
                    if(asset.offset!=declared.at("offset").get<uint64_t>()||asset.length!=declared.at("length").get<uint64_t>()||hex(asset.hash)!=declared.at("sha256").get<std::string>())
                        throw std::runtime_error("copied asset lost exact values");
                }
            }
            catch(const Error& error){actual=static_cast<int>(error.code);}
            if(actual!=expected)throw std::runtime_error(test.at("name").get<std::string>()+": expected "+std::to_string(expected)+", got "+std::to_string(actual));
            ++checks;
        }
        const auto& base=vectors.at("cases").at(0).at("catalog");
        auto copied=parse(base); // Document is already freed; the returned catalog owns all strings.
        if(copied->primary.at("readme")!="readme"||copied->assets.at(1).resource!="docs/README.md")throw std::runtime_error("catalog ownership");
        ++checks;
        if(parse(base,{},false))throw std::runtime_error("legacy document unexpectedly has catalog");
        ++checks;
        for(int mode=0;mode<4;++mode) {
            PresentationLimits limits;
            if(mode==0)limits.entries=2;
            if(mode==1)limits.text=1;
            if(mode==2)limits.icon=1;
            if(mode==3)limits.total=1;
            bool limited=false;try{parse(base,limits);}catch(const Error& error){limited=error.code==Code::resource_limit;}
            if(!limited)throw std::runtime_error("custom presentation limit ignored");
            ++checks;
        }
        bool null_rejected=false;try{parse_presentation_catalog(nullptr);}catch(const Error& error){null_rejected=error.code==Code::invalid_metadata;}
        if(!null_rejected)throw std::runtime_error("null document accepted");
        ++checks;
        std::cout<<checks<<" presentation catalog checks passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
