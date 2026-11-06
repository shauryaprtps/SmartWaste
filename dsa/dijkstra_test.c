#include <stdio.h>
#include "graph.h"
#include "dijkstra.h"

int main()
{
    Graph graph;

    initializeGraph(&graph, 4);

    // Create road network
    addEdge(&graph, 0, 1, 4);
    addEdge(&graph, 0, 2, 2);
    addEdge(&graph, 1, 2, 5);
    addEdge(&graph, 1, 3, 3);
    addEdge(&graph, 2, 3, 1);

    int path[MAX_NODES];
    int pathLength;
    int totalDistance;

    int source = 0;
    int destination = 3;

    printf("SmartWaste Route Optimization\n");

    if (shortestPath(&graph, source, destination,
                     path, &pathLength, &totalDistance))
    {
        printf("\nShortest Route: ");

        for (int i = 0; i < pathLength; i++)
        {
            printf("%d", path[i]);

            if (i < pathLength - 1)
            {
                printf(" -> ");
            }
        }

        printf("\nTotal Distance: %d km\n", totalDistance);
    }
    else
    {
        printf("No route available!\n");
    }

    freeGraph(&graph);

    return 0;
}