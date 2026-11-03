// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

#include <ylplugin/qt.hpp>
#include <QResource>
#include <QDir>
#include <mutex>

namespace ylp {
namespace {
std::mutex& mutex() { static auto* m=new std::mutex; return *m; }
std::vector<Bytes>& retained() { static auto* buffers=new std::vector<Bytes>;return *buffers; }
class RccAdapter final:public Adapter {
    QString root_;RccApproval approval_;Bytes bytes_;bool mounted_=false;
public:
    RccAdapter(QString root,RccApproval approval):root_(std::move(root)),approval_(std::move(approval)) {
        if(!root_.startsWith('/')||root_.contains("..")||root_.contains('\\')||QDir::cleanPath(root_)!=root_)
            throw Error(Code::permission,"normalized absolute Qt mount root required");
    }
    ~RccAdapter() override {cleanup();}
    std::string id() const override {return "qt.rcc";}
    std::string version() const override {return "1.0";}
    void validate(const Package& p,const Entrypoint& e) override {
        if(e.runtime!=id()||p.resource(e.resource).media!="application/x-qt-resource")
            throw Error(Code::runtime,"qt.rcc entrypoint/media mismatch");
        if(p.resource(e.resource).extraction==2)throw Error(Code::unsupported,"memory RCC adapter cannot honor extract-required");
    }
    void prepare(const Package& p,const Entrypoint& e,const Host& host,const Cancel& cancel) override {
        if(!approval_||!approval_(p.metadata().id,p.resource(e.resource)))throw Error(Code::permission,"host must approve RCC bytes before Qt parses them");
        bytes_=p.read(e.resource,host.keys,cancel);
        if(bytes_.size()<20||std::string(bytes_.begin(),bytes_.begin()+4)!="qres")throw Error(Code::runtime,"invalid RCC header");
        // QResource has no length-bearing overload. The host owns the independent
        // RCC validator/trust boundary; this is not a replacement parser.
    }
    void activate() override {
        std::lock_guard lock(mutex());
        if(bytes_.empty()||mounted_)throw Error(Code::runtime,"RCC adapter not prepared/already mounted");
        if(!QResource::registerResource(bytes_.data(),root_))throw Error(Code::runtime,"Qt rejected RCC resource");mounted_=true;
    }
    void start() override {if(!mounted_)throw Error(Code::runtime,"RCC adapter is not active");}
    void stop() noexcept override {} // Host tears down QML engines and files before cleanup.
    void cleanup() noexcept override {
        std::lock_guard lock(mutex());
        if(mounted_) {
            if(!QResource::unregisterResource(bytes_.data(),root_)) {
                // Preserve lifetime even for QFile handles that outlive their engine.
                // Allocation failure cannot safely free referenced bytes: terminate.
                retained().push_back(std::move(bytes_));
            }
            mounted_=false;
        }
        bytes_.clear();
    }
};
}
std::unique_ptr<Adapter> rcc_adapter(QString root,RccApproval approval){return std::make_unique<RccAdapter>(std::move(root),std::move(approval));}
size_t retained_rcc_buffers(){std::lock_guard lock(mutex());return retained().size();}
}
