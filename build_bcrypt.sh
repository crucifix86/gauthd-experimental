#!/bin/bash

# Build script for gauthd with bcrypt support

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== Building gauthd with bcrypt support ===${NC}"

# Install bcrypt library if not present
echo -e "${YELLOW}Checking for bcrypt library...${NC}"
if ! dpkg -l | grep -q libbcrypt-dev; then
    echo -e "${YELLOW}Installing bcrypt library...${NC}"
    sudo apt-get update
    sudo apt-get install -y libbcrypt-dev
fi

# Create a simple bcrypt test to verify installation
echo -e "${YELLOW}Creating bcrypt test...${NC}"
cat > test_bcrypt.c << 'EOF'
#include <stdio.h>
#include <string.h>
#include <bcrypt/bcrypt.h>

int main() {
    char salt[64];
    char hash[64];
    const char *password = "test123";
    
    if (bcrypt_gensalt(12, salt) != 0) {
        printf("Failed to generate salt\n");
        return 1;
    }
    
    if (bcrypt_hashpw(password, salt, hash) != 0) {
        printf("Failed to hash password\n");
        return 1;
    }
    
    printf("Password: %s\n", password);
    printf("Hash: %s\n", hash);
    
    if (bcrypt_checkpw(password, hash) == 0) {
        printf("Password verification: SUCCESS\n");
    } else {
        printf("Password verification: FAILED\n");
    }
    
    return 0;
}
EOF

# Test bcrypt library
echo -e "${YELLOW}Testing bcrypt library...${NC}"
gcc -o test_bcrypt test_bcrypt.c -lbcrypt
if ./test_bcrypt; then
    echo -e "${GREEN}✓ Bcrypt library working correctly${NC}"
else
    echo -e "${RED}✗ Bcrypt library test failed${NC}"
    exit 1
fi
rm -f test_bcrypt test_bcrypt.c

# Build gauthd
echo -e "${YELLOW}Building gauthd...${NC}"
make clean
make all || { echo -e "${RED}Build failed${NC}"; exit 1; }

echo -e "${GREEN}=== Build completed successfully ===${NC}"
echo -e "${YELLOW}Note: Don't forget to:${NC}"
echo "1. Run the SQL migration script: mysql -u root -p < bcrypt_migration.sql"
echo "2. Update your gauthd.conf if needed"
echo "3. Test with a small set of users first"