#include "bcrypt_auth.h"
#include <cstring>
#include <cstdio>

// For now, we'll use the libbcrypt C library
extern "C" {
    int bcrypt_gensalt(int factor, char salt[64]);
    int bcrypt_hashpw(const char *passwd, const char salt[64], char hash[64]);
    int bcrypt_checkpw(const char *passwd, const char hash[64]);
}

namespace GNET
{

bool BcryptAuth::HashPassword(const std::string& password, std::string& hash)
{
    char salt[64];
    char hash_buffer[64];
    
    // Generate salt with cost factor
    if (bcrypt_gensalt(BCRYPT_COST, salt) != 0) {
        return false;
    }
    
    // Hash the password
    if (bcrypt_hashpw(password.c_str(), salt, hash_buffer) != 0) {
        return false;
    }
    
    hash = std::string(hash_buffer);
    return true;
}

bool BcryptAuth::VerifyPassword(const std::string& password, const std::string& hash)
{
    // Verify password against hash
    return bcrypt_checkpw(password.c_str(), hash.c_str()) == 0;
}

std::string BcryptAuth::OctetsToString(const Octets& oct)
{
    return std::string((const char*)oct.begin(), oct.size());
}

Octets BcryptAuth::StringToOctets(const std::string& str)
{
    Octets oct;
    oct.replace(str.c_str(), str.length());
    return oct;
}

bool BcryptAuth::IsBcryptHash(const std::string& hash)
{
    // Bcrypt hashes start with $2a$, $2b$, or $2y$
    return hash.length() >= 4 && 
           hash[0] == '$' && 
           hash[1] == '2' && 
           (hash[2] == 'a' || hash[2] == 'b' || hash[2] == 'y') &&
           hash[3] == '$';
}

bool BcryptAuth::MigratePassword(const Octets& old_hash, const std::string& password, std::string& new_hash)
{
    // For migration, we need to verify the password against the old hash first
    // This would require implementing MD5 verification
    // For now, just create a new bcrypt hash
    return HashPassword(password, new_hash);
}

}