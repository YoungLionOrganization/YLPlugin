#include "../internal.hpp"
#include <set>
#if defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <cerrno>
#if defined(__linux__)
#include <sys/syscall.h>
#include <linux/fs.h>
#endif
#include <openssl/rand.h>
#endif

namespace ylp {
#if defined(__linux__) || defined(__APPLE__)
namespace {
struct FD {
    int value=-1;
    explicit FD(int fd=-1):value(fd) {}
    ~FD(){ if(value>=0) ::close(value); }
    FD(const FD&)=delete; FD& operator=(const FD&)=delete;
    FD(FD&& other) noexcept:value(other.value){other.value=-1;}
    FD& operator=(FD&& other) noexcept { if(value>=0) ::close(value); value=other.value; other.value=-1; return *this; }
};
FD open_parent(const std::filesystem::path& p) {
    FD current(::open(p.is_absolute()?"/":".",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));
    require(current.value>=0,Code::io,"cannot open extraction root");
    for(const auto& part:p) {
        auto name=part.string(); if(name=="/"||name=="."||name.empty()) continue;
        require(name!="..",Code::permission,"parent traversal in extraction destination");
        FD next(::openat(current.value,name.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));
        require(next.value>=0,Code::permission,"extraction ancestor is missing, inaccessible or a symlink"); current=std::move(next);
    }
    return current;
}
}
#endif
void Package::extract(const std::filesystem::path& destination,const std::vector<std::string>& selected,const KeyProvider& keys,const Cancel& cancel) const {
    metadata(); cancelled(cancel);
    std::vector<std::string> names=selected;
    if(names.empty()) for(const auto& r:resources()) names.push_back(r.path);
    std::set<std::string> unique;
    for(const auto& name:names) {
        require(unique.insert(name).second,Code::invalid_metadata,"duplicate extraction selection");
        require(resource(name).extraction!=1,Code::permission,"direct-required resource cannot be extracted: "+name);
    }
#if defined(__linux__) || defined(__APPLE__)
    auto final=destination.filename().string();
    require(safe_path(final)&&final.find('/')==std::string::npos,Code::permission,"safe new destination directory required");
    auto parent=open_parent(destination.parent_path());
    struct stat parent_info{};
    require(::fstat(parent.value,&parent_info)==0&&parent_info.st_uid==::geteuid()&&
            (parent_info.st_mode&(S_IWGRP|S_IWOTH))==0,Code::permission,
            "extraction parent must be owned by the current user and not writable by group/others");
    struct stat info{};
    require(::fstatat(parent.value,final.c_str(),&info,AT_SYMLINK_NOFOLLOW)!=0&&errno==ENOENT,Code::permission,"extraction destination already exists or cannot be checked");
    std::array<uint8_t,16> random{};
    require(RAND_bytes(random.data(),static_cast<int>(random.size()))==1,Code::io,"extraction RNG failure");
    std::string stage=".ylplugin-extract-"+hex(random);
    require(::mkdirat(parent.value,stage.c_str(),0700)==0,Code::io,"cannot create private extraction staging directory");
    bool published=false;
    // Staging is private. Cleanup uses the anchored descriptor, never a re-resolved parent path.
    auto cleanup=[&] {
        if(published) return;
        for(auto it=names.rbegin();it!=names.rend();++it) {
            std::string relative=stage+"/"+*it; ::unlinkat(parent.value,relative.c_str(),0);
            for(size_t pos=relative.rfind('/');pos!=std::string::npos&&pos>stage.size();pos=relative.rfind('/',pos-1))
                ::unlinkat(parent.value,relative.substr(0,pos).c_str(),AT_REMOVEDIR);
        }
        ::unlinkat(parent.value,stage.c_str(),AT_REMOVEDIR);
    };
    try {
        FD root(::openat(parent.value,stage.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
        require(root.value>=0,Code::io,"cannot open staging directory");
        for(const auto& name:names) {
            cancelled(cancel); auto bytes=read(name,keys,cancel); FD dir(::dup(root.value));
            require(dir.value>=0,Code::io,"cannot duplicate directory handle");
            size_t start=0;
            for(size_t pos=name.find('/');pos!=std::string::npos;pos=name.find('/',start)) {
                auto component=name.substr(start,pos-start);
                if(::mkdirat(dir.value,component.c_str(),0700)!=0) require(errno==EEXIST,Code::io,"cannot create extraction subdirectory");
                FD next(::openat(dir.value,component.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
                require(next.value>=0,Code::permission,"unsafe extraction directory"); dir=std::move(next); start=pos+1;
            }
            auto leaf=name.substr(start); FD file(::openat(dir.value,leaf.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));
            require(file.value>=0,Code::permission,"cannot exclusively create extraction file");
            for(size_t off=0;off<bytes.size();) {
                cancelled(cancel); auto n=::write(file.value,bytes.data()+off,std::min<size_t>(1024*1024,bytes.size()-off));
                if(n<0&&errno==EINTR) continue;
                require(n>0,Code::io,"extraction write failed"); off+=static_cast<size_t>(n);
            }
            require(::fsync(file.value)==0,Code::io,"extraction fsync failed");
        }
        cancelled(cancel);
#if defined(__linux__) && defined(SYS_renameat2)
        require(::syscall(SYS_renameat2,parent.value,stage.c_str(),parent.value,final.c_str(),RENAME_NOREPLACE)==0,Code::io,"atomic non-replacing extraction publish failed");
#elif defined(__APPLE__)
        require(::renameatx_np(parent.value,stage.c_str(),parent.value,final.c_str(),RENAME_EXCL)==0,Code::io,"atomic non-replacing extraction publish failed");
#else
        fail(Code::unsupported,"platform lacks atomic non-replacing directory publication");
#endif
        published=true;
    } catch(...) { cleanup(); throw; }
#else
    (void)destination; (void)keys;
    fail(Code::unsupported,"secure extraction is currently implemented for Linux/macOS only");
#endif
}
} // namespace ylp
