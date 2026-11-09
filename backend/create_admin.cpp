#include "authmanager.h"

#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>

using namespace std;

// Read password without displaying typed characters
string readHiddenPassword()
{
    string password;
    termios oldSettings, newSettings;

    if (tcgetattr(STDIN_FILENO, &oldSettings) != 0)
        return "";

    newSettings = oldSettings;
    newSettings.c_lflag &= ~ECHO;

    if (tcsetattr(
            STDIN_FILENO,
            TCSAFLUSH,
            &newSettings) != 0)
        return "";

    getline(cin, password);

    tcsetattr(
        STDIN_FILENO,
        TCSAFLUSH,
        &oldSettings
    );

    cout << endl;

    return password;
}

int main()
{
    cout << "====================================\n";
    cout << "     SMARTWASTE ADMIN SETUP\n";
    cout << "====================================\n\n";

    AuthManager auth;

    if (!auth.connect("database/smartwaste.db"))
    {
        cerr << "Database connection failed.\n";
        return 1;
    }

    string name, email, phone;

    cout << "Enter Admin Name: ";
    getline(cin, name);

    cout << "Enter Admin Email: ";
    getline(cin, email);

    cout << "Enter Admin Phone (10 digits): ";
    getline(cin, phone);

    cout << "Create Admin Password: ";
    string password = readHiddenPassword();

    cout << "Confirm Admin Password: ";
    string confirmPassword = readHiddenPassword();

    if (password != confirmPassword)
    {
        cout << "Passwords do not match.\n";
        auth.close();
        return 1;
    }

    if (password.size() < 8)
    {
        cout << "Password must contain at least 8 characters.\n";
        auth.close();
        return 1;
    }

    bool success = auth.createInitialAdmin(
        name,
        email,
        phone,
        password
    );

    auth.close();

    if (success)
    {
        cout << "\nAdmin account created successfully!\n";
        cout << "You can now use Admin Login.\n";
    }
    else
    {
        cout << "\nAdmin account could not be created.\n";
        cout << "Check the details or whether an Admin already exists.\n";
    }

    return success ? 0 : 1;
}
