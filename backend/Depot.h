#ifndef DEPOT_H
#define DEPOT_H

#include <iostream>
#include <string>
using namespace std;

class Depot
{
private:
    string depotID;
    string depotName;
    int locationNode;

public:
    Depot(string id, string name, int location)
    {
        depotID = id;
        depotName = name;
        locationNode = location;
    }

    string getID()
    {
        return depotID;
    }

    string getName()
    {
        return depotName;
    }

    int getLocationNode()
    {
        return locationNode;
    }

    void displayDepot()
    {
        cout << depotID << " | "
             << depotName << " | Location: "
             << locationNode << endl;
    }
};

#endif