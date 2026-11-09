#include "Database.h"

Database::Database()
{
    db = NULL;
}

Database::~Database()
{
    close();
}

bool Database::connect(string databasePath)
{
    if (db != NULL)
        close();

    int result = sqlite3_open(databasePath.c_str(), &db);

    if (result != SQLITE_OK)
    {
        cout << "Database connection failed: "
             << sqlite3_errmsg(db) << endl;
        return false;
    }

    sqlite3_exec(db, "PRAGMA foreign_keys = ON;",
                 NULL, NULL, NULL);

    cout << "SQLite database connected successfully!\n";
    return true;
}

void Database::close()
{
    if (db != NULL)
    {
        sqlite3_close(db);
        db = NULL;
    }
}

bool Database::loadBins(vector<Bin> &bins)
{
    const char *sql =
        "SELECT bin_id, location_name, location_node, "
        "capacity_kg, fill_percentage, garbage_type "
        "FROM bins ORDER BY bin_id;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    bins.clear();

    int result;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        const unsigned char *idText =
            sqlite3_column_text(statement, 0);

        const unsigned char *locationText =
            sqlite3_column_text(statement, 1);

        const unsigned char *typeText =
            sqlite3_column_text(statement, 5);

        string id = idText ?
            reinterpret_cast<const char *>(idText) : "";

        string location = locationText ?
            reinterpret_cast<const char *>(locationText) : "";

        string type = typeText ?
            reinterpret_cast<const char *>(typeText) : "";

        int node = sqlite3_column_int(statement, 2);
        float capacity = sqlite3_column_double(statement, 3);
        float fill = sqlite3_column_double(statement, 4);

        bins.push_back(
            Bin(id, location, node, capacity, fill, type)
        );
    }

    sqlite3_finalize(statement);

    return result == SQLITE_DONE;
}

bool Database::loadTrucks(vector<Truck> &trucks)
{
    const char *sql =
        "SELECT truck_id, max_capacity, current_load, "
        "current_location, status FROM trucks ORDER BY truck_id;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    trucks.clear();

    int result;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        const unsigned char *idText =
            sqlite3_column_text(statement, 0);

        const unsigned char *statusText =
            sqlite3_column_text(statement, 4);

        string id = idText ?
            reinterpret_cast<const char *>(idText) : "";

        string status = statusText ?
            reinterpret_cast<const char *>(statusText) : "Available";

        float capacity = sqlite3_column_double(statement, 1);
        float load = sqlite3_column_double(statement, 2);
        int location = sqlite3_column_int(statement, 3);

        trucks.push_back(
            Truck(id, capacity, load, location, status)
        );
    }

    sqlite3_finalize(statement);

    return result == SQLITE_DONE;
}
bool Database::loadDrivers(vector<Driver> &drivers)
{
    const char *sql =
        "SELECT driver_id, driver_name, phone_number, truck_id "
        "FROM drivers ORDER BY driver_id;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    drivers.clear();

    int result;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(statement, 0);

        const unsigned char *nameText =
            sqlite3_column_text(statement, 1);

        const unsigned char *phoneText =
            sqlite3_column_text(statement, 2);

        const unsigned char *truckText =
            sqlite3_column_text(statement, 3);

        string name = nameText ?
            reinterpret_cast<const char *>(nameText) : "";

        string phone = phoneText ?
            reinterpret_cast<const char *>(phoneText) : "";

        string truck = truckText ?
            reinterpret_cast<const char *>(truckText) : "";

        drivers.push_back(
            Driver(id, name, phone, truck)
        );
    }

    sqlite3_finalize(statement);

    return result == SQLITE_DONE;
}

bool Database::updateBinFill(string binID, float fill)
{
    if (fill < 0 || fill > 100)
        return false;

    const char *sql =
        "UPDATE bins SET fill_percentage = ?, "
        "updated_at = CURRENT_TIMESTAMP WHERE bin_id = ?;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_double(statement, 1, fill);

    sqlite3_bind_text(statement, 2, binID.c_str(),
                      -1, SQLITE_TRANSIENT);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) > 0;

    sqlite3_finalize(statement);

    return success;
}

bool Database::updateTruckLoad(string truckID,
                               float load,
                               int location,
                               string status)
{
    if (load < 0)
        return false;

    const char *sql =
        "UPDATE trucks SET current_load = ?, "
        "current_location = ?, status = ? "
        "WHERE truck_id = ?;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_double(statement, 1, load);
    sqlite3_bind_int(statement, 2, location);

    sqlite3_bind_text(statement, 3, status.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(statement, 4, truckID.c_str(),
                      -1, SQLITE_TRANSIENT);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) > 0;

    sqlite3_finalize(statement);

    return success;
}

bool Database::recordAssignment(string binID,
                                string truckID,
                                int distance)
{
    // Start a transaction
    if (sqlite3_exec(db, "BEGIN IMMEDIATE;",
                     NULL, NULL, NULL) != SQLITE_OK)
        return false;

    bool success = true;
    sqlite3_stmt *statement = NULL;

    // Check whether this bin or truck already has an active assignment
    const char *checkSQL =
        "SELECT COUNT(*) FROM assignments "
        "WHERE status = 'Assigned' "
        "AND (bin_id = ? OR truck_id = ?);";

    if (sqlite3_prepare_v2(db, checkSQL, -1,
                           &statement, NULL) != SQLITE_OK)
    {
        success = false;
    }

    if (success)
    {
        sqlite3_bind_text(statement, 1, binID.c_str(),
                          -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 2, truckID.c_str(),
                          -1, SQLITE_TRANSIENT);

        if (sqlite3_step(statement) == SQLITE_ROW)
        {
            if (sqlite3_column_int(statement, 0) > 0)
                success = false;
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

    // Insert the assignment only if no conflict exists
    if (success)
    {
        const char *insertSQL =
            "INSERT INTO assignments "
            "(bin_id, truck_id, distance_km, status) "
            "VALUES (?, ?, ?, 'Assigned');";

        if (sqlite3_prepare_v2(db, insertSQL, -1,
                               &statement, NULL) != SQLITE_OK)
        {
            success = false;
        }
        else
        {
            sqlite3_bind_text(statement, 1, binID.c_str(),
                              -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(statement, 2, truckID.c_str(),
                              -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(statement, 3, distance);

            if (sqlite3_step(statement) != SQLITE_DONE)
                success = false;
        }

        if (statement != NULL)
        {
            sqlite3_finalize(statement);
            statement = NULL;
        }
    }

    // Update the truck status in the database
    if (success)
    {
        const char *updateSQL =
            "UPDATE trucks SET status = 'Assigned' "
            "WHERE truck_id = ?;";

        if (sqlite3_prepare_v2(db, updateSQL, -1,
                               &statement, NULL) != SQLITE_OK)
        {
            success = false;
        }
        else
        {
            sqlite3_bind_text(statement, 1, truckID.c_str(),
                              -1, SQLITE_TRANSIENT);

            if (sqlite3_step(statement) != SQLITE_DONE ||
                sqlite3_changes(db) != 1)
                success = false;
        }

        if (statement != NULL)
        {
            sqlite3_finalize(statement);
            statement = NULL;
        }
    }

    // Commit if everything succeeded; otherwise undo everything
    if (success)
    {
        if (sqlite3_exec(db, "COMMIT;",
                         NULL, NULL, NULL) != SQLITE_OK)
        {
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            return false;
        }
    }
    else
    {
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    }

    return success;
}

bool Database::recordCollection(string binID,
                                string truckID,
                                float garbage,
                                float newLoad,
                                int newLocation,
                                string newStatus)
{
    if (garbage <= 0 || newLoad < 0)
        return false;

    if (sqlite3_exec(db, "BEGIN IMMEDIATE;",
                     NULL, NULL, NULL) != SQLITE_OK)
        return false;

    bool success = true;
    sqlite3_stmt *statement = NULL;

    const char *historySQL =
        "INSERT INTO collection_history "
        "(bin_id, truck_id, garbage_collected) "
        "VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db, historySQL, -1,
                           &statement, NULL) != SQLITE_OK)
        success = false;

    if (success)
    {
        sqlite3_bind_text(statement, 1, binID.c_str(),
                          -1, SQLITE_TRANSIENT);

        sqlite3_bind_text(statement, 2, truckID.c_str(),
                          -1, SQLITE_TRANSIENT);

        sqlite3_bind_double(statement, 3, garbage);

        success = sqlite3_step(statement) == SQLITE_DONE;
    }

    if (statement != NULL)
        sqlite3_finalize(statement);

    statement = NULL;

    const char *binSQL =
        "UPDATE bins SET fill_percentage = 0, "
        "updated_at = CURRENT_TIMESTAMP WHERE bin_id = ?;";

    if (success &&
        sqlite3_prepare_v2(db, binSQL, -1,
                           &statement, NULL) == SQLITE_OK)
    {
        sqlite3_bind_text(statement, 1, binID.c_str(),
                          -1, SQLITE_TRANSIENT);

        success = sqlite3_step(statement) == SQLITE_DONE &&
                  sqlite3_changes(db) > 0;
    }
    else if (success)
    {
        success = false;
    }

    if (statement != NULL)
        sqlite3_finalize(statement);

    statement = NULL;

    const char *truckSQL =
        "UPDATE trucks SET current_load = ?, "
        "current_location = ?, status = ? "
        "WHERE truck_id = ?;";

    if (success &&
        sqlite3_prepare_v2(db, truckSQL, -1,
                           &statement, NULL) == SQLITE_OK)
    {
        sqlite3_bind_double(statement, 1, newLoad);
        sqlite3_bind_int(statement, 2, newLocation);

        sqlite3_bind_text(statement, 3, newStatus.c_str(),
                          -1, SQLITE_TRANSIENT);

        sqlite3_bind_text(statement, 4, truckID.c_str(),
                          -1, SQLITE_TRANSIENT);

        success = sqlite3_step(statement) == SQLITE_DONE &&
                  sqlite3_changes(db) > 0;
    }
    else if (success)
    {
        success = false;
    }

    if (statement != NULL)
        sqlite3_finalize(statement);

    statement = NULL;

    const char *assignmentSQL =
        "UPDATE assignments SET status = 'Completed' "
        "WHERE bin_id = ? AND truck_id = ? "
        "AND status = 'Assigned';";

    if (success &&
        sqlite3_prepare_v2(db, assignmentSQL, -1,
                           &statement, NULL) == SQLITE_OK)
    {
        sqlite3_bind_text(statement, 1, binID.c_str(),
                          -1, SQLITE_TRANSIENT);

        sqlite3_bind_text(statement, 2, truckID.c_str(),
                          -1, SQLITE_TRANSIENT);

        success = sqlite3_step(statement) == SQLITE_DONE &&
                  sqlite3_changes(db) > 0;
    }
    else if (success)
    {
        success = false;
    }

    if (statement != NULL)
        sqlite3_finalize(statement);

    if (success)
    {
        success = sqlite3_exec(db, "COMMIT;",
                               NULL, NULL, NULL) == SQLITE_OK;
    }

    if (!success)
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);

    return success;
}
bool Database::loadActiveAssignments(vector<Truck> &trucks)
{
    const char *sql =
        "SELECT truck_id, bin_id FROM assignments "
        "WHERE status = 'Assigned';";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    int result;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        const unsigned char *truckText =
            sqlite3_column_text(statement, 0);

        const unsigned char *binText =
            sqlite3_column_text(statement, 1);

        string truckID = truckText ?
            reinterpret_cast<const char *>(truckText) : "";

        string binID = binText ?
            reinterpret_cast<const char *>(binText) : "";

        bool found = false;

        for (int i = 0; i < trucks.size(); i++)
        {
            if (trucks[i].getID() == truckID)
            {
                if (!trucks[i].getAssignedBinID().empty())
                {
                    sqlite3_finalize(statement);
                    return false;
                }

                trucks[i].restoreAssignment(binID);
                found = true;
                break;
            }
        }

        if (!found)
        {
            sqlite3_finalize(statement);
            return false;
        }
    }

    sqlite3_finalize(statement);

    return result == SQLITE_DONE;
}

// Add a new driver to SQLite
bool Database::addDriver(int driverID,
                         string driverName,
                         string phoneNumber,
                         string truckID)
{
    if (db == NULL || driverID <= 0 ||
        driverName.empty() || phoneNumber.size() != 10)
        return false;

    for (char digit : phoneNumber)
    {
        if (digit < '0' || digit > '9')
            return false;
    }

    if (sqlite3_exec(db, "BEGIN IMMEDIATE;",
                     NULL, NULL, NULL) != SQLITE_OK)
        return false;

    bool success = true;
    sqlite3_stmt *statement = NULL;

    // Prevent assigning one truck to multiple drivers
    if (!truckID.empty())
    {
        const char *checkSQL =
            "SELECT COUNT(*) FROM drivers WHERE truck_id = ?;";

        if (sqlite3_prepare_v2(db, checkSQL, -1,
                               &statement, NULL) != SQLITE_OK)
            success = false;

        if (success)
        {
            sqlite3_bind_text(statement, 1, truckID.c_str(),
                              -1, SQLITE_TRANSIENT);

            if (sqlite3_step(statement) != SQLITE_ROW ||
                sqlite3_column_int(statement, 0) > 0)
                success = false;
        }

        if (statement != NULL)
        {
            sqlite3_finalize(statement);
            statement = NULL;
        }
    }

    // Insert driver
    if (success)
    {
        const char *sql =
            "INSERT INTO drivers "
            "(driver_id, driver_name, phone_number, truck_id) "
            "VALUES (?, ?, ?, ?);";

        if (sqlite3_prepare_v2(db, sql, -1,
                               &statement, NULL) != SQLITE_OK)
            success = false;
        else
        {
            sqlite3_bind_int(statement, 1, driverID);

            sqlite3_bind_text(statement, 2, driverName.c_str(),
                              -1, SQLITE_TRANSIENT);

            sqlite3_bind_text(statement, 3, phoneNumber.c_str(),
                              -1, SQLITE_TRANSIENT);

            if (truckID.empty())
                sqlite3_bind_null(statement, 4);
            else
                sqlite3_bind_text(statement, 4, truckID.c_str(),
                                  -1, SQLITE_TRANSIENT);

            success = sqlite3_step(statement) == SQLITE_DONE;
        }

        if (statement != NULL)
        {
            sqlite3_finalize(statement);
            statement = NULL;
        }
    }

    // Commit or rollback
    if (success)
    {
        success = sqlite3_exec(db, "COMMIT;",
                               NULL, NULL, NULL) == SQLITE_OK;
    }

    if (!success)
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);

    return success;
}


// Update an existing driver
bool Database::updateDriver(int driverID,
                            string driverName,
                            string phoneNumber,
                            string truckID)
{
    if (db == NULL || driverID <= 0 ||
        driverName.empty() || phoneNumber.size() != 10)
        return false;

    for (char digit : phoneNumber)
    {
        if (digit < '0' || digit > '9')
            return false;
    }

    if (sqlite3_exec(db, "BEGIN IMMEDIATE;",
                     NULL, NULL, NULL) != SQLITE_OK)
        return false;

    bool success = true;
    sqlite3_stmt *statement = NULL;

    // Check whether another driver already has this truck
    if (!truckID.empty())
    {
        const char *checkSQL =
            "SELECT COUNT(*) FROM drivers "
            "WHERE truck_id = ? AND driver_id != ?;";

        if (sqlite3_prepare_v2(db, checkSQL, -1,
                               &statement, NULL) != SQLITE_OK)
            success = false;

        if (success)
        {
            sqlite3_bind_text(statement, 1, truckID.c_str(),
                              -1, SQLITE_TRANSIENT);

            sqlite3_bind_int(statement, 2, driverID);

            if (sqlite3_step(statement) != SQLITE_ROW ||
                sqlite3_column_int(statement, 0) > 0)
                success = false;
        }

        if (statement != NULL)
        {
            sqlite3_finalize(statement);
            statement = NULL;
        }
    }

    // Update driver record
    if (success)
    {
        const char *sql =
            "UPDATE drivers SET "
            "driver_name = ?, "
            "phone_number = ?, "
            "truck_id = ? "
            "WHERE driver_id = ?;";

        if (sqlite3_prepare_v2(db, sql, -1,
                               &statement, NULL) != SQLITE_OK)
            success = false;
        else
        {
            sqlite3_bind_text(statement, 1, driverName.c_str(),
                              -1, SQLITE_TRANSIENT);

            sqlite3_bind_text(statement, 2, phoneNumber.c_str(),
                              -1, SQLITE_TRANSIENT);

            if (truckID.empty())
                sqlite3_bind_null(statement, 3);
            else
                sqlite3_bind_text(statement, 3, truckID.c_str(),
                                  -1, SQLITE_TRANSIENT);

            sqlite3_bind_int(statement, 4, driverID);

            success = sqlite3_step(statement) == SQLITE_DONE &&
                      sqlite3_changes(db) > 0;
        }

        if (statement != NULL)
        {
            sqlite3_finalize(statement);
            statement = NULL;
        }
    }

    // Commit or rollback
    if (success)
    {
        success = sqlite3_exec(db, "COMMIT;",
                               NULL, NULL, NULL) == SQLITE_OK;
    }

    if (!success)
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);

    return success;
}


// Delete a driver only when its truck has no active assignment
bool Database::deleteDriver(int driverID)
{
    if (db == NULL || driverID <= 0)
        return false;

    const char *sql =
        "DELETE FROM drivers "
        "WHERE driver_id = ? "
        "AND NOT EXISTS ("
        "   SELECT 1 FROM assignments "
        "   WHERE assignments.truck_id = drivers.truck_id "
        "   AND assignments.status = 'Assigned'"
        ");";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_int(statement, 1, driverID);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) > 0;

    sqlite3_finalize(statement);

    return success;
}

// Load all complaints from SQLite
bool Database::loadComplaints(
    vector<ComplaintRecord> &complaints)
{
    if (db == NULL)
        return false;

    const char *sql =
        "SELECT complaint_id, bin_id, citizen_name, "
        "citizen_phone, issue_type, description, "
        "status, created_at "
        "FROM complaints "
        "ORDER BY complaint_id DESC;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    complaints.clear();

    int result;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        ComplaintRecord complaint;

        complaint.complaintID =
            sqlite3_column_int(statement, 0);

        const unsigned char *bin =
            sqlite3_column_text(statement, 1);

        const unsigned char *name =
            sqlite3_column_text(statement, 2);

        const unsigned char *phone =
            sqlite3_column_text(statement, 3);

        const unsigned char *issue =
            sqlite3_column_text(statement, 4);

        const unsigned char *description =
            sqlite3_column_text(statement, 5);

        const unsigned char *status =
            sqlite3_column_text(statement, 6);

        const unsigned char *date =
            sqlite3_column_text(statement, 7);

        complaint.binID = bin
            ? reinterpret_cast<const char*>(bin) : "";

        complaint.citizenName = name
            ? reinterpret_cast<const char*>(name) : "";

        complaint.citizenPhone = phone
            ? reinterpret_cast<const char*>(phone) : "";

        complaint.issueType = issue
            ? reinterpret_cast<const char*>(issue) : "";

        complaint.description = description
            ? reinterpret_cast<const char*>(description) : "";

        complaint.status = status
            ? reinterpret_cast<const char*>(status) : "Pending";

        complaint.createdAt = date
            ? reinterpret_cast<const char*>(date) : "";

        complaints.push_back(complaint);
    }

    sqlite3_finalize(statement);

    return result == SQLITE_DONE;
}


// Add a new citizen complaint
bool Database::addComplaint(
    string binID,
    string citizenName,
    string citizenPhone,
    string issueType,
    string description)
{
    if (db == NULL ||
        citizenName.empty() ||
        issueType.empty() ||
        description.empty() ||
        citizenPhone.size() != 10)
        return false;

    // Validate phone number
    for (char digit : citizenPhone)
    {
        if (digit < '0' || digit > '9')
            return false;
    }

    // Verify that the supplied bin exists
    if (!binID.empty())
    {
        const char *checkSQL =
            "SELECT 1 FROM bins WHERE bin_id = ?;";

        sqlite3_stmt *checkStatement = NULL;

        if (sqlite3_prepare_v2(db, checkSQL, -1,
                               &checkStatement, NULL) != SQLITE_OK)
            return false;

        sqlite3_bind_text(checkStatement, 1, binID.c_str(),
                          -1, SQLITE_TRANSIENT);

        bool binExists =
            sqlite3_step(checkStatement) == SQLITE_ROW;

        sqlite3_finalize(checkStatement);

        if (!binExists)
            return false;
    }

    const char *sql =
        "INSERT INTO complaints "
        "(bin_id, description, status, citizen_name, "
        "citizen_phone, issue_type) "
        "VALUES (?, ?, 'Pending', ?, ?, ?);";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    if (binID.empty())
        sqlite3_bind_null(statement, 1);
    else
        sqlite3_bind_text(statement, 1, binID.c_str(),
                          -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(statement, 2, description.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(statement, 3, citizenName.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(statement, 4, citizenPhone.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(statement, 5, issueType.c_str(),
                      -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);

    return success;
}


// Update complaint status
bool Database::updateComplaintStatus(
    int complaintID,
    string status)
{
    if (db == NULL || complaintID <= 0)
        return false;

    if (status != "Pending" &&
        status != "In Progress" &&
        status != "Resolved")
        return false;

    const char *sql =
        "UPDATE complaints "
        "SET status = ? "
        "WHERE complaint_id = ?;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_text(statement, 1, status.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(statement, 2, complaintID);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) > 0;

    sqlite3_finalize(statement);

    return success;
}


// Delete a complaint
bool Database::deleteComplaint(int complaintID)
{
    if (db == NULL || complaintID <= 0)
        return false;

    const char *sql =
        "DELETE FROM complaints WHERE complaint_id = ?;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_int(statement, 1, complaintID);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) > 0;

    sqlite3_finalize(statement);

    return success;
}


// Load completed collection records from SQLite
bool Database::loadCollectionHistory(
    vector<CollectionHistoryRecord> &records)
{
    if (db == NULL)
        return false;

    const char *sql =
        "SELECT collection_id, bin_id, truck_id, "
        "garbage_collected, collected_at "
        "FROM collection_history "
        "ORDER BY collection_id DESC;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    records.clear();

    int result;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        CollectionHistoryRecord record;

        record.collectionID =
            sqlite3_column_int(statement, 0);

        const unsigned char *bin =
            sqlite3_column_text(statement, 1);

        const unsigned char *truck =
            sqlite3_column_text(statement, 2);

        record.garbageCollected =
            static_cast<float>(
                sqlite3_column_double(statement, 3)
            );

        const unsigned char *date =
            sqlite3_column_text(statement, 4);

        record.binID = bin
            ? reinterpret_cast<const char*>(bin) : "";

        record.truckID = truck
            ? reinterpret_cast<const char*>(truck) : "";

        record.collectedAt = date
            ? reinterpret_cast<const char*>(date) : "";

        records.push_back(record);
    }

    sqlite3_finalize(statement);

    return result == SQLITE_DONE;
}

// Add a new truck to SQLite
bool Database::addTruck(string truckID,
                         float maxCapacity,
                         float currentLoad,
                         int currentLocation,
                         string status)
{
    if (db == NULL || truckID.empty() ||
        maxCapacity <= 0 ||
        currentLoad < 0 ||
        currentLoad > maxCapacity ||
        currentLocation < 0 || currentLocation > 3)
        return false;

    if (status != "Available" &&
        status != "Assigned" &&
        status != "NeedsDepot")
        return false;

    // A truck at 90% capacity must go for unloading
    if (currentLoad >= maxCapacity * 0.90f)
        status = "NeedsDepot";

    const char *sql =
        "INSERT INTO trucks "
        "(truck_id, max_capacity, current_load, "
        "current_location, status) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_text(statement, 1, truckID.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_double(statement, 2, maxCapacity);
    sqlite3_bind_double(statement, 3, currentLoad);
    sqlite3_bind_int(statement, 4, currentLocation);

    sqlite3_bind_text(statement, 5, status.c_str(),
                      -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);

    return success;
}


// Update an existing truck
bool Database::updateTruck(string truckID,
                           float maxCapacity,
                           float currentLoad,
                           int currentLocation,
                           string status)
{
    if (db == NULL || truckID.empty() ||
        maxCapacity <= 0 ||
        currentLoad < 0 ||
        currentLoad > maxCapacity ||
        currentLocation < 0 || currentLocation > 3)
        return false;

    if (status != "Available" &&
        status != "Assigned" &&
        status != "NeedsDepot")
        return false;

    // Automatically apply the unloading threshold
    if (currentLoad >= maxCapacity * 0.90f)
        status = "NeedsDepot";

    const char *sql =
        "UPDATE trucks SET "
        "max_capacity = ?, "
        "current_load = ?, "
        "current_location = ?, "
        "status = ? "
        "WHERE truck_id = ?;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_double(statement, 1, maxCapacity);
    sqlite3_bind_double(statement, 2, currentLoad);
    sqlite3_bind_int(statement, 3, currentLocation);

    sqlite3_bind_text(statement, 4, status.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(statement, 5, truckID.c_str(),
                      -1, SQLITE_TRANSIENT);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) > 0;

    sqlite3_finalize(statement);

    return success;
}


// Delete a truck only when it has no linked records
bool Database::deleteTruck(string truckID)
{
    if (db == NULL || truckID.empty())
        return false;

    const char *sql =
        "DELETE FROM trucks "
        "WHERE truck_id = ? "
        "AND NOT EXISTS ("
        "   SELECT 1 FROM drivers "
        "   WHERE drivers.truck_id = trucks.truck_id"
        ") "
        "AND NOT EXISTS ("
        "   SELECT 1 FROM assignments "
        "   WHERE assignments.truck_id = trucks.truck_id"
        ") "
        "AND NOT EXISTS ("
        "   SELECT 1 FROM collection_history "
        "   WHERE collection_history.truck_id = trucks.truck_id"
        ");";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_text(statement, 1, truckID.c_str(),
                      -1, SQLITE_TRANSIENT);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) > 0;

    sqlite3_finalize(statement);

    return success;
}

// Add a new bin to SQLite
bool Database::addBin(string binID,
                      string locationName,
                      int locationNode,
                      float capacityKg,
                      float fillPercentage,
                      string garbageType)
{
    if (db == NULL || binID.empty() ||
        locationName.empty() || garbageType.empty() ||
        locationNode < 0 || locationNode > 3 ||
        capacityKg <= 0 ||
        fillPercentage < 0 || fillPercentage > 100)
        return false;

    const char *sql =
        "INSERT INTO bins "
        "(bin_id, location_name, location_node, "
        "capacity_kg, fill_percentage, garbage_type) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_text(statement, 1, binID.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(statement, 2, locationName.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(statement, 3, locationNode);
    sqlite3_bind_double(statement, 4, capacityKg);
    sqlite3_bind_double(statement, 5, fillPercentage);

    sqlite3_bind_text(statement, 6, garbageType.c_str(),
                      -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);

    return success;
}


// Update an existing bin
bool Database::updateBin(string binID,
                         string locationName,
                         int locationNode,
                         float capacityKg,
                         float fillPercentage,
                         string garbageType)
{
    if (db == NULL || binID.empty() ||
        locationName.empty() || garbageType.empty() ||
        locationNode < 0 || locationNode > 3 ||
        capacityKg <= 0 ||
        fillPercentage < 0 || fillPercentage > 100)
        return false;

    const char *sql =
        "UPDATE bins SET "
        "location_name = ?, "
        "location_node = ?, "
        "capacity_kg = ?, "
        "fill_percentage = ?, "
        "garbage_type = ?, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE bin_id = ?;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_text(statement, 1, locationName.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(statement, 2, locationNode);
    sqlite3_bind_double(statement, 3, capacityKg);
    sqlite3_bind_double(statement, 4, fillPercentage);

    sqlite3_bind_text(statement, 5, garbageType.c_str(),
                      -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(statement, 6, binID.c_str(),
                      -1, SQLITE_TRANSIENT);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) > 0;

    sqlite3_finalize(statement);

    return success;
}


// Delete a bin only when it has no associated records
bool Database::deleteBin(string binID)
{
    if (db == NULL || binID.empty())
        return false;

    const char *sql =
        "DELETE FROM bins "
        "WHERE bin_id = ? "
        "AND NOT EXISTS ("
        "   SELECT 1 FROM assignments "
        "   WHERE assignments.bin_id = bins.bin_id"
        ") "
        "AND NOT EXISTS ("
        "   SELECT 1 FROM collection_history "
        "   WHERE collection_history.bin_id = bins.bin_id"
        ") "
        "AND NOT EXISTS ("
        "   SELECT 1 FROM complaints "
        "   WHERE complaints.bin_id = bins.bin_id"
        ");";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_text(statement, 1, binID.c_str(),
                      -1, SQLITE_TRANSIENT);

    bool success =
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db) > 0;

    sqlite3_finalize(statement);

    return success;
}
