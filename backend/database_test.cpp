#include <iostream>
#include <vector>

#include "Database.h"

using namespace std;

int main()
{
    Database database;

    if (!database.connect("database/smartwaste.db"))
    {
        return 1;
    }

    vector<Bin> bins;
    vector<Truck> trucks;

    cout << "\n===== LOADING BINS FROM SQLITE =====\n";

    if (database.loadBins(bins))
    {
        for (int i = 0; i < bins.size(); i++)
        {
            bins[i].displayBin();
        }
    }

    cout << "\n===== LOADING TRUCKS FROM SQLITE =====\n";

    if (database.loadTrucks(trucks))
    {
        for (int i = 0; i < trucks.size(); i++)
        {
            trucks[i].displayTruck();
        }
    }

    cout << "\nTotal Bins Loaded: " << bins.size() << endl;
    cout << "Total Trucks Loaded: " << trucks.size() << endl;

    database.close();

    cout << "\nDatabase test completed!\n";

    return 0;
}
