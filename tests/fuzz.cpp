// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include <ylplugin/ylplugin.hpp>
#include <algorithm>
#include <cstring>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data,size_t size) {
    try {
        ylp::Limits l;l.metadata=65536;l.toc=65536;l.records=128;l.record=1024*1024;l.total=4*1024*1024;
        auto reader=[data,size](uint64_t off,std::span<uint8_t> out){
            if(off>size||out.size()>size-off)throw ylp::Error(ylp::Code::io,"fuzz short read");
            std::memcpy(out.data(),data+off,out.size());
        };
        try{ylp::read_metadata(size,reader,l);}catch(const ylp::Error&){}
        ylp::Package p(size,reader,l);
        try{p.metadata();}catch(const ylp::Error&){}
        for(const auto& r:p.resources())try{p.read(r.path);}catch(const ylp::Error&){}
    }catch(const std::exception&){}
    return 0;
}
