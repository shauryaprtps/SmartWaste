#include <stdio.h>
#include <string.h>
#include "heap.h"

int main()
{
    MaxHeap heap;

    initializeHeap(&heap);

    HeapBin b1 = {"B101", 45};
    HeapBin b2 = {"B102", 95};
    HeapBin b3 = {"B103", 80};
    HeapBin b4 = {"B104", 30};
    HeapBin b5 = {"B105", 99};

    insertBin(&heap, b1);
    insertBin(&heap, b2);
    insertBin(&heap, b3);
    insertBin(&heap, b4);
    insertBin(&heap, b5);

    displayHeap(&heap);

    printf("\nStarting Garbage Collection Priority:\n");

    while (!isEmpty(&heap))
    {
        HeapBin selected = extractMax(&heap);

        printf("Collect HeapBin: %s | Fill: %.2f%%\n",
               selected.binID,
               selected.fillPercentage);
    }

    return 0;
}