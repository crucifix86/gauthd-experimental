# gauthd-experimental

Experimental fork of the Perfect World gauthd authentication server with modern security improvements.

## Overview

This project aims to modernize the authentication mechanisms in the gauthd server by replacing outdated hashing methods with bcrypt, a more secure password hashing algorithm.

## Current Issues

The original gauthd implementation uses insecure password hashing methods:
- Plain MD5 hashing (vulnerable to rainbow tables)
- Base64 encoding (not a hashing algorithm)
- Binary storage (plain text)

## Planned Improvements

1. **Bcrypt Integration**: Replace MD5 with bcrypt for password hashing
2. **Migration Support**: Provide tools to migrate existing user passwords
3. **Backwards Compatibility**: Optional support for legacy authentication during transition
4. **Security Hardening**: Additional anti-brute force mechanisms

## Building

### Prerequisites
- g++ with C++11 support
- MySQL client libraries (32-bit)
- Lua 5.1 libraries (32-bit)
- bcrypt library

### Build Instructions
```bash
# Clone the repository
git clone https://github.com/crucifix86/gauthd-experimental.git
cd gauthd-experimental

# Build gauthd
make clean
make all
```

## Configuration

Edit `gauthd.conf` to configure:
- MySQL database connection
- Server port and network settings
- Security parameters

## Security Warning

This is experimental software. Do not use in production without thorough testing.

## License

This project inherits the license from the original Perfect World server code.