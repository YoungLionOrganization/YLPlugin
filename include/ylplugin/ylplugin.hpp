#pragma once
#include <ylspec.h>
#include <nlohmann/json.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace ylp {
using Bytes = std::vector<uint8_t>;
using Json = nlohmann::json;
enum class Code { ok, invalid_container, unsupported_profile, invalid_metadata,
    resource_limit, io, integrity, invalid_signature, needs_license,
    unsupported, cancelled, permission, runtime, memory };
class Error : public std::runtime_error {
public:
    Code code;
    Error(Code c, const std::string& message) : std::runtime_error(message), code(c) {}
};
struct Limits {
    uint64_t metadata = 16 * 1024 * 1024, toc = 8 * 1024 * 1024;
    uint64_t records = 16384, record = 64 * 1024 * 1024, total = 1024ull * 1024 * 1024;
    uint64_t ratio = 256;
};
using Cancel = std::function<bool()>;
using KeyProvider = std::function<bool(const std::string&, std::array<uint8_t,32>&)>;
// Return false for unknown, revoked or disallowed signing keys.
using TrustProvider = std::function<bool(const std::string&, std::array<uint8_t,32>&)>;
struct Resource {
    std::string path, media, variant, key_id;
    uint64_t offset = 0, stored = 0, size = 0;
    uint32_t alignment = 8;
    uint8_t compression = 0, encryption = 0, extraction = 0;
    std::array<uint8_t,32> hash{};
    std::array<uint8_t,12> nonce{};
    std::array<uint8_t,16> tag{};
};
struct Input {
    std::string path, media = "application/octet-stream", variant, key_id;
    std::filesystem::path file;
    uint8_t compression = 0, extraction = 0;
    uint32_t alignment = 8;
};
struct BuildOptions { Limits limits; KeyProvider keys; Cancel cancel; };
struct Permission {
    enum class Format { strings, bitmask, bytes } format = Format::strings;
    std::string registry, registry_version;
    std::vector<std::string> strings;
    uint64_t mask = 0;
    Bytes bytes;
};
struct Entrypoint {
    std::string id, runtime, runtime_min, runtime_max, resource, os, arch, abi;
};
struct Target {
    std::string id, host, api_min, api_max, entrypoint;
    int64_t priority = 0;
    bool fallback = true;
    std::vector<std::string> required_capabilities, optional_capabilities;
    Permission required_permissions, optional_permissions;
    uint64_t memory = 0, storage = 0;
};
struct Metadata {
    std::string id, version, publisher;
    std::vector<Target> targets;
    std::vector<Entrypoint> entrypoints;
};
Metadata validate_profile(const yl_document*);
yl_status profile_validator(const yl_document*, void*, yl_error*) noexcept;
bool safe_path(const std::string&);
Json permission_json(const Permission&);
std::string hex(std::span<const uint8_t>);
Bytes unhex(const std::string&);

class Package {
public:
    struct Impl;
    explicit Package(const std::filesystem::path&, Limits = {});
    // Reader must fill exactly the requested region; callback may run concurrently.
    Package(uint64_t size, std::function<void(uint64_t,std::span<uint8_t>)>, Limits = {});
    const std::vector<Resource>& resources() const;
    const Metadata& metadata() const; // Throws on unknown/invalid profile.
    const yl_document* document() const;
    std::string metadata_text() const;
    Json inspect() const;
    Bytes read(const std::string&, const KeyProvider& = {}, const Cancel& = {}) const;
    // Store + plaintext only. Copies a bounded slice after validating the entire resource hash.
    Bytes read_slice(const std::string&, uint64_t offset, size_t count, const Cancel& = {}) const;
    void validate(const KeyProvider& = {}, const Cancel& = {}) const;
    bool signed_package() const;
    std::string signing_key_id() const;
    void verify(const TrustProvider&, const Cancel& = {}) const;
    void extract(const std::filesystem::path& destination, const std::vector<std::string>& paths = {},
                 const KeyProvider& = {}, const Cancel& = {}) const;
    const Resource& resource(const std::string&) const;
private:
    std::shared_ptr<Impl> impl_;
    friend void sign(const std::filesystem::path&, const std::filesystem::path&, const std::string&, std::span<const uint8_t>, const Cancel&);
};
void create(const std::filesystem::path& output, const std::string& manifest,
            std::vector<Input> inputs, const BuildOptions& = {});
// PEM private key is a caller-owned secret. No key is stored in the package.
void sign(const std::filesystem::path& input, const std::filesystem::path& output,
          const std::string& key_id, std::span<const uint8_t> private_pem, const Cancel& = {});

struct Host {
    std::string id, api, os, arch, abi, target;
    std::vector<std::string> capabilities;
    Json runtimes = Json::object(); // runtime ID -> version string
    Json permission_registries = Json::object(); // registry -> exact version
    std::vector<std::string> permission_formats;
    // Explicit grants keyed by target ID, with the same permission representation.
    Json grants = Json::object();
    bool allow_generic = false, allow_extraction = false, allow_native = false;
    bool require_signature = false;
    uint64_t memory = 0, storage = 0; // zero = unknown, not unlimited
    KeyProvider keys;
    TrustProvider trust;
    Cancel cancel;
};
Host host_from_json(const Json&);
Json preflight(const Package&, const Host&);

enum class Stage { discovered, compatible, validated, prepared, active, started, stopped, cleaned, failed };
class Adapter {
public:
    virtual ~Adapter() = default;
    virtual std::string id() const = 0;
    virtual std::string version() const = 0;
    virtual void validate(const Package&, const Entrypoint&) = 0;
    virtual void prepare(const Package&, const Entrypoint&, const Host&, const Cancel&) = 0;
    virtual void activate() = 0;
    virtual void start() = 0;
    virtual void stop() noexcept = 0;
    virtual void cleanup() noexcept = 0;
};
class Session {
public:
    Session(Package, Host, std::unique_ptr<Adapter>);
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    void prepare(const Cancel& = {});
    void activate();
    void start();
    void stop() noexcept;
    void cleanup() noexcept;
    Stage stage() const { return stage_; }
    Json result() const { return result_; }
private:
    Package package_; Host host_; std::unique_ptr<Adapter> adapter_;
    Stage stage_ = Stage::discovered; Json result_;
};
std::unique_ptr<Adapter> data_adapter();
// Native C ABI adapter: host supplies its own private cache and a digest-bound approval.
using NativeApproval = std::function<bool(const std::string& plugin_id, const std::string& resource,
                                         const std::array<uint8_t,32>& hash)>;
std::unique_ptr<Adapter> native_adapter(std::filesystem::path new_private_directory, NativeApproval);
} // namespace ylp
