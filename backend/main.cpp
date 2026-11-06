#include <iostream>
#include <vector>

#include "Bin.h"
#include "Truck.h"
#include "Driver.h"
#include "Depot.h"
#include "CollectionManager.h"

extern "C"
{
    #include "../dsa/graph.h"
}

using namespace std;

int main()
{
    cout << "====================================\n";
    cout << "       SMARTWASTE MANAGEMENT\n";
    cout << "====================================\n";

    // Create road network
    Graph graph;

    initializeGraph(&graph, 4);

    addEdge(&graph, 0, 1, 4);
    addEdge(&graph, 0, 2, 2);
    addEdge(&graph, 1, 2, 5);
    addEdge(&graph, 1, 3, 3);
    addEdge(&graph, 2, 3, 1);

    // Create collection manager
    CollectionManager manager(&graph);

    // Create bins
    manager.addBin(
        Bin("B101", "Sector 1", 0, 200, 45, "Dry")
    );

    manager.addBin(
        Bin("B102", "Sector 2", 1, 300, 95, "Wet")
    );

    manager.addBin(
        Bin("B103", "Sector 3", 2, 250, 80, "Dry")
    );

    manager.addBin(
        Bin("B104", "Sector 4", 3, 200, 30, "Wet")
    );

    manager.addBin(
        Bin("B105", "Sector 5", 3, 300, 99, "Mixed")
    );

    // Create trucks
    manager.addTruck(
        Truck("T01", 500, 100, 0)
    );

    manager.addTruck(
        Truck("T02", 800, 200, 2)
    );

    manager.addTruck(
        Truck("T03", 250, 50, 1)
    );

    // Create driver
    Driver driver1(
        101,
        "Aryan",
        "9876543210",
        "T02"
    );

    // Create depots
    manager.addDepot(
        Depot("D01", "Main Depot", 0)
    );

    manager.addDepot(
        Depot("D02", "Central Depot", 2)
    );

    // Display information
    manager.displayBins();
    manager.displayTrucks();
    driver1.displayDriver();
    manager.displayDepots();

    // Run collection optimization
    manager.optimizeCollection();

    // Demonstrate collection completion
    manager.completeCollection("B105");

    // Display updated information
    cout << "\n--- UPDATED SYSTEM STATUS ---\n";

    manager.displayBins();
    manager.displayTrucks();

    freeGraph(&graph);

    return 0;
}