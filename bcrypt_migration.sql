-- SQL script to add bcrypt support to Perfect World authentication database
-- This adds a password type column to track which hashing method is used

-- Add passwd_type column to auth table if it doesn't exist
ALTER TABLE auth 
ADD COLUMN IF NOT EXISTS passwd_type TINYINT DEFAULT 1 
COMMENT '1=MD5, 2=Base64, 3=Binary, 4=Bcrypt';

-- Update existing records to mark them as MD5 (assuming that's the current format)
UPDATE auth SET passwd_type = 1 WHERE passwd_type IS NULL OR passwd_type = 0;

-- Create index on passwd_type for faster lookups during migration
CREATE INDEX IF NOT EXISTS idx_passwd_type ON auth(passwd_type);

-- Optional: Create a migration status table to track progress
CREATE TABLE IF NOT EXISTS auth_migration_status (
    userid INT PRIMARY KEY,
    migration_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    old_type TINYINT,
    new_type TINYINT,
    INDEX idx_migration_date (migration_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Create stored procedure for safe password updates
DELIMITER $$

CREATE PROCEDURE IF NOT EXISTS update_user_password_bcrypt(
    IN p_userid INT,
    IN p_bcrypt_hash VARCHAR(255)
)
BEGIN
    -- Record the old password type for audit
    INSERT INTO auth_migration_status (userid, old_type, new_type)
    SELECT p_userid, passwd_type, 4
    FROM auth
    WHERE userid = p_userid
    ON DUPLICATE KEY UPDATE 
        migration_date = CURRENT_TIMESTAMP,
        old_type = VALUES(old_type),
        new_type = 4;
    
    -- Update the password and type
    UPDATE auth 
    SET passwd = p_bcrypt_hash, 
        passwd_type = 4 
    WHERE userid = p_userid;
END$$

DELIMITER ;