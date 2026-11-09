
#include "authmanager.h"

#include <iostream>
#include <cctype>
#include <algorithm>
#include <vector>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

// Password security settings
const int HASH_ITERATIONS = 120000;
const int SALT_SIZE = 16;
const int HASH_SIZE = 32;

// Convert bytes into hexadecimal text
static string toHex(const unsigned char* data, int length)
{
    const char* digits = "0123456789abcdef";
    string result;

    for (int i = 0; i < length; i++)
    {
        result += digits[data[i] >> 4];
        result += digits[data[i] & 15];
    }

    return result;
}

// Convert hexadecimal text into bytes
static bool fromHex(
    const string& input,
    vector<unsigned char>& output)
{
    if (input.size() % 2 != 0)
        return false;

    output.clear();

    for (size_t i = 0; i < input.size(); i += 2)
    {
        auto digit = [](char c) -> int
        {
            if (c >= '0' && c <= '9')
                return c - '0';

            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;

            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;

            return -1;
        };

        int high = digit(input[i]);
        int low = digit(input[i + 1]);

        if (high < 0 || low < 0)
            return false;

        output.push_back(
            static_cast<unsigned char>(high * 16 + low)
        );
    }

    return true;
}

// Generate SHA-256 hash of a session token
static bool hashToken(
    const string& token,
    string& tokenHash)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength = 0;

    if (EVP_Digest(
            token.data(),
            token.size(),
            digest,
            &digestLength,
            EVP_sha256(),
            NULL) != 1)
    {
        return false;
    }

    tokenHash = toHex(
        digest,
        static_cast<int>(digestLength)
    );

    OPENSSL_cleanse(digest, sizeof(digest));

    return true;
}

// Basic phone validation
static bool validPhone(const string& phone)
{
    if (phone.size() != 10)
        return false;

    for (char digit : phone)
    {
        if (!isdigit(static_cast<unsigned char>(digit)))
            return false;
    }

    return true;
}

// Basic email validation
static bool validEmail(const string& email)
{
    size_t at = email.find('@');

    if (at == string::npos || at == 0)
        return false;

    size_t dot = email.find('.', at + 2);

    if (dot == string::npos || dot == email.size() - 1)
        return false;

    return true;
}

// Convert email to lowercase
static string normalizeEmail(string email)
{
    transform(
        email.begin(),
        email.end(),
        email.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(tolower(c));
        }
    );

    return email;
}

// Constructor
AuthManager::AuthManager()
{
    db = NULL;
}

// Destructor
AuthManager::~AuthManager()
{
    close();
}

// Connect to SQLite
bool AuthManager::connect(const string& databasePath)
{
    close();

    if (sqlite3_open(databasePath.c_str(), &db) != SQLITE_OK)
    {
        cout << "Authentication database connection failed.\n";
        close();
        return false;
    }

    sqlite3_busy_timeout(db, 5000);

    if (sqlite3_exec(
            db,
            "PRAGMA foreign_keys = ON;",
            NULL,
            NULL,
            NULL) != SQLITE_OK)
    {
        close();
        return false;
    }

    cout << "Authentication database connected!\n";

    return true;
}

// Close database
void AuthManager::close()
{
    if (db != NULL)
    {
        sqlite3_close(db);
        db = NULL;
    }
}

// Generate random salt and hash password
bool AuthManager::hashPassword(
    const string& password,
    string& salt,
    string& passwordHash)
{
    unsigned char saltBytes[SALT_SIZE];
    unsigned char hashBytes[HASH_SIZE];

    if (RAND_bytes(saltBytes, SALT_SIZE) != 1)
        return false;

    int result = PKCS5_PBKDF2_HMAC(
        password.c_str(),
        static_cast<int>(password.size()),
        saltBytes,
        SALT_SIZE,
        HASH_ITERATIONS,
        EVP_sha256(),
        HASH_SIZE,
        hashBytes
    );

    if (result != 1)
        return false;

    salt = toHex(saltBytes, SALT_SIZE);
    passwordHash = toHex(hashBytes, HASH_SIZE);

    OPENSSL_cleanse(hashBytes, HASH_SIZE);

    return true;
}

// Verify entered password
bool AuthManager::verifyPassword(
    const string& password,
    const string& salt,
    const string& passwordHash)
{
    vector<unsigned char> saltBytes;
    vector<unsigned char> storedHash;

    if (!fromHex(salt, saltBytes) ||
        !fromHex(passwordHash, storedHash))
        return false;

    if (saltBytes.size() != SALT_SIZE ||
        storedHash.size() != HASH_SIZE)
        return false;

    unsigned char calculatedHash[HASH_SIZE];

    int result = PKCS5_PBKDF2_HMAC(
        password.c_str(),
        static_cast<int>(password.size()),
        saltBytes.data(),
        static_cast<int>(saltBytes.size()),
        HASH_ITERATIONS,
        EVP_sha256(),
        HASH_SIZE,
        calculatedHash
    );

    if (result != 1)
        return false;

    bool matches =
        CRYPTO_memcmp(
            calculatedHash,
            storedHash.data(),
            HASH_SIZE
        ) == 0;

    OPENSSL_cleanse(calculatedHash, HASH_SIZE);

    return matches;
}

// Create a new secure login session
bool AuthManager::createSession(AuthResult& result)
{
    if (db == NULL || result.accountID <= 0)
        return false;

    // Generate a random 32-byte token
    unsigned char tokenBytes[32];

    if (RAND_bytes(tokenBytes, sizeof(tokenBytes)) != 1)
        return false;

    string token = toHex(tokenBytes, sizeof(tokenBytes));

    OPENSSL_cleanse(tokenBytes, sizeof(tokenBytes));

    // Store only the token hash in SQLite
    string tokenHash;

    if (!hashToken(token, tokenHash))
        return false;

    // Remove expired sessions
    sqlite3_exec(
        db,
        "DELETE FROM auth_sessions "
        "WHERE expires_at <= datetime('now');",
        NULL,
        NULL,
        NULL
    );

    const char* sql =
        "INSERT INTO auth_sessions "
        "(account_id, token_hash, expires_at) "
        "VALUES (?, ?, datetime('now', '+12 hours'));";

    sqlite3_stmt* statement = NULL;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_int(
        statement, 1, result.accountID);

    sqlite3_bind_text(
        statement, 2, tokenHash.c_str(),
        -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);

    if (!success)
        return false;

    // Return the original token to the frontend.
    // Only its hash is stored in SQLite.
    result.sessionToken = token;

    return true;
}

// Register Citizen or Driver
AuthResult AuthManager::registerAccount(
    const string& role,
    const string& name,
    const string& email,
    const string& phone,
    const string& password)
{
    AuthResult result;

    if (db == NULL)
    {
        result.message = "Database is not connected.";
        return result;
    }

    // Admin cannot register publicly
    if (role != "citizen" && role != "driver")
    {
        result.message = "This role cannot register publicly.";
        return result;
    }

    string cleanName = name;
    string cleanEmail = normalizeEmail(email);

    if (cleanName.empty() ||
        cleanName.size() > 100 ||
        !validEmail(cleanEmail) ||
        !validPhone(phone) ||
        password.size() < 8 ||
        password.size() > 128)
    {
        result.message = "Please enter valid registration details.";
        return result;
    }

    // Citizens are active immediately.
    // Drivers require Admin approval.
    string status =
        (role == "citizen") ? "active" : "pending";

    string salt;
    string passwordHash;

    if (!hashPassword(password, salt, passwordHash))
    {
        result.message = "Password hashing failed.";
        return result;
    }

    const char* sql =
        "INSERT INTO user_accounts "
        "(role, full_name, email, phone, password_hash, "
        "password_salt, account_status) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* statement = NULL;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            NULL) != SQLITE_OK)
    {
        result.message = "Could not prepare registration.";
        return result;
    }

    sqlite3_bind_text(
        statement, 1, role.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 2, cleanName.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 3, cleanEmail.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 4, phone.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 5, passwordHash.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 6, salt.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 7, status.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(statement) == SQLITE_DONE;

    if (success)
    {
        result.accountID = static_cast<int>(
            sqlite3_last_insert_rowid(db)
        );
    }

    sqlite3_finalize(statement);

    if (!success)
    {
        result.message =
            "Email or phone number is already registered.";
        return result;
    }

    result.success = true;
    result.role = role;
    result.name = cleanName;
    result.email = cleanEmail;
    result.phone = phone;
    result.status = status;

    result.message =
        (role == "citizen")
        ? "Citizen account created successfully."
        : "Driver application submitted. Waiting for Admin approval.";

    return result;
}

// Login with role, email and password
AuthResult AuthManager::login(
    const string& role,
    const string& email,
    const string& password)
{
    AuthResult result;

    if (db == NULL)
    {
        result.message = "Database is not connected.";
        return result;
    }

    if (role != "admin" &&
        role != "driver" &&
        role != "citizen")
    {
        result.message = "Invalid account role.";
        return result;
    }

    if (email.empty() || password.empty())
    {
        result.message = "Enter email and password.";
        return result;
    }

    string cleanEmail = normalizeEmail(email);

    const char* sql =
        "SELECT account_id, full_name, email, phone, "
        "password_hash, password_salt, account_status, "
        "COALESCE(driver_id, 0) "
        "FROM user_accounts "
        "WHERE role = ? AND email = ?;";

    sqlite3_stmt* statement = NULL;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            NULL) != SQLITE_OK)
    {
        result.message = "Could not prepare login.";
        return result;
    }

    sqlite3_bind_text(
        statement, 1, role.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 2, cleanEmail.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(statement) != SQLITE_ROW)
    {
        sqlite3_finalize(statement);
        result.message = "Invalid email or password.";
        return result;
    }

    result.accountID = sqlite3_column_int(statement, 0);

    auto getText = [&](int column) -> string
    {
        const unsigned char* value =
            sqlite3_column_text(statement, column);

        return value
            ? reinterpret_cast<const char*>(value)
            : "";
    };

    result.name = getText(1);
    result.email = getText(2);
    result.phone = getText(3);

    string storedHash = getText(4);
    string storedSalt = getText(5);

    result.status = getText(6);
    result.driverID = sqlite3_column_int(statement, 7);

    sqlite3_finalize(statement);

    // Check password before revealing account status
    if (!verifyPassword(password, storedSalt, storedHash))
    {
        result.message = "Invalid email or password.";
        return result;
    }

    if (result.status == "pending")
    {
        result.message =
            "Your driver account is waiting for Admin approval.";
        return result;
    }

    if (result.status == "rejected")
    {
        result.message =
            "Your account request was rejected by Admin.";
        return result;
    }

    if (result.status != "active")
    {
        result.message = "Account is not active.";
        return result;
    }

    // Create session only after successful verification
    result.role = role;

    if (!createSession(result))
    {
        result.message = "Could not create login session.";
        return result;
    }

    result.success = true;
    result.message = "Login successful.";

    return result;
}

// Create the first Admin account privately
bool AuthManager::createInitialAdmin(
    const string& name,
    const string& email,
    const string& phone,
    const string& password)
{
    if (db == NULL ||
        name.empty() ||
        name.size() > 100 ||
        !validEmail(normalizeEmail(email)) ||
        !validPhone(phone) ||
        password.size() < 8 ||
        password.size() > 128)
        return false;

    const char* checkSQL =
        "SELECT COUNT(*) FROM user_accounts "
        "WHERE role = 'admin';";

    sqlite3_stmt* statement = NULL;

    if (sqlite3_prepare_v2(
            db,
            checkSQL,
            -1,
            &statement,
            NULL) != SQLITE_OK)
        return false;

    int count = 0;

    if (sqlite3_step(statement) == SQLITE_ROW)
        count = sqlite3_column_int(statement, 0);

    sqlite3_finalize(statement);

    if (count > 0)
        return false;

    string salt;
    string passwordHash;

    if (!hashPassword(password, salt, passwordHash))
        return false;

    const char* insertSQL =
        "INSERT INTO user_accounts "
        "(role, full_name, email, phone, password_hash, "
        "password_salt, account_status) "
        "VALUES ('admin', ?, ?, ?, ?, ?, 'active');";

    if (sqlite3_prepare_v2(
            db,
            insertSQL,
            -1,
            &statement,
            NULL) != SQLITE_OK)
        return false;

    string cleanEmail = normalizeEmail(email);

    sqlite3_bind_text(
        statement, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 2, cleanEmail.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 3, phone.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 4, passwordHash.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 5, salt.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);

    return success;
}

// Load pending driver requests
bool AuthManager::loadPendingDriverRequests(
    vector<DriverRequest>& requests)
{
    if (db == NULL)
        return false;

    const char* sql =
        "SELECT account_id, full_name, email, phone, created_at "
        "FROM user_accounts "
        "WHERE role = 'driver' AND account_status = 'pending' "
        "ORDER BY created_at ASC;";

    sqlite3_stmt* statement = NULL;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            NULL) != SQLITE_OK)
        return false;

    requests.clear();

    int step;

    while ((step = sqlite3_step(statement)) == SQLITE_ROW)
    {
        DriverRequest request;

        request.accountID = sqlite3_column_int(statement, 0);

        auto getText = [&](int column) -> string
        {
            const unsigned char* value =
                sqlite3_column_text(statement, column);

            return value
                ? reinterpret_cast<const char*>(value)
                : "";
        };

        request.name = getText(1);
        request.email = getText(2);
        request.phone = getText(3);
        request.createdAt = getText(4);

        requests.push_back(request);
    }

    sqlite3_finalize(statement);

    return step == SQLITE_DONE;
}

// Approve driver and create operational driver record
bool AuthManager::approveDriverRequest(int accountID)
{
    if (db == NULL || accountID <= 0)
        return false;

    if (sqlite3_exec(
            db,
            "BEGIN IMMEDIATE;",
            NULL,
            NULL,
            NULL) != SQLITE_OK)
        return false;

    bool success = true;
    sqlite3_stmt* statement = NULL;

    string driverName;
    string phone;

    const char* selectSQL =
        "SELECT full_name, phone FROM user_accounts "
        "WHERE account_id = ? "
        "AND role = 'driver' "
        "AND account_status = 'pending';";

    if (sqlite3_prepare_v2(
            db,
            selectSQL,
            -1,
            &statement,
            NULL) != SQLITE_OK)
    {
        success = false;
    }

    if (success)
    {
        sqlite3_bind_int(statement, 1, accountID);

        if (sqlite3_step(statement) == SQLITE_ROW)
        {
            const unsigned char* name =
                sqlite3_column_text(statement, 0);

            const unsigned char* phoneText =
                sqlite3_column_text(statement, 1);

            driverName = name
                ? reinterpret_cast<const char*>(name)
                : "";

            phone = phoneText
                ? reinterpret_cast<const char*>(phoneText)
                : "";
        }
        else
        {
            success = false;
        }
    }

    if (statement != NULL)
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }

    // Create driver record without assigning a truck yet
    const char* insertSQL =
        "INSERT INTO drivers "
        "(driver_name, phone_number, truck_id) "
        "VALUES (?, ?, NULL);";

    if (success)
    {
        if (sqlite3_prepare_v2(
                db,
                insertSQL,
                -1,
                &statement,
                NULL) != SQLITE_OK)
        {
            success = false;
        }
        else
        {
            sqlite3_bind_text(
                statement, 1, driverName.c_str(),
                -1, SQLITE_TRANSIENT);

            sqlite3_bind_text(
                statement, 2, phone.c_str(),
                -1, SQLITE_TRANSIENT);

            success = sqlite3_step(statement) == SQLITE_DONE;
        }
    }

    int driverID = 0;

    if (success)
    {
        driverID = static_cast<int>(
            sqlite3_last_insert_rowid(db)
        );
    }

    if (statement != NULL)
    {
        sqlite3_finalize(statement);
        statement = NULL;
    }

    // Activate account and link operational driver ID
    const char* updateSQL =
        "UPDATE user_accounts "
        "SET account_status = 'active', driver_id = ? "
        "WHERE account_id = ? "
        "AND role = 'driver' "
        "AND account_status = 'pending';";

    if (success)
    {
        if (sqlite3_prepare_v2(
                db,
                updateSQL,
                -1,
                &statement,
                NULL) != SQLITE_OK)
        {
            success = false;
        }
        else
        {
            sqlite3_bind_int(statement, 1, driverID);
            sqlite3_bind_int(statement, 2, accountID);

            success =
                sqlite3_step(statement) == SQLITE_DONE &&
                sqlite3_changes(db) == 1;
        }
    }

    if (statement != NULL)
        sqlite3_finalize(statement);

    if (success)
    {
        success = sqlite3_exec(
            db,
            "COMMIT;",
            NULL,
            NULL,
            NULL
        ) == SQLITE_OK;
    }

    if (!success)
    {
        sqlite3_exec(
            db,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );
    }

    return success;
}

// Reject a pending driver request
bool AuthManager::rejectDriverRequest(int accountID)
{
    if (db == NULL || accountID <= 0)
        return false;

    const char* sql =
        "UPDATE user_accounts "
        "SET account_status = 'rejected' "
        "WHERE account_id = ? "
        "AND role = 'driver' "
        "AND account_status = 'pending';";

    sqlite3_stmt* statement = NULL;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_int(statement, 1, accountID);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) == 1;

    sqlite3_finalize(statement);

    return success;
}

// Validate session and required role
bool AuthManager::validateSession(
    const string& token,
    const string& requiredRole,
    AuthResult& user)
{
    if (db == NULL || token.empty())
        return false;

    if (requiredRole != "admin" &&
        requiredRole != "driver" &&
        requiredRole != "citizen")
        return false;

    string tokenHash;

    if (!hashToken(token, tokenHash))
        return false;

    const char* sql =
        "SELECT u.account_id, u.role, u.full_name, "
        "u.email, u.phone, u.account_status, "
        "COALESCE(u.driver_id, 0) "
        "FROM auth_sessions s "
        "JOIN user_accounts u "
        "ON s.account_id = u.account_id "
        "WHERE s.token_hash = ? "
        "AND s.expires_at > datetime('now') "
        "AND u.account_status = 'active' "
        "AND u.role = ?;";

    sqlite3_stmt* statement = NULL;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_text(
        statement, 1, tokenHash.c_str(),
        -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 2, requiredRole.c_str(),
        -1, SQLITE_TRANSIENT);

    bool valid = sqlite3_step(statement) == SQLITE_ROW;

    if (valid)
    {
        user.accountID = sqlite3_column_int(statement, 0);

        auto getText = [&](int column) -> string
        {
            const unsigned char* value =
                sqlite3_column_text(statement, column);

            return value
                ? reinterpret_cast<const char*>(value)
                : "";
        };

        user.role = getText(1);
        user.name = getText(2);
        user.email = getText(3);
        user.phone = getText(4);
        user.status = getText(5);
        user.driverID = sqlite3_column_int(statement, 6);

        user.success = true;
        user.message = "Session is valid.";
    }

    sqlite3_finalize(statement);

    return valid;
}

// Logout and remove the stored session
bool AuthManager::logout(const string& token)
{
    if (db == NULL || token.empty())
        return false;

    string tokenHash;

    if (!hashToken(token, tokenHash))
        return false;

    const char* sql =
        "DELETE FROM auth_sessions WHERE token_hash = ?;";

    sqlite3_stmt* statement = NULL;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_text(
        statement, 1, tokenHash.c_str(),
        -1, SQLITE_TRANSIENT);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) == 1;

    sqlite3_finalize(statement);

    return success;
}
// =====================================
// UPDATE CITIZEN PROFILE
// =====================================

bool AuthManager::updateProfile(
    int accountID,
    const string& name,
    const string& email,
    const string& phone,
    string& errorMessage)
{
    if (db == NULL || accountID <= 0)
    {
        errorMessage = "Invalid account.";
        return false;
    }

    if (name.empty() || email.empty() || !validPhone(phone))
    {
        errorMessage = "Please enter valid profile details.";
        return false;
    }

    size_t atPosition = email.find('@');
    size_t dotPosition = email.rfind('.');

    if (atPosition == string::npos ||
        dotPosition == string::npos ||
        atPosition == 0 ||
        dotPosition < atPosition + 2 ||
        dotPosition == email.size() - 1)
    {
        errorMessage = "Please enter a valid email address.";
        return false;
    }

    const char* sql =
        "UPDATE user_accounts "
        "SET full_name = ?, email = ?, phone = ? "
        "WHERE account_id = ? "
        "AND role = 'citizen' "
        "AND account_status = 'active';";

    sqlite3_stmt* statement = NULL;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            NULL) != SQLITE_OK)
    {
        errorMessage = "Could not prepare profile update.";
        return false;
    }

    sqlite3_bind_text(
        statement, 1, name.c_str(),
        -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 2, email.c_str(),
        -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        statement, 3, phone.c_str(),
        -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(
        statement, 4, accountID);

    int result = sqlite3_step(statement);

    bool success =
        result == SQLITE_DONE &&
        sqlite3_changes(db) == 1;

    if (success)
    {
        errorMessage = "Profile updated successfully.";
    }
    else if ((result & 0xFF) == SQLITE_CONSTRAINT)
    {
        errorMessage =
            "This email or phone number is already registered.";
    }
    else
    {
        errorMessage =
            result == SQLITE_DONE
                ? "Citizen account not found."
                : "Could not update profile.";
    }

    sqlite3_finalize(statement);

    return success;
}