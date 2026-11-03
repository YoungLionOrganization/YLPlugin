// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include <ylplugin/qt.hpp>
#include <QCoreApplication>
#include <QFile>
#include <iostream>
#include <filesystem>

using namespace ylp;
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try{
        Package p(std::filesystem::path(YLP_SOURCE_DIR)/"examples/artifacts/demo.ylplugin");
        Host h=host_from_json({{"id","xyz.younglion.leominigames"},{"api","0.7"},{"os","linux"},{"arch","x86_64"},
            {"capabilities",{"resources"}},{"runtimes",{{"qt.rcc","1.0"}}},{"permission_formats",{"strings"}},{"memory",64*1024*1024}});
        auto approve=[](const std::string&,const Resource&){return true;};
        for(int i=0;i<10;++i){
            Session s(p,h,rcc_adapter("/__ylplugin_test",approve));s.prepare();s.activate();s.start();
            QFile qml(":/__ylplugin_test/Main.qml");check(qml.open(QIODevice::ReadOnly),"mounted QML missing");check(qml.readAll().contains("Tap to score"),"wrong QML bytes");qml.close();
            // QFile's engine can hold a QResource even after close, so destroy it before cleanup.
        }
        // The QFile in each loop is destroyed before the session (reverse declaration order).
        check(retained_rcc_buffers()==0,"normal engine/file teardown must free RCC buffer");
        {
            Session session(p,h,rcc_adapter("/__ylplugin_held",approve));session.prepare();session.activate();session.start();
            QFile held(":/__ylplugin_held/Main.qml");check(held.open(QIODevice::ReadOnly),"held QML open");
            session.cleanup();check(held.readAll().contains("Tap to score"),"buffer freed while QFile alive");
            check(retained_rcc_buffers()==1,"busy unregister must retain buffer");
        }
        Session denied(p,h,rcc_adapter("/__ylplugin_denied",[](const auto&,const auto&){return false;}));
        bool rejected=false;try{denied.prepare();}catch(const Error&){rejected=true;}check(rejected,"unapproved RCC reached Qt");
        std::cout<<"Qt memory RCC: repeated lifecycle, outstanding QFile, host denial passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
