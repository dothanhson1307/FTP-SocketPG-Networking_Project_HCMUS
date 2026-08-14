#include "Hash.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <array>

#if __has_include(<openssl/sha.h>)
#include <openssl/sha.h>

string calculateFileSHA256(const string& filepath) {
    std::ifstream fin(filepath, std::ios::binary);
    if (!fin.is_open()) {
        return "";
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    char buffer[4096];
    while (fin.read(buffer, sizeof(buffer)) || fin.gcount() > 0) {
        SHA256_Update(&sha256, buffer, static_cast<uint32_t>(fin.gcount()));
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}

#else

namespace {

inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32_t sig0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline uint32_t sig1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline uint32_t gam0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

inline uint32_t gam1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

struct SHA256Context {
    uint32_t state[8]{
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    uint64_t bitlen = 0;
    uint8_t data[64]{};
    size_t datalen = 0;

    void transform() {
        uint32_t m[64];
        for (int i = 0, j = 0; i < 16; ++i, j += 4) {
            m[i] = (static_cast<uint32_t>(data[j]) << 24) |
                   (static_cast<uint32_t>(data[j + 1]) << 16) |
                   (static_cast<uint32_t>(data[j + 2]) << 8) |
                   (static_cast<uint32_t>(data[j + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            m[i] = gam1(m[i - 2]) + m[i - 7] + gam0(m[i - 15]) + m[i - 16];
        }

        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = h + sig1(e) + ch(e, f, g) + K[i] + m[i];
            uint32_t t2 = sig0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

    void update(const uint8_t* input, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            data[datalen++] = input[i];
            if (datalen == 64) {
                transform();
                bitlen += 512;
                datalen = 0;
            }
        }
    }

    void final(uint8_t hash[32]) {
        size_t i = datalen;
        if (datalen < 56) {
            data[i++] = 0x80;
            while (i < 56) data[i++] = 0x00;
        } else {
            data[i++] = 0x80;
            while (i < 64) data[i++] = 0x00;
            transform();
            std::memset(data, 0, 56);
        }

        bitlen += datalen * 8;
        for (int j = 7; j >= 0; --j) {
            data[56 + (7 - j)] = static_cast<uint8_t>((bitlen >> (j * 8)) & 0xFF);
        }
        transform();

        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 8; ++k) {
                hash[k * 4 + j] = static_cast<uint8_t>((state[k] >> (24 - j * 8)) & 0xFF);
            }
        }
    }
};

} // anonymous namespace

string calculateFileSHA256(const string& filepath) {
    std::ifstream fin(filepath, std::ios::binary);
    if (!fin.is_open()) {
        return "";
    }

    SHA256Context ctx;
    char buffer[4096];
    while (fin.read(buffer, sizeof(buffer)) || fin.gcount() > 0) {
        ctx.update(reinterpret_cast<const uint8_t*>(buffer), static_cast<size_t>(fin.gcount()));
    }

    uint8_t hash[32];
    ctx.final(hash);

    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

#endif
