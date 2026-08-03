#include "Hash.h"
#include <CommonCrypto/CommonDigest.h>
#include <fstream>
#include <iomanip>
#include <sstream>

string calculateFileSHA256(const string& filepath) {
    std::ifstream fin(filepath, std::ios::binary);
    if (!fin.is_open()) {
        return "";
    }

    CC_SHA256_CTX sha256;
    CC_SHA256_Init(&sha256);

    char buffer[4096];
    while (fin.read(buffer, sizeof(buffer)) || fin.gcount() > 0) {
        CC_SHA256_Update(&sha256, buffer, static_cast<CC_LONG>(fin.gcount()));
    }

    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}