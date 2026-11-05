// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include "api.hpp"
#include <ylplugin/ylplugin.h>

namespace ylp::binding {
namespace {
void check(bool condition,const char* text) { if(!condition)throw Error(Code::invalid_metadata,text); }
Limits limits_from(const Json& j) {
    Limits l;
    auto field=[&](const char* name,uint64_t& out) {
        if(!j.contains(name))return;
        const auto& value=j.at(name);
        check(value.is_number_integer()&&(value.is_number_unsigned()||value.get<int64_t>()>=0),"limits must be nonnegative integers");
        out=value.get<uint64_t>();
    };
    check(j.is_object(),"limits must be an object");
    field("metadata",l.metadata);field("toc",l.toc);field("records",l.records);
    field("record",l.record);field("total",l.total);field("ratio",l.ratio);return l;
}
std::filesystem::path path(const std::string& text) {
    check(text.find('\0')==std::string::npos,"NUL in filesystem path");
    return std::filesystem::path(reinterpret_cast<const char8_t*>(text.c_str()));
}
struct Busy {
    Handle* handle;
    explicit Busy(Handle* h):handle(h) {if(h){check(!h->busy,"reentrant package operations are not allowed");h->busy=true;}}
    ~Busy(){if(handle)handle->busy=false;}
};
}
Handle::Handle(const std::string& name,const Json& limits):package(std::make_unique<Package>(path(name),limits_from(limits))) {}
void close(Handle* h) {check(h&&!h->busy,"cannot close a package during a callback");h->package.reset();}
Result invoke(Handle* h,const std::string& op,const Json& args,std::span<const uint8_t> binary,const Callbacks& cb) {
    Busy busy(h);
    if(op=="version")return std::string(ylp_version());
    if(op=="readMetadata")return ylp::read_metadata(path(args.at("path").get<std::string>()),limits_from(args.value("limits",Json::object())));
    if(op=="create") {
        std::vector<Input> inputs;
        for(const auto& j:args.at("resources")) {
            Input in;in.path=j.at("path").get<std::string>();in.file=path(j.at("file").get<std::string>());
            in.media=j.value("media",std::string("application/octet-stream"));in.variant=j.value("variant",std::string());
            in.key_id=j.value("key_id",std::string());auto compression=j.value("compression",0),extraction=j.value("extraction",0);
            check(compression>=0&&compression<=1&&extraction>=0&&extraction<=2,"invalid compression/extraction selector");
            in.compression=static_cast<uint8_t>(compression);in.extraction=static_cast<uint8_t>(extraction);
            if(j.contains("alignment"))check(j.at("alignment").is_number_integer()&&
                (j.at("alignment").is_number_unsigned()||j.at("alignment").get<int64_t>()>=0)&&j.at("alignment").get<uint64_t>()<=UINT32_MAX,"alignment must fit uint32");
            in.alignment=j.value("alignment",uint32_t(8));
            inputs.push_back(std::move(in));
        }
        BuildOptions options;options.keys=cb.keys;options.cancel=cb.cancel;
        create(path(args.at("output").get<std::string>()),args.at("manifest").get<std::string>(),std::move(inputs),options);return {};
    }
    if(op=="sign") {
        sign(path(args.at("input").get<std::string>()),path(args.at("output").get<std::string>()),args.at("keyId").get<std::string>(),binary,cb.cancel);return {};
    }
    check(h&&h->package,"package is closed");auto& p=*h->package;
    if(op=="metadata")return p.metadata_text();
    if(op=="inspect")return p.inspect();
    if(op=="targets") {
        Json rows=Json::array();for(const auto& t:p.metadata().targets) rows.push_back({{"id",t.id},{"host",t.host},{"api_min",t.api_min},
            {"api_max",t.api_max},{"entrypoint",t.entrypoint},{"priority",t.priority},{"fallback",t.fallback},
            {"required_capabilities",t.required_capabilities},{"optional_capabilities",t.optional_capabilities},
            {"required_permissions",permission_json(t.required_permissions)},{"optional_permissions",permission_json(t.optional_permissions)},
            {"memory",t.memory},{"storage",t.storage}});return rows;
    }
    if(op=="entrypoints") {
        Json rows=Json::array();for(const auto& e:p.metadata().entrypoints)rows.push_back({{"id",e.id},{"runtime",e.runtime},
            {"runtime_min",e.runtime_min},{"runtime_max",e.runtime_max},{"resource",e.resource},{"os",e.os},{"arch",e.arch},{"abi",e.abi}});return rows;
    }
    if(op=="permissionBytes") {
        const auto& index=args.at("target");
        check(index.is_number_integer()&&(index.is_number_unsigned()||index.get<int64_t>()>=0)&&index.get<uint64_t>()<=SIZE_MAX,"invalid target index");
        const auto& target=p.metadata().targets.at(static_cast<size_t>(index.get<uint64_t>()));
        const auto& permission=args.value("optional",false)?target.optional_permissions:target.required_permissions;
        check(permission.format==Permission::Format::bytes,"permission representation is not bytes");return permission.bytes;
    }
    if(op=="read")return p.read(args.at("path").get<std::string>(),cb.keys,cb.cancel);
    if(op=="slice") {
        for(const char* key:{"offset","count"})check(args.at(key).is_number_integer()&&
            (args.at(key).is_number_unsigned()||args.at(key).get<int64_t>()>=0),"slice range must be nonnegative integers");
        auto count=args.at("count").get<uint64_t>();check(count<=SIZE_MAX,"slice exceeds address space");
        return p.read_slice(args.at("path").get<std::string>(),args.at("offset").get<uint64_t>(),static_cast<size_t>(count),cb.cancel);
    }
    if(op=="validate"){p.validate(cb.keys,cb.cancel);return {};}
    if(op=="verify"){p.verify(cb.trust,cb.cancel);return {};}
    if(op=="check") {auto host=host_from_json(args.at("host"));host.keys=cb.keys;host.trust=cb.trust;host.cancel=cb.cancel;return preflight(p,host);}
    if(op=="extract"){p.extract(path(args.at("destination").get<std::string>()),args.value("paths",std::vector<std::string>{}),cb.keys,cb.cancel);return {};}
    throw Error(Code::unsupported,"unknown binding operation");
}
}
