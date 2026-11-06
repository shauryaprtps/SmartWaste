#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>

#include "Bin.h"
#include "Truck.h"

using namespace std;

class Database
{
private:
    sqlite3 *db;

public:
    Database();

    ~Database();

    bool connect(string databasePath);

    void close();

    bool loadBins(vector<Bin> &bins);

    bool loadTrucks(vector<Truck> &trucks);

    bool updateBinFill(string binID, float fill);

    bool updateTruckLoad(string truckID,
                         float load,
                         int location,
                         string status);
};

#endif
