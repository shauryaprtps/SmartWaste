
#include <iostream>
#include <string>
#include <vector>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "Database.h"
#include "Bin.h"
#include "authapi.h"

using namespace std;
using json = nlohmann::json;

int main()
{
    cout << "====================================\n";
    cout << "       SMARTWASTE HTTP API\n";
    cout << "====================================\n";

    // Connect to the existing SQLite database
    Database database;

    if (!database.connect("database/smartwaste.db"))
    {
        cerr << "Could not connect to SQLite database!\n";
        return 1;
    }
    // Connect authentication to the same SQLite database
AuthManager auth;

if (!auth.connect("database/smartwaste.db"))
{
    cerr << "Authentication database connection failed!\n";
    return 1;
}

    // Create HTTP server
    httplib::Server server;
    // Process one API request at a time.
// This protects the shared SQLite connection.
server.new_task_queue = [] {
    return new httplib::ThreadPool(1, 0);
};

    // Allow communication from the browser frontend
  // Allow communication from the browser frontend
server.set_default_headers({
    {"Access-Control-Allow-Origin", "*"},
    {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
    {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
});

    // Handle browser preflight requests
    server.Options(R"(/api/.*)",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            res.status = 204;
        }
    );
    // Register login, registration and Admin approval APIs
registerAuthRoutes(server, auth);

    // ------------------------------------
    // API 1: Check server status
    // GET /api/health
    // ------------------------------------

    server.Get("/api/health",
        [](const httplib::Request& req,
           httplib::Response& res)
        {
            json response;

            response["success"] = true;
            response["message"] =
                "SmartWaste API is running";

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );

    // ------------------------------------
    // API 2: Get all bins from SQLite
    // GET /api/bins
    // ------------------------------------

    server.Get("/api/bins",
        [&database](const httplib::Request& req,
                    httplib::Response& res)
        {
            vector<Bin> bins;

            // Retrieve records from SQLite
            if (!database.loadBins(bins))
            {
                json error;

                error["success"] = false;
                error["message"] =
                    "Could not retrieve bins from database";

                res.status = 500;

                res.set_content(
                    error.dump(4),
                    "application/json"
                );

                return;
            }

            // Convert C++ bin objects into JSON
            json result = json::array();

            for (Bin& bin : bins)
            {
                json item;

                item["id"] = bin.getID();
                item["location"] = bin.getLocation();

                item["locationNode"] =
                    bin.getLocationNode();

                item["capacity"] =
                    bin.getMaxCapacityKg();

                item["fill"] =
                    bin.getFillPercentage();

                item["type"] =
                    bin.getGarbageType();

                result.push_back(item);
            }

            cout << "Bins requested: "
                 << bins.size() << endl;

            res.set_content(
                result.dump(4),
                "application/json"
            );
        }
    );

    // ------------------------------------
    // Validate incoming bin JSON
    // ------------------------------------

    auto parseBinPayload = [](const string& body) -> json
    {
        json data = json::parse(body, nullptr, false);

        if (data.is_discarded() || !data.is_object())
            return nullptr;

        if (!data.contains("location") ||
            !data.contains("locationNode") ||
            !data.contains("capacity") ||
            !data.contains("fill") ||
            !data.contains("type"))
            return nullptr;

        if (!data["location"].is_string() ||
            !data["locationNode"].is_number_integer() ||
            !data["capacity"].is_number() ||
            !data["fill"].is_number() ||
            !data["type"].is_string())
            return nullptr;

        string location = data["location"].get<string>();
        string type = data["type"].get<string>();

        int node = data["locationNode"].get<int>();
        double capacity = data["capacity"].get<double>();
        double fill = data["fill"].get<double>();

        if (location.empty() ||
            node < 0 || node > 3 ||
            capacity <= 0 ||
            fill < 0 || fill > 100)
            return nullptr;

        if (type != "Dry" &&
            type != "Wet" &&
            type != "Mixed")
            return nullptr;

        return data;
    };

    // ------------------------------------
    // API 3: Add a new bin
    // POST /api/bins
    // ------------------------------------

    server.Post("/api/bins",
        [&database, &parseBinPayload](
            const httplib::Request& req,
            httplib::Response& res)
        {
            json data = parseBinPayload(req.body);

            if (data.is_null() ||
                !data.contains("id") ||
                !data["id"].is_string() ||
                data["id"].get<string>().empty())
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Invalid bin details"})",
                    "application/json"
                );
                return;
            }

            bool success = database.addBin(
                data["id"].get<string>(),
                data["location"].get<string>(),
                data["locationNode"].get<int>(),
                data["capacity"].get<float>(),
                data["fill"].get<float>(),
                data["type"].get<string>()
            );

            json response;

            if (success)
            {
                response["success"] = true;
                response["message"] = "Bin added successfully";
                res.status = 201;
            }
            else
            {
                response["success"] = false;
                response["message"] =
                    "Could not add bin. ID may already exist.";
                res.status = 409;
            }

            res.set_content(response.dump(4), "application/json");
        }
    );

    // ------------------------------------
    // API 4: Update an existing bin
    // PUT /api/bins/B101
    // ------------------------------------

    server.Put(R"(/api/bins/([^/]+))",
        [&database, &parseBinPayload](
            const httplib::Request& req,
            httplib::Response& res)
        {
            string binID = req.matches[1].str();

            json data = parseBinPayload(req.body);

            if (data.is_null())
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Invalid bin details"})",
                    "application/json"
                );
                return;
            }

            bool success = database.updateBin(
                binID,
                data["location"].get<string>(),
                data["locationNode"].get<int>(),
                data["capacity"].get<float>(),
                data["fill"].get<float>(),
                data["type"].get<string>()
            );

            json response;

            response["success"] = success;
            response["message"] = success
                ? "Bin updated successfully"
                : "Bin not found or update failed";

            res.status = success ? 200 : 404;

            res.set_content(response.dump(4), "application/json");
        }
    );

    // ------------------------------------
    // API 5: Delete a bin
    // DELETE /api/bins/B101
    // ------------------------------------

    server.Delete(R"(/api/bins/([^/]+))",
        [&database](
            const httplib::Request& req,
            httplib::Response& res)
        {
            string binID = req.matches[1].str();

            bool success = database.deleteBin(binID);

            json response;

            response["success"] = success;

            response["message"] = success
                ? "Bin deleted successfully"
                : "Bin not found or has linked records";

            res.status = success ? 200 : 409;

            res.set_content(response.dump(4), "application/json");
        }
    );

// ------------------------------------
// Validate incoming truck JSON
// ------------------------------------

auto parseTruckPayload = [](const string& body) -> json
{
    json data = json::parse(body, nullptr, false);

    if (data.is_discarded() || !data.is_object())
        return nullptr;

    if (!data.contains("capacity") ||
        !data.contains("load") ||
        !data.contains("location") ||
        !data.contains("status"))
        return nullptr;

    if (!data["capacity"].is_number() ||
        !data["load"].is_number() ||
        !data["location"].is_number_integer() ||
        !data["status"].is_string())
        return nullptr;

    double capacity = data["capacity"].get<double>();
    double load = data["load"].get<double>();
    int location = data["location"].get<int>();
    string status = data["status"].get<string>();

    if (capacity <= 0 ||
        load < 0 ||
        load > capacity ||
        location < 0 ||
        location > 3)
        return nullptr;

    if (status != "Available" &&
        status != "Assigned" &&
        status != "NeedsDepot")
        return nullptr;

    return data;
};


// ------------------------------------
// API 6: Get all trucks
// GET /api/trucks
// ------------------------------------

server.Get("/api/trucks",
    [&database](const httplib::Request& req,
                httplib::Response& res)
    {
        vector<Truck> trucks;

        if (!database.loadTrucks(trucks))
        {
            res.status = 500;

            res.set_content(
                R"({"success":false,"message":"Could not load trucks"})",
                "application/json"
            );

            return;
        }

        json result = json::array();

        for (Truck& truck : trucks)
        {
            json item;

            item["id"] = truck.getID();
            item["capacity"] = truck.getMaxCapacity();
            item["load"] = truck.getCurrentLoad();
            item["location"] = truck.getCurrentLocation();
            item["status"] = truck.getStatus();

            result.push_back(item);
        }

        cout << "Trucks requested: "
             << trucks.size() << endl;

        res.set_content(
            result.dump(4),
            "application/json"
        );
    }
);


// ------------------------------------
// API 7: Add a new truck
// POST /api/trucks
// ------------------------------------

server.Post("/api/trucks",
    [&database, &parseTruckPayload](
        const httplib::Request& req,
        httplib::Response& res)
    {
        json data = parseTruckPayload(req.body);

        if (data.is_null() ||
            !data.contains("id") ||
            !data["id"].is_string() ||
            data["id"].get<string>().empty())
        {
            res.status = 400;

            res.set_content(
                R"({"success":false,"message":"Invalid truck details"})",
                "application/json"
            );

            return;
        }

        bool success = database.addTruck(
            data["id"].get<string>(),
            data["capacity"].get<float>(),
            data["load"].get<float>(),
            data["location"].get<int>(),
            data["status"].get<string>()
        );

        json response;

        response["success"] = success;

        response["message"] = success
            ? "Truck added successfully"
            : "Could not add truck. ID may already exist.";

        res.status = success ? 201 : 409;

        res.set_content(
            response.dump(4),
            "application/json"
        );
    }
);


// ------------------------------------
// API 8: Update an existing truck
// PUT /api/trucks/T01
// ------------------------------------

server.Put(R"(/api/trucks/([^/]+))",
    [&database, &parseTruckPayload](
        const httplib::Request& req,
        httplib::Response& res)
    {
        string truckID = req.matches[1].str();

        json data = parseTruckPayload(req.body);

        if (data.is_null())
        {
            res.status = 400;

            res.set_content(
                R"({"success":false,"message":"Invalid truck details"})",
                "application/json"
            );

            return;
        }

        bool success = database.updateTruck(
            truckID,
            data["capacity"].get<float>(),
            data["load"].get<float>(),
            data["location"].get<int>(),
            data["status"].get<string>()
        );

        json response;

        response["success"] = success;

        response["message"] = success
            ? "Truck updated successfully"
            : "Truck not found or update failed";

        res.status = success ? 200 : 404;

        res.set_content(
            response.dump(4),
            "application/json"
        );
    }
);


// ------------------------------------
// API 9: Delete a truck
// DELETE /api/trucks/T01
// ------------------------------------

server.Delete(R"(/api/trucks/([^/]+))",
    [&database](
        const httplib::Request& req,
        httplib::Response& res)
    {
        string truckID = req.matches[1].str();

        bool success = database.deleteTruck(truckID);

        json response;

        response["success"] = success;

        response["message"] = success
            ? "Truck deleted successfully"
            : "Truck not found or has linked records";

        res.status = success ? 200 : 409;

        res.set_content(
            response.dump(4),
            "application/json"
        );
    }
);

    // ------------------------------------
    // Convert D101 format into SQLite ID 101
    // ------------------------------------

    auto parseDriverID = [](const string& value, int& driverID)
    {
        if (value.size() < 2 || value[0] != 'D')
            return false;

        long long number = 0;

        for (size_t i = 1; i < value.size(); i++)
        {
            if (value[i] < '0' || value[i] > '9')
                return false;

            number = number * 10 + (value[i] - '0');

            if (number > 2147483647LL)
                return false;
        }

        if (number <= 0)
            return false;

        driverID = static_cast<int>(number);
        return true;
    };

    // ------------------------------------
    // Validate driver details
    // ------------------------------------

    auto parseDriverPayload = [](const string& body) -> json
    {
        json data = json::parse(body, nullptr, false);

        if (data.is_discarded() || !data.is_object())
            return nullptr;

        if (!data.contains("name") ||
            !data.contains("phone") ||
            !data.contains("truckId"))
            return nullptr;

        if (!data["name"].is_string() ||
            !data["phone"].is_string() ||
            !data["truckId"].is_string())
            return nullptr;

        string name = data["name"].get<string>();
        string phone = data["phone"].get<string>();

        if (name.empty() || phone.size() != 10)
            return nullptr;

        for (char digit : phone)
        {
            if (digit < '0' || digit > '9')
                return nullptr;
        }

        return data;
    };

    // ------------------------------------
    // API 10: Get all drivers
    // GET /api/drivers
    // ------------------------------------

    server.Get("/api/drivers",
        [&database](const httplib::Request& req,
                    httplib::Response& res)
        {
            vector<Driver> drivers;

            if (!database.loadDrivers(drivers))
            {
                res.status = 500;
                res.set_content(
                    R"({"success":false,"message":"Could not load drivers"})",
                    "application/json"
                );
                return;
            }

            json result = json::array();

            for (Driver& driver : drivers)
            {
                json item;

                item["id"] =
                    "D" + to_string(driver.getDriverID());

                item["name"] = driver.getName();
                item["phone"] = driver.getPhoneNumber();
                item["truckId"] = driver.getTruckID();

                result.push_back(item);
            }

            cout << "Drivers requested: "
                 << drivers.size() << endl;

            res.set_content(
                result.dump(4),
                "application/json"
            );
        }
    );

    // ------------------------------------
    // API 11: Add a new driver
    // POST /api/drivers
    // ------------------------------------

    server.Post("/api/drivers",
        [&database, &parseDriverID, &parseDriverPayload](
            const httplib::Request& req,
            httplib::Response& res)
        {
            json data = parseDriverPayload(req.body);

            int driverID = 0;

            if (data.is_null() ||
                !data.contains("id") ||
                !data["id"].is_string() ||
                !parseDriverID(
                    data["id"].get<string>(), driverID))
            {
                res.status = 400;

                res.set_content(
                    R"({"success":false,"message":"Invalid driver details or ID. Use D followed by numbers."})",
                    "application/json"
                );
                return;
            }

            bool success = database.addDriver(
                driverID,
                data["name"].get<string>(),
                data["phone"].get<string>(),
                data["truckId"].get<string>()
            );

            json response;

            response["success"] = success;

            response["message"] = success
                ? "Driver added successfully"
                : "Could not add driver. ID or truck may already be assigned.";

            res.status = success ? 201 : 409;

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );

    // ------------------------------------
    // API 12: Update a driver
    // PUT /api/drivers/D101
    // ------------------------------------

    server.Put(R"(/api/drivers/([^/]+))",
        [&database, &parseDriverID, &parseDriverPayload](
            const httplib::Request& req,
            httplib::Response& res)
        {
            int driverID = 0;

            if (!parseDriverID(req.matches[1].str(), driverID))
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Invalid driver ID"})",
                    "application/json"
                );
                return;
            }

            json data = parseDriverPayload(req.body);

            if (data.is_null())
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Invalid driver details"})",
                    "application/json"
                );
                return;
            }

            bool success = database.updateDriver(
                driverID,
                data["name"].get<string>(),
                data["phone"].get<string>(),
                data["truckId"].get<string>()
            );

            json response;

            response["success"] = success;

            response["message"] = success
                ? "Driver updated successfully"
                : "Driver not found or truck already assigned.";

            res.status = success ? 200 : 409;

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );

    // ------------------------------------
    // API 13: Delete a driver
    // DELETE /api/drivers/D101
    // ------------------------------------

    server.Delete(R"(/api/drivers/([^/]+))",
        [&database, &parseDriverID](
            const httplib::Request& req,
            httplib::Response& res)
        {
            int driverID = 0;

            if (!parseDriverID(req.matches[1].str(), driverID))
            {
                res.status = 400;
                res.set_content(
                    R"({"success":false,"message":"Invalid driver ID"})",
                    "application/json"
                );
                return;
            }

            bool success = database.deleteDriver(driverID);

            json response;

            response["success"] = success;

            response["message"] = success
                ? "Driver deleted successfully"
                : "Driver not found or truck has an active assignment.";

            res.status = success ? 200 : 409;

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );

    // ------------------------------------
    // Convert C001 format into SQLite ID 1
    // ------------------------------------

    auto parseComplaintID = [](const string& value, int& complaintID)
    {
        if (value.size() < 2 || value[0] != 'C')
            return false;

        long long number = 0;

        for (size_t i = 1; i < value.size(); i++)
        {
            if (value[i] < '0' || value[i] > '9')
                return false;

            number = number * 10 + (value[i] - '0');

            if (number > 2147483647LL)
                return false;
        }

        if (number <= 0)
            return false;

        complaintID = static_cast<int>(number);
        return true;
    };

    // ------------------------------------
    // Format SQLite ID as C001, C002...
    // ------------------------------------

    auto formatComplaintID = [](int id)
    {
        string number = to_string(id);

        while (number.size() < 3)
            number = "0" + number;

        return "C" + number;
    };

    // ------------------------------------
    // API 14: Get all complaints
    // GET /api/complaints
    // ------------------------------------

    server.Get("/api/complaints",
        [&database, &formatComplaintID](
            const httplib::Request& req,
            httplib::Response& res)
        {
            vector<ComplaintRecord> complaints;

            if (!database.loadComplaints(complaints))
            {
                res.status = 500;

                res.set_content(
                    R"({"success":false,"message":"Could not load complaints"})",
                    "application/json"
                );

                return;
            }

            json result = json::array();

            for (const ComplaintRecord& complaint : complaints)
            {
                json item;

                item["id"] =
                    formatComplaintID(complaint.complaintID);

                item["binId"] = complaint.binID;
                item["citizen"] = complaint.citizenName;
                item["phone"] = complaint.citizenPhone;
                item["issue"] = complaint.issueType;
                item["description"] = complaint.description;
                item["date"] = complaint.createdAt;
                item["status"] = complaint.status;

                result.push_back(item);
            }

            cout << "Complaints requested: "
                 << complaints.size() << endl;

            res.set_content(
                result.dump(4),
                "application/json"
            );
        }
    );


    // ------------------------------------
    // API 15: Add a citizen complaint
    // POST /api/complaints
    // ------------------------------------

    server.Post("/api/complaints",
        [&database](
            const httplib::Request& req,
            httplib::Response& res)
        {
            json data = json::parse(req.body, nullptr, false);

            if (data.is_discarded() ||
                !data.is_object() ||
                !data.contains("citizen") ||
                !data.contains("phone") ||
                !data.contains("issue") ||
                !data.contains("description"))
            {
                res.status = 400;

                res.set_content(
                    R"({"success":false,"message":"Invalid complaint details"})",
                    "application/json"
                );

                return;
            }

            if (!data["citizen"].is_string() ||
                !data["phone"].is_string() ||
                !data["issue"].is_string() ||
                !data["description"].is_string())
            {
                res.status = 400;

                res.set_content(
                    R"({"success":false,"message":"Complaint fields must contain text"})",
                    "application/json"
                );

                return;
            }

            string binID = "";

            if (data.contains("binId") && !data["binId"].is_null())
            {
                if (!data["binId"].is_string())
                {
                    res.status = 400;

                    res.set_content(
                        R"({"success":false,"message":"Invalid bin ID"})",
                        "application/json"
                    );

                    return;
                }

                binID = data["binId"].get<string>();
            }

            string citizen = data["citizen"].get<string>();
            string phone = data["phone"].get<string>();
            string issue = data["issue"].get<string>();
            string description = data["description"].get<string>();

            if (citizen.empty() ||
                description.empty() ||
                issue.empty() ||
                phone.size() != 10)
            {
                res.status = 400;

                res.set_content(
                    R"({"success":false,"message":"Please provide valid complaint details and a 10-digit phone number"})",
                    "application/json"
                );

                return;
            }

            bool success = database.addComplaint(
                binID,
                citizen,
                phone,
                issue,
                description
            );

            json response;

            response["success"] = success;

            response["message"] = success
                ? "Complaint submitted successfully"
                : "Could not submit complaint. Check the bin ID or existing records.";

            res.status = success ? 201 : 409;

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );


    // ------------------------------------
    // API 16: Update complaint status
    // PUT /api/complaints/C001
    // ------------------------------------

    server.Put(R"(/api/complaints/([^/]+))",
        [&database, &parseComplaintID](
            const httplib::Request& req,
            httplib::Response& res)
        {
            int complaintID = 0;

            if (!parseComplaintID(
                    req.matches[1].str(), complaintID))
            {
                res.status = 400;

                res.set_content(
                    R"({"success":false,"message":"Invalid complaint ID"})",
                    "application/json"
                );

                return;
            }

            json data = json::parse(req.body, nullptr, false);

            if (data.is_discarded() ||
                !data.is_object() ||
                !data.contains("status") ||
                !data["status"].is_string())
            {
                res.status = 400;

                res.set_content(
                    R"({"success":false,"message":"Invalid complaint status"})",
                    "application/json"
                );

                return;
            }

            string status = data["status"].get<string>();

            bool success = database.updateComplaintStatus(
                complaintID,
                status
            );

            json response;

            response["success"] = success;

            response["message"] = success
                ? "Complaint status updated successfully"
                : "Complaint not found or invalid status";

            res.status = success ? 200 : 404;

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );


    // ------------------------------------
    // API 17: Delete a complaint
    // DELETE /api/complaints/C001
    // ------------------------------------

    server.Delete(R"(/api/complaints/([^/]+))",
        [&database, &parseComplaintID](
            const httplib::Request& req,
            httplib::Response& res)
        {
            int complaintID = 0;

            if (!parseComplaintID(
                    req.matches[1].str(), complaintID))
            {
                res.status = 400;

                res.set_content(
                    R"({"success":false,"message":"Invalid complaint ID"})",
                    "application/json"
                );

                return;
            }

            bool success = database.deleteComplaint(complaintID);

            json response;

            response["success"] = success;

            response["message"] = success
                ? "Complaint deleted successfully"
                : "Complaint not found or deletion failed";

            res.status = success ? 200 : 404;

            res.set_content(
                response.dump(4),
                "application/json"
            );
        }
    );

    // ------------------------------------
    // API 18: Get collection history
    // GET /api/collections
    // ------------------------------------

    server.Get("/api/collections",
        [&database](const httplib::Request& req,
                    httplib::Response& res)
        {
            vector<CollectionHistoryRecord> records;

            if (!database.loadCollectionHistory(records))
            {
                res.status = 500;

                res.set_content(
                    R"({"success":false,"message":"Could not load collection history"})",
                    "application/json"
                );

                return;
            }

            json result = json::array();

            for (const CollectionHistoryRecord& record : records)
            {
                json item;

                item["id"] = record.collectionID;
                item["binId"] = record.binID;
                item["truckId"] = record.truckID;
                item["wasteCollected"] = record.garbageCollected;
                item["date"] = record.collectedAt;

                result.push_back(item);
            }

            cout << "Collection records requested: "
                 << records.size() << endl;

            res.set_content(
                result.dump(4),
                "application/json"
            );
        }
    );


    // Start server
    cout << "\nSmartWaste API started successfully!\n";
    cout << "Server address: http://localhost:8080\n";
    cout << "\nAvailable endpoints:\n";
    cout << "GET /api/health\n";
    cout << "GET /api/bins\n";
    cout << "GET /api/trucks\n";
    cout << "POST /api/trucks\n";
    cout << "PUT /api/trucks/{id}\n";
    cout << "DELETE /api/trucks/{id}\n";
    cout << "GET /api/drivers\n";
    cout << "POST /api/drivers\n";
    cout << "PUT /api/drivers/{id}\n";
    cout << "DELETE /api/drivers/{id}\n";
    cout << "GET /api/complaints\n";
    cout << "POST /api/complaints\n";
    cout << "PUT /api/complaints/{id}\n";
    cout << "DELETE /api/complaints/{id}\n";
    cout << "GET /api/collections\n";

    cout << "\nPress Ctrl + C to stop the server.\n";

    bool running = server.listen("0.0.0.0", 8080);

    database.close();

    return running ? 0 : 1;
}