// password_utils.cpp - SHA-256, HMAC-SHA256 and PBKDF2 (standard algorithms)
#include "password_utils.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#if defined(_WIN32)
  #include <windows.h>
  #include <bcrypt.h>     // BCryptGenRandom (Windows CNG); link with -lbcrypt
#else
  #include <sys/random.h> // non-Windows builds only (used to test this code off Windows)
#endif

namespace {

using Bytes = std::vector<std::uint8_t>;

const std::uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

std::uint32_t rotr(std::uint32_t x, unsigned int n) { return (x >> n) | (x << (32 - n)); }

Bytes toBytes(const std::string& text) { return Bytes(text.begin(), text.end()); }
std::string toText(const Bytes& bytes) { return std::string(bytes.begin(), bytes.end()); }

Bytes sha256(const Bytes& data) {
    std::uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    // Padding: 0x80, zeros, then the message length in bits (64-bit, big-endian)
    Bytes msg(data);
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) msg.push_back(0x00);
    std::uint64_t bitLength = static_cast<std::uint64_t>(data.size()) * 8;
    for (int i = 7; i >= 0; --i) msg.push_back(static_cast<std::uint8_t>(bitLength >> (i * 8)));

    for (std::size_t offset = 0; offset < msg.size(); offset += 64) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(msg[offset + 4 * i]) << 24) |
                   (static_cast<std::uint32_t>(msg[offset + 4 * i + 1]) << 16) |
                   (static_cast<std::uint32_t>(msg[offset + 4 * i + 2]) << 8) |
                   static_cast<std::uint32_t>(msg[offset + 4 * i + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        std::uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            std::uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            std::uint32_t ch = (e & f) ^ (~e & g);
            std::uint32_t t1 = hh + S1 + ch + K[i] + w[i];
            std::uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            std::uint32_t t2 = S0 + maj;
            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    Bytes digest;
    for (int i = 0; i < 8; ++i) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            digest.push_back(static_cast<std::uint8_t>(h[i] >> shift));
        }
    }
    return digest;
}

Bytes hmacSha256(const Bytes& key, const Bytes& message) {
    Bytes k(key);
    if (k.size() > 64) k = sha256(k);
    k.resize(64, 0x00);

    Bytes inner(64), outer(64);
    for (std::size_t i = 0; i < 64; ++i) {
        inner[i] = static_cast<std::uint8_t>(k[i] ^ 0x36);
        outer[i] = static_cast<std::uint8_t>(k[i] ^ 0x5c);
    }
    inner.insert(inner.end(), message.begin(), message.end());
    Bytes innerHash = sha256(inner);
    outer.insert(outer.end(), innerHash.begin(), innerHash.end());
    return sha256(outer);
}

Bytes pbkdf2(const Bytes& password, const Bytes& salt, unsigned int iterations, std::size_t keyLength) {
    Bytes derived;
    for (std::uint32_t block = 1; derived.size() < keyLength; ++block) {
        Bytes input(salt);
        for (int shift = 24; shift >= 0; shift -= 8) {
            input.push_back(static_cast<std::uint8_t>(block >> shift));
        }
        Bytes u = hmacSha256(password, input);
        Bytes t = u;
        for (unsigned int i = 1; i < iterations; ++i) {
            u = hmacSha256(password, u);
            for (std::size_t j = 0; j < t.size(); ++j) t[j] ^= u[j];
        }
        derived.insert(derived.end(), t.begin(), t.end());
    }
    derived.resize(keyLength);
    return derived;
}

std::string toHex(const Bytes& bytes) {
    const char* digits = "0123456789abcdef";
    std::string out;
    for (std::uint8_t b : bytes) {
        out.push_back(digits[b >> 4]);
        out.push_back(digits[b & 0x0f]);
    }
    return out;
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool fromHex(const std::string& text, std::string& out) {
    if (text.empty() || text.size() % 2 != 0) return false;
    out.clear();
    for (std::size_t i = 0; i < text.size(); i += 2) {
        int hi = hexValue(text[i]), lo = hexValue(text[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<char>(hi * 16 + lo));
    }
    return true;
}

// Compares two strings without stopping at the first difference.
bool sameText(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return diff == 0;
}

// Fills the buffer with cryptographically secure random bytes from the operating system.
// Windows: BCryptGenRandom (CNG) using the system-preferred RNG.
// Other systems (only for testing this code off Windows): getrandom().
// Returns false if the system could not supply the bytes; the caller must then NOT
// create a record (there is deliberately no fallback to a weaker generator).
bool secureRandomBytes(std::uint8_t* buffer, std::size_t length) {
#if defined(_WIN32)
    NTSTATUS status = BCryptGenRandom(nullptr, buffer, static_cast<ULONG>(length),
                                      BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return status >= 0;   // NT_SUCCESS
#else
    std::size_t done = 0;
    while (done < length) {
        ssize_t got = getrandom(buffer + done, length - done, 0);
        if (got <= 0) return false;
        done += static_cast<std::size_t>(got);
    }
    return true;
#endif
}

std::string makeSalt(std::size_t length) {
    std::string salt(length, '\0');
    if (!secureRandomBytes(reinterpret_cast<std::uint8_t*>(&salt[0]), length)) {
        throw std::runtime_error("Secure random number generator is not available.");
    }
    return salt;
}

// Splits "a$b$c$d" into its parts.
std::vector<std::string> splitRecord(const std::string& record) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : record) {
        if (c == '$') { parts.push_back(current); current.clear(); }
        else current.push_back(c);
    }
    parts.push_back(current);
    return parts;
}

const char* const SCHEME = "pbkdf2_sha256";
const std::size_t SALT_LENGTH = 16;   // bytes -> 32 hex characters in the record
const std::size_t HASH_LENGTH = 32;   // bytes -> 64 hex characters in the record

} // namespace

namespace PasswordUtils {

std::string sha256Hex(const std::string& data) {
    return toHex(sha256(toBytes(data)));
}

std::string pbkdf2HmacSha256Hex(const std::string& password, const std::string& salt,
                                unsigned int iterations, std::size_t keyLength) {
    return toHex(pbkdf2(toBytes(password), toBytes(salt), iterations, keyLength));
}

std::string hashPassword(const std::string& password) {
    std::string salt = makeSalt(SALT_LENGTH);
    std::string hashHex = pbkdf2HmacSha256Hex(password, salt, DEFAULT_ITERATIONS, HASH_LENGTH);
    return std::string(SCHEME) + "$" + std::to_string(DEFAULT_ITERATIONS) + "$" +
           toHex(toBytes(salt)) + "$" + hashHex;
}

bool verifyPassword(const std::string& password, const std::string& storedRecord) {
    std::vector<std::string> parts = splitRecord(storedRecord);
    if (parts.size() != 4 || parts[0] != SCHEME) return false;

    // Iteration count: digits only, between 1 and 10 million
    const std::string& iterText = parts[1];
    if (iterText.empty() || iterText.size() > 8 ||
        iterText.find_first_not_of("0123456789") != std::string::npos) {
        return false;
    }
    unsigned long iterations = std::stoul(iterText);
    if (iterations == 0 || iterations > 10000000UL) return false;

    // Salt: exactly 16 bytes = exactly 32 hexadecimal characters
    std::string salt;
    if (parts[2].size() != SALT_LENGTH * 2 || !fromHex(parts[2], salt)) return false;

    // Hash: exactly 32 bytes = exactly 64 hexadecimal characters
    std::string expected;
    if (parts[3].size() != HASH_LENGTH * 2 || !fromHex(parts[3], expected)) return false;

    // Compare the raw bytes without stopping at the first difference
    std::string actual = toText(pbkdf2(toBytes(password), toBytes(salt),
                                       static_cast<unsigned int>(iterations), HASH_LENGTH));
    return sameText(actual, expected);
}

} // namespace PasswordUtils
