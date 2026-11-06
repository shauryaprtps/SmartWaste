#include <stdio.h>
#include <string.h>
#include "heap.h"

int main()
{
    MaxHeap heap;

    initializeHeap(&heap);

    Bin b1 = {"B101", 45};
    Bin b2 = {"B102", 95};
    Bin b3 = {"B103", 80};
    Bin b4 = {"B104", 30};
    Bin b5 = {"B105", 99};

    insertBin(&heap, b1);
    insertBin(&heap, b2);
    insertBin(&heap, b3);
    insertBin(&heap, b4);
    insertBin(&heap, b5);

    displayHeap(&heap);

    printf("\nStarting Garbage Collection Priority:\n");

    while (!isEmpty(&heap))
    {
        Bin selected = extractMax(&heap);

        printf("Collect Bin: %s | Fill: %.2f%%\n",
               selected.binID,
               selected.fillPercentage);
    }

    return 0;
}