#ifndef BIN_H
#define BIN_H

#include <iostream>
#include <string>
using namespace std;

class Bin
{
private:
    string binID;
    string locationName;
    int locationNode;
    float maxCapacityKg;
    float fillPercentage;
    string garbageType;

public:
    Bin(string id, string location, int node,
        float capacity, float fill, string type)
    {
        binID = id;
        locationName = location;
        locationNode = node;
        maxCapacityKg = capacity;
        fillPercentage = fill;
        garbageType = type;
    }

    string getID()
    {
        return binID;
    }

    string getLocation()
    {
        return locationName;
    }

    int getLocationNode()
    {
        return locationNode;
    }

    float getFillPercentage()
    {
        return fillPercentage;
    }

    float getEstimatedGarbage()
    {
        return maxCapacityKg * fillPercentage / 100.0f;
    }

    string getGarbageType()
    {
        return garbageType;
    }

    void updateFill(float fill)
    {
        if (fill >= 0 && fill <= 100)
        {
            fillPercentage = fill;
        }
    }

    void collectGarbage()
    {
        fillPercentage = 0;
    }

    void displayBin()
    {
        cout << binID << " | "
             << locationName << " | "
             << fillPercentage << "% | "
             << getEstimatedGarbage() << " kg"
             << endl;
    }
};

#endif