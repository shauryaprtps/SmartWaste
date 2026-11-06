#include <stdio.h>
#include "graph.h"

int main()
{
    Graph graph;

    initializeGraph(&graph, 4);

    addEdge(&graph, 0, 1, 4);
    addEdge(&graph, 0, 2, 2);
    addEdge(&graph, 1, 2, 5);
    addEdge(&graph, 1, 3, 3);
    addEdge(&graph, 2, 3, 1);

    printf("SmartWaste Road Network:\n");

    displayGraph(&graph);

    freeGraph(&graph);

    return 0;
}