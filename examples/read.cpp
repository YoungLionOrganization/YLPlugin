// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include <ylplugin/ylplugin.hpp>
#include <iostream>
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    try {
        ylp::Package p(argv[1]);p.validate();
        for(const auto& r:p.resources())std::cout<<r.path<<" "<<p.read(r.path).size()<<"\n";
    } catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
