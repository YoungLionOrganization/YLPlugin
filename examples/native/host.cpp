// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include <ylplugin/ylplugin.hpp>
#include <iostream>
#include <algorithm>
int main(int argc,char** argv) {
    if(argc!=4){std::cerr<<"usage: native_host PACKAGE NEW_PRIVATE_CACHE APPROVED_SHA256\n";return 2;}
    try {
        const auto approved=ylp::unhex(argv[3]);
        if(approved.size()!=32)throw std::runtime_error("approved SHA256 must contain exactly 32 bytes");
        ylp::Package package(argv[1]);
#if defined(_WIN32)
        const std::string os="windows";
#elif defined(__APPLE__)
        const std::string os="macos";
#else
        const std::string os="linux";
#endif
#if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
        const std::string arch="arm64";
#else
        const std::string arch="x86_64";
#endif
        auto host=ylp::host_from_json({{"id","xyz.example.native-host"},{"api","1.0"},{"os",os},{"arch",arch},{"abi","ylp-c1"},
            {"runtimes",{{"native","1.0"}}},{"memory",67108864},{"storage",67108864},{"allow_native",true},{"allow_extraction",true}});
        auto result=ylp::preflight(package,host);std::cout<<result.dump(2)<<'\n';
        if(result.at("status").get<std::string>()!="SUPPORTED")return 4;
        ylp::Session session(package,host,ylp::native_adapter(argv[2],[&](const auto&,const auto&,const auto& digest){
            // Approval comes from an external host-controlled allowlist/input,
            // not a comparison with the plugin's own claimed digest.
            return std::equal(approved.begin(),approved.end(),digest.begin());
        }));
        session.prepare();session.activate();session.start();session.stop();session.cleanup();return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
