#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>

#include "Bin.h"
#include "Truck.h"
#include "Driver.h"
struct ComplaintRecord
{
    int complaintID;
    string binID;
    string citizenName;
    string citizenPhone;
    string issueType;
    string description;
    string status;
    string createdAt;
};
struct CollectionHistoryRecord
{
    int collectionID;
    string binID;
    string truckID;
    float garbageCollected;
    string collectedAt;
};
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
    // Truck management operations for Admin API
bool addTruck(string truckID,
              float maxCapacity,
              float currentLoad,
              int currentLocation,
              string status);

bool updateTruck(string truckID,
                 float maxCapacity,
                 float currentLoad,
                 int currentLocation,
                 string status);

bool deleteTruck(string truckID);
    bool loadActiveAssignments(vector<Truck> &trucks);
    bool loadDrivers(vector<Driver> &drivers);
    // Complaint management operations
bool loadComplaints(vector<ComplaintRecord> &complaints);
bool loadCollectionHistory(
    vector<CollectionHistoryRecord> &records
);

bool updateComplaintStatus(int complaintID, string status);

bool deleteComplaint(int complaintID);

bool addComplaint(string binID,
                  string citizenName,
                  string citizenPhone,
                  string issueType,
                  string description);
    // Driver management operations for Admin API
bool addDriver(int driverID,
               string driverName,
               string phoneNumber,
               string truckID);

bool updateDriver(int driverID,
                  string driverName,
                  string phoneNumber,
                  string truckID);

bool deleteDriver(int driverID);
// Bin management operations for Admin API
bool addBin(string binID,
            string locationName,
            int locationNode,
            float capacityKg,
            float fillPercentage,
            string garbageType);

bool updateBin(string binID,
               string locationName,
               int locationNode,
               float capacityKg,
               float fillPercentage,
               string garbageType);

bool deleteBin(string binID);
    bool updateBinFill(string binID, float fill);

    bool updateTruckLoad(string truckID,
                         float load,
                         int location,
                         string status);

    bool recordAssignment(string binID,
                          string truckID,
                          int distance);

    bool recordCollection(string binID,
                          string truckID,
                          float garbage,
                          float newLoad,
                          int newLocation,
                          string newStatus);
};

#endif
