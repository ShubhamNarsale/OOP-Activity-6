// password_utils.h - Password hashing helper
//
// Passwords are never stored. Instead we store a salted PBKDF2-HMAC-SHA256 hash,
// a standard, published password-hashing method (RFC 8018 with HMAC-SHA-256).
// It is written here with the standard library only, so the build stays simple.
#ifndef PASSWORD_UTILS_H
#define PASSWORD_UTILS_H

#include <cstddef>
#include <string>

namespace PasswordUtils {

// Number of PBKDF2 rounds. More rounds = slower guessing for an attacker.
constexpr unsigned int DEFAULT_ITERATIONS = 100000;

// Returns a text record: "pbkdf2_sha256$<iterations>$<salt hex>$<hash hex>"
// A new 16-byte salt is created every time from the operating system's cryptographically
// secure generator (Windows: BCryptGenRandom), so equal passwords get different records.
// Throws std::runtime_error if the secure generator fails (no weaker fallback is used).
std::string hashPassword(const std::string& password);

// Checks a typed password against a record produced by hashPassword().
// A record must have exactly 4 '$'-separated parts: scheme "pbkdf2_sha256", a rounds
// count (1..10,000,000), a salt of exactly 32 hex characters (16 bytes) and a hash of
// exactly 64 hex characters (32 bytes). Anything else (empty, wrong length, odd length,
// non-hex) returns false for every password. Hash bytes are compared in constant time.
bool verifyPassword(const std::string& password, const std::string& storedRecord);

// Lower-level functions, exposed so they can be checked against published test vectors.
std::string sha256Hex(const std::string& data);
std::string pbkdf2HmacSha256Hex(const std::string& password, const std::string& salt,
                                unsigned int iterations, std::size_t keyLength);

} // namespace PasswordUtils

#endif // PASSWORD_UTILS_H
