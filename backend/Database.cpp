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
    int result = sqlite3_open(databasePath.c_str(), &db);

    if (result != SQLITE_OK)
    {
        cout << "Database connection failed: "
             << sqlite3_errmsg(db) << endl;

        return false;
    }

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

    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
    {
        cout << "Could not read bins: "
             << sqlite3_errmsg(db) << endl;
        return false;
    }

    bins.clear();

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        string id = reinterpret_cast<const char *>(
            sqlite3_column_text(statement, 0));

        string location = reinterpret_cast<const char *>(
            sqlite3_column_text(statement, 1));

        int node = sqlite3_column_int(statement, 2);

        float capacity = sqlite3_column_double(statement, 3);

        float fill = sqlite3_column_double(statement, 4);

        const unsigned char *typeText =
            sqlite3_column_text(statement, 5);

        string type = typeText
            ? reinterpret_cast<const char *>(typeText)
            : "";

        Bin bin(id, location, node, capacity, fill, type);

        bins.push_back(bin);
    }

    sqlite3_finalize(statement);

    return true;
}

bool Database::loadTrucks(vector<Truck> &trucks)
{
    const char *sql =
        "SELECT truck_id, max_capacity, current_load, "
        "current_location FROM trucks ORDER BY truck_id;";

    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
    {
        cout << "Could not read trucks: "
             << sqlite3_errmsg(db) << endl;
        return false;
    }

    trucks.clear();

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        string id = reinterpret_cast<const char *>(
            sqlite3_column_text(statement, 0));

        float capacity = sqlite3_column_double(statement, 1);

        float load = sqlite3_column_double(statement, 2);

        int location = sqlite3_column_int(statement, 3);

        Truck truck(id, capacity, load, location);

        trucks.push_back(truck);
    }

    sqlite3_finalize(statement);

    return true;
}

bool Database::updateBinFill(string binID, float fill)
{
    if (fill < 0 || fill > 100)
    {
        cout << "Invalid bin fill percentage!\n";
        return false;
    }

    const char *sql =
        "UPDATE bins SET fill_percentage = ?, "
        "updated_at = CURRENT_TIMESTAMP WHERE bin_id = ?;";

    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
    {
        return false;
    }

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
    {
        cout << "Invalid truck load!\n";
        return false;
    }

    const char *sql =
        "UPDATE trucks SET current_load = ?, "
        "current_location = ?, status = ? "
        "WHERE truck_id = ?;";

    sqlite3_stmt *statement;

    if (sqlite3_prepare_v2(db, sql, -1,
                           &statement, NULL) != SQLITE_OK)
    {
        return false;
    }

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
