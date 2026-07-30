#pragma once

// Linux compatibility layer for the Apple CommonCrypto SHA-256 API used by
// ResponseRelated/Integrity/Hash.cpp.  The Son source file remains unchanged.

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

using CC_LONG = unsigned long;

constexpr int CC_SHA256_DIGEST_LENGTH = 32;

struct CC_SHA256_CTX {
    std::array<std::uint32_t, 8> state{};
    std::array<std::uint8_t, 64> buffer{};
    std::uint64_t bitCount = 0;
    std::size_t bufferSize = 0;
};

inline constexpr std::array<std::uint32_t, 64> kRoundConstants = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

inline std::uint32_t sha256RotateRight(std::uint32_t value, unsigned int bits) {
    return (value >> bits) | (value << (32U - bits));
}

inline void sha256Transform(CC_SHA256_CTX& context, const std::uint8_t* block) {
    std::array<std::uint32_t, 64> words{};
    for (std::size_t index = 0; index < 16; ++index) {
        const std::size_t offset = index * 4;
        words[index] = (static_cast<std::uint32_t>(block[offset]) << 24U)
            | (static_cast<std::uint32_t>(block[offset + 1]) << 16U)
            | (static_cast<std::uint32_t>(block[offset + 2]) << 8U)
            | static_cast<std::uint32_t>(block[offset + 3]);
    }

    for (std::size_t index = 16; index < words.size(); ++index) {
        const std::uint32_t smallSigma0 = sha256RotateRight(words[index - 15], 7) ^ sha256RotateRight(words[index - 15], 18) ^ (words[index - 15] >> 3U);
        const std::uint32_t smallSigma1 = sha256RotateRight(words[index - 2], 17) ^ sha256RotateRight(words[index - 2], 19) ^ (words[index - 2] >> 10U);
        words[index] = words[index - 16] + smallSigma0 + words[index - 7] + smallSigma1;
    }

    std::uint32_t a = context.state[0];
    std::uint32_t b = context.state[1];
    std::uint32_t c = context.state[2];
    std::uint32_t d = context.state[3];
    std::uint32_t e = context.state[4];
    std::uint32_t f = context.state[5];
    std::uint32_t g = context.state[6];
    std::uint32_t h = context.state[7];

    for (std::size_t index = 0; index < words.size(); ++index) {
        const std::uint32_t bigSigma1 = sha256RotateRight(e, 6) ^ sha256RotateRight(e, 11) ^ sha256RotateRight(e, 25);
        const std::uint32_t choose = (e & f) ^ ((~e) & g);
        const std::uint32_t temporary1 = h + bigSigma1 + choose + kRoundConstants[index] + words[index];
        const std::uint32_t bigSigma0 = sha256RotateRight(a, 2) ^ sha256RotateRight(a, 13) ^ sha256RotateRight(a, 22);
        const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temporary2 = bigSigma0 + majority;

        h = g;
        g = f;
        f = e;
        e = d + temporary1;
        d = c;
        c = b;
        b = a;
        a = temporary1 + temporary2;
    }

    context.state[0] += a;
    context.state[1] += b;
    context.state[2] += c;
    context.state[3] += d;
    context.state[4] += e;
    context.state[5] += f;
    context.state[6] += g;
    context.state[7] += h;
}

inline int CC_SHA256_Init(CC_SHA256_CTX* context) {
    if (context == nullptr) {
        return 0;
    }

    context->state = {0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
                      0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
    context->bitCount = 0;
    context->bufferSize = 0;
    return 1;
}

inline int CC_SHA256_Update(CC_SHA256_CTX* context, const void* data, CC_LONG length) {
    if (context == nullptr || (data == nullptr && length != 0)) {
        return 0;
    }

    const auto* bytes = static_cast<const std::uint8_t*>(data);
    context->bitCount += static_cast<std::uint64_t>(length) * 8U;

    for (CC_LONG index = 0; index < length; ++index) {
        context->buffer[context->bufferSize++] = bytes[index];
        if (context->bufferSize == context->buffer.size()) {
            sha256Transform(*context, context->buffer.data());
            context->bufferSize = 0;
        }
    }

    return 1;
}

inline int CC_SHA256_Final(unsigned char* digest, CC_SHA256_CTX* context) {
    if (digest == nullptr || context == nullptr) {
        return 0;
    }

    context->buffer[context->bufferSize++] = 0x80U;
    if (context->bufferSize > 56) {
        while (context->bufferSize < context->buffer.size()) {
            context->buffer[context->bufferSize++] = 0;
        }
        sha256Transform(*context, context->buffer.data());
        context->bufferSize = 0;
    }

    while (context->bufferSize < 56) {
        context->buffer[context->bufferSize++] = 0;
    }

    const std::uint64_t bitCount = context->bitCount;
    for (int shift = 56; shift >= 0; shift -= 8) {
        context->buffer[context->bufferSize++] = static_cast<std::uint8_t>(bitCount >> shift);
    }
    sha256Transform(*context, context->buffer.data());

    for (std::size_t index = 0; index < context->state.size(); ++index) {
        const std::uint32_t word = context->state[index];
        digest[index * 4] = static_cast<unsigned char>(word >> 24U);
        digest[index * 4 + 1] = static_cast<unsigned char>(word >> 16U);
        digest[index * 4 + 2] = static_cast<unsigned char>(word >> 8U);
        digest[index * 4 + 3] = static_cast<unsigned char>(word);
    }

    return 1;
}
