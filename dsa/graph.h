#ifndef GRAPH_H
#define GRAPH_H

#define MAX_NODES 20

typedef struct Edge
{
    int destination;
    int distance;
    struct Edge *next;

} Edge;

typedef struct
{
    int nodeCount;
    Edge *adjList[MAX_NODES];

} Graph;

void initializeGraph(Graph *graph, int nodes);

void addEdge(Graph *graph, int source,
             int destination, int distance);

void displayGraph(Graph *graph);

void freeGraph(Graph *graph);

#endif