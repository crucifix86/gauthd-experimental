#ifndef __BCRYPT_AUTH_H
#define __BCRYPT_AUTH_H

#include <string>
#include "octets.h"

namespace GNET
{

class BcryptAuth
{
public:
    // Generate bcrypt hash from password
    static bool HashPassword(const std::string& password, std::string& hash);
    
    // Verify password against bcrypt hash
    static bool VerifyPassword(const std::string& password, const std::string& hash);
    
    // Convert Octets to string for bcrypt processing
    static std::string OctetsToString(const Octets& oct);
    
    // Convert string to Octets for storage
    static Octets StringToOctets(const std::string& str);
    
    // Check if a hash is bcrypt format (for migration detection)
    static bool IsBcryptHash(const std::string& hash);
    
    // Migrate old MD5 hash to bcrypt (requires plaintext password)
    static bool MigratePassword(const Octets& old_hash, const std::string& password, std::string& new_hash);
    
private:
    static const int BCRYPT_COST = 12; // Cost factor for bcrypt
};

}

#endif // __BCRYPT_AUTH_H