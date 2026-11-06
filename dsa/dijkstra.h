#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph.h"

int shortestPath(Graph *graph,
                 int source,
                 int destination,
                 int path[],
                 int *pathLength,
                 int *totalDistance);

#endif