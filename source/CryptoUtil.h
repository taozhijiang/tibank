#ifndef _TiBANK_CRYPTO_UTIL_HPP_
#define _TiBANK_CRYPTO_UTIL_HPP_

#include <cmath>
#include <iomanip>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include "General.h"

// 类静态函数可以直接将函数定义丢在头文件中

struct CryptoUtil {


static std::string base64_encode(const std::string &ascii) {

    std::string base64;

    BIO *bio, *b64;
    BUF_MEM *bptr = BUF_MEM_new();

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new(BIO_s_mem());
    BIO_push(b64, bio);
    BIO_set_mem_buf(b64, bptr, BIO_CLOSE);

    // Write directly to base64-buffer to avoid copy
    auto base64_length = static_cast<std::size_t>(round(4 * ceil(static_cast<double>(ascii.size()) / 3.0)));
    base64.resize(base64_length);
    bptr->length = 0;
    bptr->max = base64_length + 1;
    bptr->data = &base64[0];

    if(BIO_write(b64, &ascii[0], static_cast<int>(ascii.size())) <= 0 || BIO_flush(b64) <= 0)
        base64.clear();

    // To keep &base64[0] through BIO_free_all(b64)
    bptr->length = 0;
    bptr->max = 0;
    bptr->data = (char*)NULL;

    BIO_free_all(b64);

    return base64;
}

static std::string base64_decode(const std::string &base64) noexcept {

    std::string ascii;

    // Resize ascii, however, the size is a up to two bytes too large.
    ascii.resize((6 * base64.size()) / 8);
    BIO *b64, *bio;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    // TODO: Remove in 2020
#if OPENSSL_VERSION_NUMBER <= 0x1000115fL
    bio = BIO_new_mem_buf((char *)&base64[0], static_cast<int>(base64.size()));
#else
    bio = BIO_new_mem_buf(&base64[0], static_cast<int>(base64.size()));
#endif
    bio = BIO_push(b64, bio);

    auto decoded_length = BIO_read(bio, &ascii[0], static_cast<int>(ascii.size()));
    if(decoded_length > 0)
        ascii.resize(static_cast<std::size_t>(decoded_length));
    else
        ascii.clear();

    BIO_free_all(b64);

    return ascii;
}

/// Return hex string from bytes in input string.
static std::string to_hex_string(const std::string &input) noexcept {
    std::stringstream hex_stream;
    hex_stream << std::hex << std::internal << std::setfill('0');
    for(size_t i=0; i<input.size(); ++i)
        hex_stream << std::setw(2) << static_cast<int>(static_cast<unsigned char>(input[i]));

    return hex_stream.str();
}

static std::string md5(const std::string &input, std::size_t iterations = 1) noexcept {
    std::string hash;

    hash.resize(128 / 8);
    ::MD5(reinterpret_cast<const unsigned char *>(&input[0]), input.size(), reinterpret_cast<unsigned char *>(&hash[0]));

    for(std::size_t c = 1; c < iterations; ++c)
        ::MD5(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

    return hash;
}


static std::string sha1(const std::string &input, std::size_t iterations = 1) noexcept {
    std::string hash;

    hash.resize(160 / 8);
    ::SHA1(reinterpret_cast<const unsigned char *>(&input[0]), input.size(), reinterpret_cast<unsigned char *>(&hash[0]));

    for(std::size_t c = 1; c < iterations; ++c)
        ::SHA1(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

    return hash;
}


static std::string sha256(const std::string &input, std::size_t iterations = 1) noexcept {
    std::string hash;

    hash.resize(256 / 8);
    ::SHA256(reinterpret_cast<const unsigned char *>(&input[0]), input.size(), reinterpret_cast<unsigned char *>(&hash[0]));

    for(std::size_t c = 1; c < iterations; ++c)
        ::SHA256(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

    return hash;
}


static std::string sha512(const std::string &input, std::size_t iterations = 1) noexcept {
    std::string hash;

    hash.resize(512 / 8);
    ::SHA512(reinterpret_cast<const unsigned char *>(&input[0]), input.size(), reinterpret_cast<unsigned char *>(&hash[0]));

    for(std::size_t c = 1; c < iterations; ++c)
        ::SHA512(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

    return hash;
}
};

#endif /* _TiBANK_CRYPTO_UTIL_HPP_ */
