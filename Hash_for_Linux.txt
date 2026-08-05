#include "Hash.h"
#include <openssl/sha.h>
#include <fstream>
#include <iomanip>
#include <sstream>

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
