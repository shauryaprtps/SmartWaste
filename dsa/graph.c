#include <stdio.h>
#include <stdlib.h>
#include "graph.h"

void initializeGraph(Graph *graph, int nodes)
{
    graph->nodeCount = nodes;

    for (int i = 0; i < nodes; i++)
    {
        graph->adjList[i] = NULL;
    }
}

void addEdge(Graph *graph, int source,
             int destination, int distance)
{
    if (source < 0 || source >= graph->nodeCount ||
        destination < 0 || destination >= graph->nodeCount ||
        distance < 0)
    {
        printf("Invalid road information!\n");
        return;
    }

    Edge *newEdge = malloc(sizeof(Edge));

    if (newEdge == NULL)
    {
        printf("Memory allocation failed!\n");
        return;
    }

    newEdge->destination = destination;
    newEdge->distance = distance;

    newEdge->next = graph->adjList[source];
    graph->adjList[source] = newEdge;

    // Add reverse connection for an undirected road
    Edge *reverseEdge = malloc(sizeof(Edge));

    if (reverseEdge == NULL)
    {
        printf("Memory allocation failed!\n");
        return;
    }

    reverseEdge->destination = source;
    reverseEdge->distance = distance;

    reverseEdge->next = graph->adjList[destination];
    graph->adjList[destination] = reverseEdge;
}

void displayGraph(Graph *graph)
{
    for (int i = 0; i < graph->nodeCount; i++)
    {
        Edge *temp = graph->adjList[i];

        printf("\nLocation %d: ", i);

        while (temp != NULL)
        {
            printf(" -> %d (%d km)",
                   temp->destination,
                   temp->distance);

            temp = temp->next;
        }
    }

    printf("\n");
}

void freeGraph(Graph *graph)
{
    for (int i = 0; i < graph->nodeCount; i++)
    {
        Edge *temp = graph->adjList[i];

        while (temp != NULL)
        {
            Edge *nextEdge = temp->next;
            free(temp);
            temp = nextEdge;
        }

        graph->adjList[i] = NULL;
    }

    graph->nodeCount = 0;
}