#pragma once
#include <ylplugin/ylplugin.hpp>
#include <fstream>
#include <mutex>
#include <openssl/evp.h>

namespace ylp {
[[noreturn]] inline void fail(Code c, const std::string& m) { throw Error(c,m); }
inline void require(bool b, Code c, const std::string& m) { if(!b) fail(c,m); }
inline void cancelled(const Cancel& c) { if(c && c()) fail(Code::cancelled,"operation cancelled"); }
uint64_t get(std::span<const uint8_t>, size_t, size_t);
void put(Bytes&, size_t, uint64_t, size_t);
uint64_t align(uint64_t n, uint64_t a);
void zero(std::span<const uint8_t>);
Bytes encode_record(const Resource&);
Bytes sha256(std::span<const uint8_t>);
Bytes crypt(bool encrypt, std::span<const uint8_t> input, std::span<const uint8_t> aad,
            const std::array<uint8_t,32>& key, Resource&);
Bytes sign_digest(std::span<const uint8_t> pem, std::span<const uint8_t> message);
bool verify_digest(const std::array<uint8_t,32>&, std::span<const uint8_t>, std::span<const uint8_t>);
Bytes file_bytes(const std::filesystem::path&, uint64_t maximum);
struct DocumentDeleter { void operator()(yl_document* d) const { yl_document_free(d); } };
using Document = std::unique_ptr<yl_document,DocumentDeleter>;
struct YlBuffer {
    yl_buffer value{};
    ~YlBuffer() { yl_buffer_free(&value); }
};
Document parse_manifest(const std::string&, uint64_t limit);
struct Package::Impl {
    uint64_t size = 0, flags = 0, sig_offset = 0;
    Limits limits;
    Bytes header, signature;
    Document document;
    std::vector<Resource> resources;
    mutable std::once_flag metadata_once;
    mutable std::unique_ptr<Metadata> metadata;
    std::function<void(uint64_t,std::span<uint8_t>)> reader;
    void read_at(uint64_t offset, std::span<uint8_t> out) const;
    Bytes bytes(uint64_t offset, uint64_t count) const;
    void parse();
    Bytes digest(const Cancel&) const;
};
class Output {
public:
    std::filesystem::path destination, temporary;
    std::fstream stream;
    explicit Output(const std::filesystem::path&);
    ~Output();
    void write(std::span<const uint8_t>);
    uint64_t position();
    void pad(uint64_t boundary);
    void publish();
};
Json resource_json(const Resource&);
} // namespace ylp
