
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS user_accounts (
    account_id INTEGER PRIMARY KEY AUTOINCREMENT,

    role TEXT NOT NULL
        CHECK (role IN ('admin', 'driver', 'citizen')),

    full_name TEXT NOT NULL,
    email TEXT NOT NULL COLLATE NOCASE UNIQUE,
    phone TEXT NOT NULL UNIQUE,

    password_hash TEXT NOT NULL,
    password_salt TEXT NOT NULL,

    account_status TEXT NOT NULL DEFAULT 'active'
        CHECK (account_status IN ('active', 'pending', 'rejected')),

    driver_id INTEGER UNIQUE,

    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (driver_id)
        REFERENCES drivers(driver_id)
);

CREATE INDEX IF NOT EXISTS idx_accounts_role_status
ON user_accounts(role, account_status);


CREATE TABLE IF NOT EXISTS auth_sessions (
    session_id INTEGER PRIMARY KEY AUTOINCREMENT,

    account_id INTEGER NOT NULL,

    token_hash TEXT NOT NULL UNIQUE,

    expires_at DATETIME NOT NULL,

    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (account_id)
        REFERENCES user_accounts(account_id)
        ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_sessions_account
ON auth_sessions(account_id);

CREATE INDEX IF NOT EXISTS idx_sessions_expiry
ON auth_sessions(expires_at);
