#include "../internal.hpp"
#include <openssl/pem.h>
#include <openssl/crypto.h>
#include <climits>

namespace ylp {
std::string hex(std::span<const uint8_t> bytes) {
    static constexpr char chars[]="0123456789abcdef";
    std::string out; out.reserve(bytes.size()*2);
    for(auto b:bytes) { out+=chars[b>>4]; out+=chars[b&15]; }
    return out;
}
Bytes unhex(const std::string& s) {
    require(s.size()%2==0,Code::invalid_metadata,"hex must have even length");
    auto nibble=[](char c)->uint8_t {
        if(c>='0'&&c<='9') return static_cast<uint8_t>(c-'0');
        if(c>='a'&&c<='f') return static_cast<uint8_t>(c-'a'+10);
        if(c>='A'&&c<='F') return static_cast<uint8_t>(c-'A'+10);
        fail(Code::invalid_metadata,"invalid hex digit");
    };
    Bytes out; out.reserve(s.size()/2);
    for(size_t i=0;i<s.size();i+=2) out.push_back(static_cast<uint8_t>((nibble(s[i])<<4)|nibble(s[i+1])));
    return out;
}
Bytes sha256(std::span<const uint8_t> input) {
    Bytes out(32); unsigned n=0;
    require(EVP_Digest(input.data(),input.size(),out.data(),&n,EVP_sha256(),nullptr)==1&&n==32,
            Code::integrity,"SHA256 failure"); return out;
}
Bytes crypt(bool encrypt, std::span<const uint8_t> input, std::span<const uint8_t> aad,
            const std::array<uint8_t,32>& key, Resource& r) {
    require(input.size()<=INT_MAX && aad.size()<=INT_MAX,Code::resource_limit,"AEAD input limit");
    std::unique_ptr<EVP_CIPHER_CTX,decltype(&EVP_CIPHER_CTX_free)> ctx(EVP_CIPHER_CTX_new(),EVP_CIPHER_CTX_free);
    require(bool(ctx),Code::memory,"AEAD context allocation");
    auto ok=[&](int result) { require(result==1,Code::integrity,"AES-GCM operation failed"); };
    ok(EVP_CipherInit_ex(ctx.get(),EVP_aes_256_gcm(),nullptr,nullptr,nullptr,encrypt?1:0));
    ok(EVP_CIPHER_CTX_ctrl(ctx.get(),EVP_CTRL_GCM_SET_IVLEN,12,nullptr));
    ok(EVP_CipherInit_ex(ctx.get(),nullptr,nullptr,key.data(),r.nonce.data(),encrypt?1:0));
    int n=0, tail=0;
    ok(EVP_CipherUpdate(ctx.get(),nullptr,&n,aad.data(),static_cast<int>(aad.size())));
    Bytes out(input.size()+16);
    ok(EVP_CipherUpdate(ctx.get(),out.data(),&n,input.data(),static_cast<int>(input.size())));
    if(!encrypt) ok(EVP_CIPHER_CTX_ctrl(ctx.get(),EVP_CTRL_GCM_SET_TAG,16,r.tag.data()));
    if(EVP_CipherFinal_ex(ctx.get(),out.data()+n,&tail)!=1) {
        OPENSSL_cleanse(out.data(),out.size()); fail(Code::integrity,"AES-GCM authentication failed");
    }
    if(encrypt) ok(EVP_CIPHER_CTX_ctrl(ctx.get(),EVP_CTRL_GCM_GET_TAG,16,r.tag.data()));
    out.resize(static_cast<size_t>(n+tail)); return out;
}
Bytes sign_digest(std::span<const uint8_t> pem, std::span<const uint8_t> message) {
    require(pem.size()<=INT_MAX,Code::resource_limit,"private key input limit");
    std::unique_ptr<BIO,decltype(&BIO_free)> bio(BIO_new_mem_buf(pem.data(),static_cast<int>(pem.size())),BIO_free);
    require(bool(bio),Code::memory,"key BIO allocation");
    // An explicit password callback prevents OpenSSL from prompting on stdin.
    auto password=[](char*,int,int,void*)->int { return 0; };
    std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)> key(PEM_read_bio_PrivateKey(bio.get(),nullptr,password,nullptr),EVP_PKEY_free);
    require(key && EVP_PKEY_is_a(key.get(),"ED25519"),Code::invalid_signature,"unencrypted Ed25519 PEM key required");
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    require(ctx && EVP_DigestSignInit(ctx.get(),nullptr,nullptr,nullptr,key.get())==1,Code::invalid_signature,"Ed25519 init failed");
    Bytes sig(64); size_t n=sig.size();
    require(EVP_DigestSign(ctx.get(),sig.data(),&n,message.data(),message.size())==1&&n==64,Code::invalid_signature,"Ed25519 sign failed");
    return sig;
}
bool verify_digest(const std::array<uint8_t,32>& pub, std::span<const uint8_t> message, std::span<const uint8_t> sig) {
    std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)> key(EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,nullptr,pub.data(),pub.size()),EVP_PKEY_free);
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    return key && ctx && EVP_DigestVerifyInit(ctx.get(),nullptr,nullptr,nullptr,key.get())==1 &&
        EVP_DigestVerify(ctx.get(),sig.data(),sig.size(),message.data(),message.size())==1;
}
} // namespace ylp
