#include "authapi.h"

#include <string>
#include <iostream>

#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

// Read the Bearer token from the request
static string getBearerToken(const httplib::Request& req)
{
    string header = req.get_header_value("Authorization");

    const string prefix = "Bearer ";

    if (header.size() <= prefix.size() ||
        header.compare(0, prefix.size(), prefix) != 0)
    {
        return "";
    }

    return header.substr(prefix.size());
}

// Check whether the request belongs to an active Admin
static bool isAdminRequest(
    const httplib::Request& req,
    AuthManager& auth)
{
    string token = getBearerToken(req);

    AuthResult user;

    return !token.empty() &&
           auth.validateSession(token, "admin", user);
}

// Register authentication routes
void registerAuthRoutes(
    httplib::Server& server,
    AuthManager& auth)
{
    // ----------------------------------------
    // API: Citizen and Driver registration
    // POST /api/auth/register
    // ----------------------------------------

    server.Post("/api/auth/register",
        [&auth](
            const httplib::Request& req,
            httplib::Response& res)
        {
            json data = json::parse(req.body, nullptr, false);

            if (data.is_discarded() ||
                !data.is_object() ||
                !data.contains("role") ||
                !data.contains("name") ||
                !data.contains("email") ||
                !data.contains("phone") ||
                !data.contains("password"))
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Missing registration details"})",
                    "application/json"
                );
                return;
            }

            if (!data["role"].is_string() ||
                !data["name"].is_string() ||
                !data["email"].is_string() ||
                !data["phone"].is_string() ||
                !data["password"].is_string())
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Invalid registration fields"})",
                    "application/json"
                );
                return;
            }

            string role = data["role"].get<string>();
            string name = data["name"].get<string>();
            string email = data["email"].get<string>();
            string phone = data["phone"].get<string>();
            string password = data["password"].get<string>();

            // AuthManager allows only citizen and driver
            AuthResult result = auth.registerAccount(
                role,
                name,
                email,
                phone,
                password
            );

            json response;

            response["success"] = result.success;
            response["message"] = result.message;

            if (result.success)
            {
                response["accountId"] = result.accountID;
                response["role"] = result.role;
                response["status"] = result.status;

                res.status =
                    (role == "driver") ? 202 : 201;
            }
            else
            {
                res.status = 400;
            }

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );


    // ----------------------------------------
    // API: Login
    // POST /api/auth/login
    // ----------------------------------------

    server.Post("/api/auth/login",
        [&auth](
            const httplib::Request& req,
            httplib::Response& res)
        {
            json data = json::parse(req.body, nullptr, false);

            if (data.is_discarded() ||
                !data.is_object() ||
                !data.contains("role") ||
                !data.contains("email") ||
                !data.contains("password"))
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Please provide role, email and password"})",
                    "application/json"
                );
                return;
            }

            if (!data["role"].is_string() ||
                !data["email"].is_string() ||
                !data["password"].is_string())
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Invalid login details"})",
                    "application/json"
                );
                return;
            }

            string role = data["role"].get<string>();
            string email = data["email"].get<string>();
            string password = data["password"].get<string>();

            AuthResult result = auth.login(
                role,
                email,
                password
            );

            json response;

            response["success"] = result.success;
            response["message"] = result.message;

            if (result.success)
            {
                response["token"] = result.sessionToken;
                response["accountId"] = result.accountID;
                response["role"] = result.role;
                response["name"] = result.name;
                response["email"] = result.email;
                response["phone"] = result.phone;
                response["status"] = result.status;

                if (result.role == "driver")
                {
                    response["driverId"] = result.driverID;
                }

                res.status = 200;
            }
            else
            {
                // Pending driver applications receive 403
                res.status =
                    (result.status == "pending" ||
                     result.status == "rejected")
                    ? 403 : 401;
            }

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );


    // ----------------------------------------
    // API: Logout
    // POST /api/auth/logout
    // ----------------------------------------

    server.Post("/api/auth/logout",
        [&auth](
            const httplib::Request& req,
            httplib::Response& res)
        {
            string token = getBearerToken(req);

            if (token.empty())
            {
                res.status = 401;
                res.set_content(
                    R"({"success":false,"message":"Login token is required"})",
                    "application/json"
                );
                return;
            }

            bool success = auth.logout(token);

            json response;

            response["success"] = success;
            response["message"] = success
                ? "Logged out successfully"
                : "Session not found or already expired";

            res.status = success ? 200 : 401;

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );


    // ----------------------------------------
    // =====================================
// CITIZEN PROFILE API
// =====================================

// GET /api/auth/profile
// Get the logged-in citizen's profile

server.Get("/api/auth/profile",
    [&auth](
        const httplib::Request& req,
        httplib::Response& res)
    {
        string token = getBearerToken(req);
        AuthResult user;

        if (token.empty() ||
            !auth.validateSession(token, "citizen", user))
        {
            res.status = 401;
            res.set_content(
                R"({"success":false,"message":"Citizen login required"})",
                "application/json"
            );
            return;
        }

        json response;

        response["success"] = true;
        response["accountId"] = user.accountID;
        response["name"] = user.name;
        response["email"] = user.email;
        response["phone"] = user.phone;
        response["role"] = user.role;

        res.set_content(
            response.dump(4),
            "application/json"
        );
    }
);


// PUT /api/auth/profile
// Update the logged-in citizen's profile

server.Put("/api/auth/profile",
    [&auth](
        const httplib::Request& req,
        httplib::Response& res)
    {
        string token = getBearerToken(req);
        AuthResult user;

        if (token.empty() ||
            !auth.validateSession(token, "citizen", user))
        {
            res.status = 401;
            res.set_content(
                R"({"success":false,"message":"Citizen login required"})",
                "application/json"
            );
            return;
        }

        json data = json::parse(req.body, nullptr, false);

        if (data.is_discarded() ||
            !data.is_object() ||
            !data.contains("name") ||
            !data.contains("email") ||
            !data.contains("phone") ||
            !data["name"].is_string() ||
            !data["email"].is_string() ||
            !data["phone"].is_string())
        {
            res.status = 400;
            res.set_content(
                R"({"success":false,"message":"Invalid profile details"})",
                "application/json"
            );
            return;
        }

        string name = data["name"].get<string>();
        string email = data["email"].get<string>();
        string phone = data["phone"].get<string>();
        string errorMessage;

        bool success = auth.updateProfile(
            user.accountID,
            name,
            email,
            phone,
            errorMessage
        );

        json response;

        response["success"] = success;
        response["message"] = errorMessage;

        if (success)
        {
            AuthResult updatedUser;

            if (auth.validateSession(
                    token, "citizen", updatedUser))
            {
                response["name"] = updatedUser.name;
                response["email"] = updatedUser.email;
                response["phone"] = updatedUser.phone;
            }

            res.status = 200;
        }
        else
        {
            res.status = 400;
        }

        res.set_content(
            response.dump(4),
            "application/json"
        );
    }
);
    // API: List pending Driver requests
    // GET /api/admin/driver-requests
    // Admin only
    // ----------------------------------------

    server.Get("/api/admin/driver-requests",
        [&auth](
            const httplib::Request& req,
            httplib::Response& res)
        {
            if (!isAdminRequest(req, auth))
            {
                res.status = 403;
                res.set_content(
                    R"({"success":false,"message":"Admin access required"})",
                    "application/json"
                );
                return;
            }

            vector<DriverRequest> requests;

            if (!auth.loadPendingDriverRequests(requests))
            {
                res.status = 500;
                res.set_content(
                    R"({"success":false,"message":"Could not load driver requests"})",
                    "application/json"
                );
                return;
            }

            json result = json::array();

            for (const DriverRequest& request : requests)
            {
                json item;

                item["accountId"] = request.accountID;
                item["name"] = request.name;
                item["email"] = request.email;
                item["phone"] = request.phone;
                item["createdAt"] = request.createdAt;

                result.push_back(item);
            }

            res.set_content(
                result.dump(4),
                "application/json"
            );
        }
    );


    // ----------------------------------------
    // API: Approve a Driver request
    // POST /api/admin/driver-requests/{id}/approve
    // Admin only
    // ----------------------------------------

    server.Post(
        R"(/api/admin/driver-requests/([0-9]+)/approve)",
        [&auth](
            const httplib::Request& req,
            httplib::Response& res)
        {
            if (!isAdminRequest(req, auth))
            {
                res.status = 403;
                res.set_content(
                    R"({"success":false,"message":"Admin access required"})",
                    "application/json"
                );
                return;
            }

            int accountID = 0;

            try
            {
                accountID = stoi(req.matches[1].str());
            }
            catch (...)
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Invalid account ID"})",
                    "application/json"
                );
                return;
            }

            bool success = auth.approveDriverRequest(accountID);

            json response;

            response["success"] = success;
            response["message"] = success
                ? "Driver application approved successfully"
                : "Could not approve this driver request";

            res.status = success ? 200 : 409;

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );


    // ----------------------------------------
    // API: Reject a Driver request
    // POST /api/admin/driver-requests/{id}/reject
    // Admin only
    // ----------------------------------------

    server.Post(
        R"(/api/admin/driver-requests/([0-9]+)/reject)",
        [&auth](
            const httplib::Request& req,
            httplib::Response& res)
        {
            if (!isAdminRequest(req, auth))
            {
                res.status = 403;
                res.set_content(
                    R"({"success":false,"message":"Admin access required"})",
                    "application/json"
                );
                return;
            }

            int accountID = 0;

            try
            {
                accountID = stoi(req.matches[1].str());
            }
            catch (...)
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Invalid account ID"})",
                    "application/json"
                );
                return;
            }

            bool success = auth.rejectDriverRequest(accountID);

            json response;

            response["success"] = success;
            response["message"] = success
                ? "Driver application rejected"
                : "Could not reject this driver request";

            res.status = success ? 200 : 409;

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );

    cout << "Authentication routes registered successfully!\n";
}
