#ifndef COLLECTION_MANAGER_H
#define COLLECTION_MANAGER_H

#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <climits>

#include "Bin.h"
#include "Truck.h"
#include "Depot.h"
#include "Database.h"
#include "Driver.h"

extern "C"
{
    #include "../dsa/heap.h"
    #include "../dsa/graph.h"
    #include "../dsa/dijkstra.h"
}

using namespace std;

class CollectionManager
{
private:
    vector<Bin> bins;
    vector<Truck> trucks;
    vector<Depot> depots;
    vector<Driver> drivers;

    Graph *roadGraph;
    Database *database;

public:
    CollectionManager(Graph *graph, Database *db)
    {
        roadGraph = graph;
        database = db;
    }

    bool loadData()
    {
        if (!database->loadBins(bins))
        {
            cout << "Could not load bins from database!\n";
            return false;
        }

        if (!database->loadTrucks(trucks))
        {
            cout << "Could not load trucks from database!\n";
            return false;
        }
        if (!database->loadDrivers(drivers))
{
    cout << "Could not load drivers from database!\n";
    return false;
}
        if (!database->loadActiveAssignments(trucks))
{
    cout << "Could not restore assignments!\n";
    return false;
}

        cout << "\nDatabase records loaded successfully!\n";
        cout << "Bins: " << bins.size() << endl;
        cout << "Trucks: " << trucks.size() << endl;
        cout << "Drivers: " << drivers.size() << endl;

        return true;
    }

    void addDepot(Depot depot)
    {
        depots.push_back(depot);
    }

    void displayBins()
    {
        cout << "\n--- BIN INFORMATION ---\n";

        for (int i = 0; i < bins.size(); i++)
            bins[i].displayBin();
    }

    void displayTrucks()
    {
        cout << "\n--- TRUCK INFORMATION ---\n";

        for (int i = 0; i < trucks.size(); i++)
            trucks[i].displayTruck();
    }

    void optimizeCollection()
    {
        MaxHeap heap;
        initializeHeap(&heap);

        // Insert database bins into C Max Heap
        for (int i = 0; i < bins.size(); i++)
        {
            if (bins[i].getFillPercentage() <= 0)
                continue;

            HeapBin item;

            snprintf(item.binID, sizeof(item.binID),
                     "%s", bins[i].getID().c_str());

            item.fillPercentage =
                bins[i].getFillPercentage();

            insertBin(&heap, item);
        }

        cout << "\n--- SMART COLLECTION OPTIMIZATION ---\n";

        while (!isEmpty(&heap))
        {
            HeapBin selected = extractMax(&heap);

            int binIndex = -1;

            for (int i = 0; i < bins.size(); i++)
            {
                if (bins[i].getID() == selected.binID)
                {
                    binIndex = i;
                    break;
                }
            }

            if (binIndex == -1)
                continue;

            Bin &bin = bins[binIndex];

// Skip bins that are already assigned
bool alreadyAssigned = false;

for (int j = 0; j < trucks.size(); j++)
{
    if (trucks[j].getAssignedBinID() == bin.getID())
    {
        alreadyAssigned = true;
        break;
    }
}

if (alreadyAssigned)
{
    cout << "\nSkipping " << bin.getID()
         << " - Already assigned to a truck.\n";
    continue;
}

float requiredGarbage =
    bin.getEstimatedGarbage();

            int bestTruck = -1;
            int shortestDistance = 1000000;

            // Find nearest eligible truck using Dijkstra
            for (int i = 0; i < trucks.size(); i++)
            {
                if (trucks[i].getStatus() != "Available" ||
                    !trucks[i].canCollect(requiredGarbage))
                    continue;

                int path[MAX_NODES];
                int pathLength;
                int distance;

                int found = shortestPath(
                    roadGraph,
                    trucks[i].getCurrentLocation(),
                    bin.getLocationNode(),
                    path,
                    &pathLength,
                    &distance
                );

                if (found && distance < shortestDistance)
                {
                    shortestDistance = distance;
                    bestTruck = i;
                }
            }

            if (bestTruck != -1)
{
    bool saved = database->recordAssignment(
        bin.getID(),
        trucks[bestTruck].getID(),
        shortestDistance
    );

    if (!saved)
    {
        cout << "Assignment could not be saved!\n";
        continue;
    }

    bool assigned = trucks[bestTruck].assignBin(
        bin.getID(), requiredGarbage
    );

    if (!assigned)
    {
        cout << "Truck assignment failed!\n";
        continue;
    }

    cout << "\nSelected Bin: " << bin.getID()
         << " (" << bin.getFillPercentage()
         << "% full)\n";

    cout << "Assigned Truck: "
         << trucks[bestTruck].getID() << endl;

    cout << "Estimated Garbage: "
         << requiredGarbage << " kg\n";

    cout << "Shortest Distance: "
         << shortestDistance << " km\n";
}
        }
    }

    void completeCollection(string binID)
    {
        for (int i = 0; i < trucks.size(); i++)
        {
            if (trucks[i].getAssignedBinID() != binID)
                continue;

            for (int j = 0; j < bins.size(); j++)
            {
                if (bins[j].getID() != binID)
                    continue;

                float garbage = bins[j].getEstimatedGarbage();

                float newLoad =
                    trucks[i].getCurrentLoad() + garbage;

                string newStatus =
                    (newLoad >= trucks[i].getMaxCapacity() * 0.90f)
                    ? "NeedsDepot" : "Available";

                // Save collection to SQLite first
                bool saved = database->recordCollection(
                    binID,
                    trucks[i].getID(),
                    garbage,
                    newLoad,
                    bins[j].getLocationNode(),
                    newStatus
                );

                if (!saved)
                {
                    cout << "Database update failed! "
                         << "Collection not completed.\n";
                    return;
                }

                // Update C++ objects
                bool success = trucks[i].completeCollection(
                    binID,
                    garbage,
                    bins[j].getLocationNode()
                );

               if (!success)
{
    cout << "Collection failed in truck system!\n";
    return;
}

bins[j].collectGarbage();

cout << "\nCollection Completed!\n";
cout << "Bin: " << binID << endl;
cout << "Truck: " << trucks[i].getID() << endl;
cout << "Garbage Collected: " << garbage << " kg\n";
cout << "Database updated successfully!\n";

if (trucks[i].getStatus() == "NeedsDepot")
{
    sendTruckToNearestDepot(i);
}


                return;
            }
        }

        cout << "No matching assignment found!\n";
    }
void sendTruckToNearestDepot(int truckIndex)
{
    if (truckIndex < 0 || truckIndex >= trucks.size())
        return;

    if (trucks[truckIndex].getStatus() != "NeedsDepot")
        return;

    int nearestDepot = -1;
    int minimumDistance = INT_MAX;

    int bestPath[MAX_NODES];
    int bestPathLength = 0;

    for (int i = 0; i < depots.size(); i++)
    {
        int path[MAX_NODES];
        int pathLength = 0;
        int distance = INT_MAX;

        int reachable = shortestPath(
            roadGraph,
            trucks[truckIndex].getCurrentLocation(),
            depots[i].getLocationNode(),
            path,
            &pathLength,
            &distance
        );

        if (reachable && distance < minimumDistance)
        {
            minimumDistance = distance;
            nearestDepot = i;

            bestPathLength = pathLength;

            for (int j = 0; j < pathLength; j++)
                bestPath[j] = path[j];
        }
    }

    if (nearestDepot == -1)
    {
        cout << "No reachable depot found for "
             << trucks[truckIndex].getID() << endl;
        return;
    }

    cout << "\n--- TRUCK RETURNING TO DEPOT ---\n";

    cout << "Truck: "
         << trucks[truckIndex].getID() << endl;

    cout << "Nearest Depot: "
         << depots[nearestDepot].getName() << endl;

    cout << "Shortest Distance: "
         << minimumDistance << " km" << endl;

    cout << "Route: ";

    for (int i = 0; i < bestPathLength; i++)
    {
        cout << bestPath[i];

        if (i < bestPathLength - 1)
            cout << " -> ";
    }

    cout << endl;

    int depotLocation =
        depots[nearestDepot].getLocationNode();

    bool saved = database->updateTruckLoad(
        trucks[truckIndex].getID(),
        0,
        depotLocation,
        "Available"
    );

    if (!saved)
    {
        cout << "Database update failed! "
             << "Truck was not unloaded.\n";
        return;
    }

    trucks[truckIndex].setLocation(depotLocation);
    trucks[truckIndex].unloadGarbage();

    cout << "Garbage unloaded successfully!\n";
    cout << "Truck Load: 0 kg\n";
    cout << "Truck Status: Available\n";
}
void displayDriverDetails(int driverID)
{
    for (int i = 0; i < drivers.size(); i++)
    {
        if (drivers[i].getDriverID() == driverID)
        {
            cout << "\n--- DRIVER DETAILS ---\n";

            drivers[i].displayDriver();

            string assignedTruck = drivers[i].getTruckID();

            for (int j = 0; j < trucks.size(); j++)
            {
                if (trucks[j].getID() == assignedTruck)
                {
                    cout << "\n--- ASSIGNED TRUCK ---\n";
                    trucks[j].displayTruck();

                    cout << "\n--- ASSIGNED BIN ---\n";

                    if (trucks[j].getAssignedBinID() == "")
                        cout << "No bin currently assigned.\n";
                    else
                        cout << "Bin ID: "
                             << trucks[j].getAssignedBinID() << endl;

                    return;
                }
            }

            cout << "No truck linked to this driver.\n";
            return;
        }
    }

    cout << "Driver not found!\n";
}
    void displayDepots()
    {
        cout << "\n--- DEPOT INFORMATION ---\n";

        for (int i = 0; i < depots.size(); i++)
            depots[i].displayDepot();
    }
};

#endif
