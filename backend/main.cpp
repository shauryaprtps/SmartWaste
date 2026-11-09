#include <iostream>

#include "Database.h"
#include "CollectionManager.h"
#include "Depot.h"

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

    // Connect to SQLite
    Database database;

    if (!database.connect("database/smartwaste.db"))
        return 1;

    // Create road network
    Graph graph;

    initializeGraph(&graph, 4);

    addEdge(&graph, 0, 1, 4);
    addEdge(&graph, 0, 2, 2);
    addEdge(&graph, 1, 2, 5);
    addEdge(&graph, 1, 3, 3);
    addEdge(&graph, 2, 3, 1);

    // Connect database and road network
    CollectionManager manager(&graph, &database);

    // Load bins and trucks from SQLite
    if (!manager.loadData())
    {
        freeGraph(&graph);
        return 1;
    }

    // Display database records
    manager.displayBins();
    manager.displayTrucks();
    manager.displayDriverDetails(101);

    // Add depot information
    manager.addDepot(
        Depot("D01", "Main Depot", 0)
    );

    manager.addDepot(
        Depot("D02", "Central Depot", 2)
    );

    manager.displayDepots();

    // Run priority and route optimization
    manager.optimizeCollection();

    // Demonstrate completed collection
     //  manager.completeCollection("B104");

    // Show updated application status
    cout << "\n--- UPDATED SYSTEM STATUS ---\n";

    manager.displayBins();
    manager.displayTrucks();

    // Clean up
    freeGraph(&graph);
    database.close();

    return 0;
}
