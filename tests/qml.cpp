// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include <ylplugin/qt.hpp>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <iostream>

int main(int argc,char** argv) {
    QGuiApplication app(argc,argv);
    try {
        ylp::Package p(std::filesystem::path(YLP_SOURCE_DIR)/"examples/artifacts/demo.ylplugin");
        auto h=ylp::host_from_json({{"id","xyz.younglion.leominigames"},{"api","0.7"},{"os","linux"},{"arch","x86_64"},
            {"capabilities",{"resources"}},{"runtimes",{{"qt.rcc","1.0"}}},{"permission_formats",{"strings"}},{"memory",64*1024*1024}});
        for(int i=0;i<5;++i) {
            // The fixture is authored in this repository; this is explicit test-only approval.
            ylp::Session s(p,h,ylp::rcc_adapter("/__ylplugin_qml",[](const auto&,const auto&){return true;}));
            s.prepare();s.activate();s.start();
            {
                QQmlEngine engine;QQmlComponent component(&engine,QUrl("qrc:/__ylplugin_qml/Main.qml"));
                std::unique_ptr<QObject> root(component.create());
                if(!root)throw std::runtime_error(component.errorString().toStdString());
                if(!qobject_cast<QQuickItem*>(root.get())||root->property("score").toInt()!=0)throw std::runtime_error("QML root contract");
                if(!root->setProperty("score",7)||root->property("score").toInt()!=7)throw std::runtime_error("QML score property");
            } // QML objects and engine are destroyed before resource unmount.
            s.stop();s.cleanup();
        }
        if(ylp::retained_rcc_buffers()!=0)throw std::runtime_error("QML engine retained resources after teardown");
        std::cout<<"5 real QML engine creation/teardown cycles passed\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
