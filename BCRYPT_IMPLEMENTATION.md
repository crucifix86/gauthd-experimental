# Bcrypt Implementation Guide

## Overview

This document describes the bcrypt password hashing implementation for the gauthd authentication server.

## Architecture Changes

### 1. New Components

- **BcryptAuth class** (`bcrypt_auth.h/cpp`): Handles bcrypt operations
  - Password hashing with configurable cost factor (default: 12)
  - Password verification
  - Hash format detection
  - Octets/string conversion utilities

- **Enhanced AuthManager** (`authmanager.h/cpp`): 
  - `AuthPasswdBcrypt()`: Verify passwords using bcrypt
  - `CreateBcryptHash()`: Generate new bcrypt hashes
  - Thread-safe bcrypt operations

- **Extended MySQL Manager** (`gmysqlclient.h/cpp`):
  - `MatrixPasswdBcrypt()`: Retrieve user credentials for bcrypt auth
  - `UpdatePasswordToBcrypt()`: Update password to bcrypt format
  - `CheckPasswordFormat()`: Determine current hash type

### 2. Database Schema Changes

The `auth` table now includes:
```sql
passwd_type TINYINT DEFAULT 1
```

Values:
- 1 = MD5 hash
- 2 = Base64 encoded
- 3 = Binary/plain text
- 4 = Bcrypt hash

### 3. Authentication Flow

1. **User Login Request**
   - Client sends username and password
   - Server queries database for user credentials

2. **Hash Type Detection**
   - Check `passwd_type` column
   - If not set, analyze hash format

3. **Authentication**
   - **Bcrypt (type 4)**: Use bcrypt verification
   - **Legacy (types 1-3)**: Use existing auth methods

4. **Automatic Migration**
   - On successful legacy auth, generate bcrypt hash
   - Update database with new hash and type

## Implementation Details

### Password Hashing
```cpp
std::string password = "user_password";
std::string hash;
if (BcryptAuth::HashPassword(password, hash)) {
    // Store hash in database
}
```

### Password Verification
```cpp
std::string password = "user_password";
std::string stored_hash = "$2a$12$..."; // From database
if (BcryptAuth::VerifyPassword(password, stored_hash)) {
    // Authentication successful
}
```

### Migration Process
```cpp
// In authentication handler
if (legacy_auth_success) {
    std::string bcrypt_hash;
    if (AuthManager::CreateBcryptHash(password, bcrypt_hash)) {
        db->UpdatePasswordToBcrypt(userid, bcrypt_hash);
    }
}
```

## Security Considerations

1. **Cost Factor**: Set to 12 for balance between security and performance
2. **Migration**: Passwords are only migrated on successful login
3. **Backwards Compatibility**: System supports both legacy and bcrypt auth
4. **Thread Safety**: All bcrypt operations are mutex-protected

## Deployment Steps

1. **Database Migration**
   ```bash
   mysql -u root -p game_database < bcrypt_migration.sql
   ```

2. **Build with Bcrypt**
   ```bash
   ./build_bcrypt.sh
   ```

3. **Configuration**
   - No configuration changes required
   - System auto-detects hash types

4. **Testing**
   - Test with existing accounts (legacy auth)
   - Create new accounts (bcrypt from start)
   - Verify automatic migration works

## Performance Impact

- Initial login: ~100ms additional for bcrypt verification
- Subsequent logins: Cached sessions unaffected
- Migration: One-time cost per user on first login

## Monitoring

Track migration progress:
```sql
SELECT passwd_type, COUNT(*) 
FROM auth 
GROUP BY passwd_type;
```

View migration history:
```sql
SELECT * FROM auth_migration_status 
ORDER BY migration_date DESC 
LIMIT 100;
```

## Rollback Plan

If issues arise:
1. Revert to original gauthd binary
2. Keep database changes (backwards compatible)
3. passwd_type column can remain for future use

## Future Enhancements

1. Configurable cost factor per deployment
2. Support for Argon2 hashing
3. Batch migration tools for offline conversion
4. Password strength requirements