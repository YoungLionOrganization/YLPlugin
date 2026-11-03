#pragma once
#include <ylplugin/ylplugin.hpp>
#include <variant>

namespace ylp::binding {
struct Handle {
    std::unique_ptr<Package> package;
    bool busy=false;
    explicit Handle(const std::string& path, const Json& limits);
};
using Result=std::variant<std::monostate,std::string,Bytes,Json>;
struct Callbacks { KeyProvider keys,trust; Cancel cancel; };
// All adapters use this dispatch into the SAME core. JSON is an internal argument
// transport, never another parser for the YLPlugin container.
Result invoke(Handle*,const std::string& operation,const Json&,std::span<const uint8_t>,const Callbacks&);
void close(Handle*);
}
