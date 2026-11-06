#ifndef COLLECTION_MANAGER_H
#define COLLECTION_MANAGER_H

#include <iostream>
#include <vector>
#include <string>
#include <cstdio>

#include "Bin.h"
#include "Truck.h"
#include "Depot.h"

// Connect C functions with C++
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
    Graph *roadGraph;

public:
    CollectionManager(Graph *graph)
    {
        roadGraph = graph;
    }

    void addBin(Bin bin)
    {
        bins.push_back(bin);
    }

    void addTruck(Truck truck)
    {
        trucks.push_back(truck);
    }

    void addDepot(Depot depot)
    {
        depots.push_back(depot);
    }

    void displayBins()
    {
        cout << "\n--- BIN INFORMATION ---\n";

        for (int i = 0; i < bins.size(); i++)
        {
            bins[i].displayBin();
        }
    }

    void displayTrucks()
    {
        cout << "\n--- TRUCK INFORMATION ---\n";

        for (int i = 0; i < trucks.size(); i++)
        {
            trucks[i].displayTruck();
        }
    }

    void optimizeCollection()
    {
        MaxHeap heap;
        initializeHeap(&heap);

        // Insert bins into the C Max Heap
        for (int i = 0; i < bins.size(); i++)
        {
            HeapBin item;

            snprintf(item.binID, sizeof(item.binID),
                     "%s", bins[i].getID().c_str());

            item.fillPercentage =
                bins[i].getFillPercentage();

            insertBin(&heap, item);
        }

        cout << "\n--- SMART COLLECTION OPTIMIZATION ---\n";

        // Process bins in priority order
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
            {
                continue;
            }

            Bin &bin = bins[binIndex];

            float requiredGarbage =
                bin.getEstimatedGarbage();

            int bestTruck = -1;
            int shortestDistance = 1000000;

            // Find the nearest eligible truck
            for (int i = 0; i < trucks.size(); i++)
            {
                if (!trucks[i].canCollect(requiredGarbage) ||
                    trucks[i].getStatus() != "Available")
                {
                    continue;
                }

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

            // Assign the nearest suitable truck
            if (bestTruck != -1)
            {
                trucks[bestTruck].assignBin(
                    bin.getID(),
                    requiredGarbage
                );

                cout << "\nSelected Bin: "
                     << bin.getID()
                     << " (" << bin.getFillPercentage()
                     << "% full)\n";

                cout << "Assigned Truck: "
                     << trucks[bestTruck].getID()
                     << endl;

                cout << "Estimated Garbage: "
                     << requiredGarbage << " kg\n";

                cout << "Shortest Distance: "
                     << shortestDistance << " km\n";
            }
            else
            {
                cout << "\nNo suitable truck available for "
                     << bin.getID() << endl;
            }
        }
    }

    void completeCollection(string binID)
    {
        for (int i = 0; i < trucks.size(); i++)
        {
            if (trucks[i].getAssignedBinID() == binID)
            {
                for (int j = 0; j < bins.size(); j++)
                {
                    if (bins[j].getID() == binID)
                    {
                        float garbage =
                            bins[j].getEstimatedGarbage();

                        bool success = trucks[i].completeCollection(
                            binID,
                            garbage,
                            bins[j].getLocationNode()
                        );

                        if (success)
                        {
                            bins[j].collectGarbage();

                            cout << "\nCollection Completed!\n";
                            cout << "Bin: " << binID << endl;
                            cout << "Truck: "
                                 << trucks[i].getID() << endl;
                            cout << "Garbage Collected: "
                                 << garbage << " kg\n";
                        }

                        return;
                    }
                }
            }
        }

        cout << "No matching assignment found!\n";
    }

    void displayDepots()
    {
        cout << "\n--- DEPOT INFORMATION ---\n";

        for (int i = 0; i < depots.size(); i++)
        {
            depots[i].displayDepot();
        }
    }
};

#endif