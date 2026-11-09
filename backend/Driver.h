#ifndef DRIVER_H
#define DRIVER_H

#include <iostream>
#include <string>
using namespace std;

class Driver
{
private:
    int driverID;
    string driverName;
    string phoneNumber;
    string truckID;

public:
    Driver(int id, string name,
           string phone, string truck)
    {
        driverID = id;
        driverName = name;
        phoneNumber = phone;
        truckID = truck;
    }

    int getDriverID()
    {
        return driverID;
    }

    string getName()
    {
        return driverName;
    }
    string getPhoneNumber()
    {
    return phoneNumber;
    }

    string getTruckID()
    {
        return truckID;
    }

    void displayDriver()
    {
        cout << "Driver ID: " << driverID
             << " | Name: " << driverName
             << " | Truck: " << truckID
             << endl;
    }
};

#endif