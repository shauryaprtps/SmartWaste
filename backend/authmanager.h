
#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <string>
#include <vector>
#include <sqlite3.h>

using namespace std;

struct AuthResult
{
    bool success = false;
    string message;

    int accountID = 0;
    string role;
    string name;
    string email;
    string phone;
    string status;

    int driverID = 0;

    // Returned after successful login
    string sessionToken;
};

struct DriverRequest
{
    int accountID = 0;
    string name;
    string email;
    string phone;
    string createdAt;
};

class AuthManager
{
private:
    sqlite3* db;

    bool hashPassword(
        const string& password,
        string& salt,
        string& passwordHash
    );

    bool verifyPassword(
        const string& password,
        const string& salt,
        const string& passwordHash
    );

    bool createSession(AuthResult& result);

public:
    AuthManager();
    ~AuthManager();

    bool connect(const string& databasePath);
    void close();

    AuthResult registerAccount(
        const string& role,
        const string& name,
        const string& email,
        const string& phone,
        const string& password
    );

    AuthResult login(
        const string& role,
        const string& email,
        const string& password
    );

    bool createInitialAdmin(
        const string& name,
        const string& email,
        const string& phone,
        const string& password
    );

    bool loadPendingDriverRequests(
        vector<DriverRequest>& requests
    );

    bool approveDriverRequest(int accountID);

    bool rejectDriverRequest(int accountID);

    // Validate a session token and its role
    bool validateSession(
        const string& token,
        const string& requiredRole,
        AuthResult& user
    );

    // End a login session
    bool logout(const string& token);
    // Update the logged-in citizen's profile
bool updateProfile(
    int accountID,
    const string& name,
    const string& email,
    const string& phone,
    string& errorMessage
);
};

#endif
