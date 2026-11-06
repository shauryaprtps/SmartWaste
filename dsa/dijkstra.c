#include <stdio.h>
#include <limits.h>
#include "dijkstra.h"

#define INF INT_MAX

int shortestPath(Graph *graph,
                 int source,
                 int destination,
                 int path[],
                 int *pathLength,
                 int *totalDistance)
{
    int distance[MAX_NODES];
    int visited[MAX_NODES] = {0};
    int parent[MAX_NODES];

    int n = graph->nodeCount;

    *pathLength = 0;
    *totalDistance = INF;

    if (source < 0 || source >= n ||
        destination < 0 || destination >= n)
    {
        return 0;
    }

    // Initialize distances and parents
    for (int i = 0; i < n; i++)
    {
        distance[i] = INF;
        parent[i] = -1;
    }

    distance[source] = 0;

    // Dijkstra's algorithm
    for (int count = 0; count < n; count++)
    {
        int minDistance = INF;
        int current = -1;

        // Find the nearest unvisited node
        for (int i = 0; i < n; i++)
        {
            if (!visited[i] &&
                distance[i] < minDistance)
            {
                minDistance = distance[i];
                current = i;
            }
        }

        if (current == -1)
        {
            break;
        }

        visited[current] = 1;

        if (current == destination)
        {
            break;
        }

        // Check all connected roads
        Edge *temp = graph->adjList[current];

        while (temp != NULL)
        {
            int next = temp->destination;
            int weight = temp->distance;

            if (!visited[next] &&
                distance[current] != INF &&
                distance[current] <= INF - weight &&
                distance[current] + weight < distance[next])
            {
                distance[next] =
                    distance[current] + weight;

                parent[next] = current;
            }

            temp = temp->next;
        }
    }

    // Check whether destination is reachable
    if (distance[destination] == INF)
    {
        return 0;
    }

    // Reconstruct shortest path
    int reversePath[MAX_NODES];
    int count = 0;

    int current = destination;

    while (current != -1 && count < MAX_NODES)
    {
        reversePath[count++] = current;
        current = parent[current];
    }

    // Reverse the route
    for (int i = 0; i < count; i++)
    {
        path[i] = reversePath[count - 1 - i];
    }

    *pathLength = count;
    *totalDistance = distance[destination];

    return 1;
}