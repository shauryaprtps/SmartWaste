#ifndef TRUCK_H
#define TRUCK_H

#include <iostream>
#include <string>
using namespace std;

class Truck
{
private:
    string truckID;
    float maxCapacity;
    float currentLoad;
    int currentLocation;
    string status;
    string assignedBinID;

public:
    Truck(string id, float capacity, float load,
          int location, string truckStatus = "Available")
    {
        truckID = id;
        maxCapacity = capacity;
        currentLoad = load;
        currentLocation = location;
        status = truckStatus;
        assignedBinID = "";
    }

    string getID() { return truckID; }
    float getRemainingCapacity() { return maxCapacity - currentLoad; }
    float getCurrentLoad() { return currentLoad; }
    float getMaxCapacity() { return maxCapacity; }
    int getCurrentLocation() { return currentLocation; }
    string getStatus() { return status; }
    string getAssignedBinID() { return assignedBinID; }

    bool canCollect(float garbage)
    {
        return garbage > 0 &&
               getRemainingCapacity() >= garbage;
    }

    bool assignBin(string binID, float garbage)
    {
        if (status != "Available" || !canCollect(garbage))
            return false;

        assignedBinID = binID;
        status = "Assigned";
        return true;
    }
    void restoreAssignment(string binID)
{
    assignedBinID = binID;
    status = "Assigned";
}

    bool completeCollection(string binID, float garbage,
                            int binLocation)
    {
        if (assignedBinID != binID || !canCollect(garbage))
            return false;

        currentLoad += garbage;
        currentLocation = binLocation;
        assignedBinID = "";

        if (currentLoad >= maxCapacity * 0.90f)
            status = "NeedsDepot";
        else
            status = "Available";

        return true;
    }

    void unloadGarbage()
    {
        currentLoad = 0;
        assignedBinID = "";
        status = "Available";
    }

    void setLocation(int location)
    {
        currentLocation = location;
    }

    void displayTruck()
    {
        cout << truckID
             << " | Load: " << currentLoad
             << "/" << maxCapacity
             << " kg | Remaining: " << getRemainingCapacity()
             << " kg | Status: " << status << endl;
    }
};

#endif
