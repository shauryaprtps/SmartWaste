#include <stdio.h>
#include <string.h>
#include "heap.h"

void initializeHeap(MaxHeap *heap)
{
    heap->size = 0;
}

int isEmpty(MaxHeap *heap)
{
    return heap->size == 0;
}

void insertBin(MaxHeap *heap, Bin bin)
{
    if (heap->size >= MAX_BINS)
    {
        printf("Heap is full!\n");
        return;
    }

    int i = heap->size;

    heap->bins[i] = bin;
    heap->size++;

    // Move the bin upward if its priority is higher
    while (i > 0)
    {
        int parent = (i - 1) / 2;

        if (heap->bins[i].fillPercentage >
            heap->bins[parent].fillPercentage)
        {
            Bin temp = heap->bins[i];
            heap->bins[i] = heap->bins[parent];
            heap->bins[parent] = temp;

            i = parent;
        }
        else
        {
            break;
        }
    }
}

void heapifyDown(MaxHeap *heap, int i)
{
    int largest = i;

    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < heap->size &&
        heap->bins[left].fillPercentage >
        heap->bins[largest].fillPercentage)
    {
        largest = left;
    }

    if (right < heap->size &&
        heap->bins[right].fillPercentage >
        heap->bins[largest].fillPercentage)
    {
        largest = right;
    }

    if (largest != i)
    {
        Bin temp = heap->bins[i];
        heap->bins[i] = heap->bins[largest];
        heap->bins[largest] = temp;

        heapifyDown(heap, largest);
    }
}

Bin extractMax(MaxHeap *heap)
{
    Bin emptyBin = {"NONE", 0};

    if (isEmpty(heap))
    {
        printf("No bins available!\n");
        return emptyBin;
    }

    Bin highest = heap->bins[0];

    heap->bins[0] = heap->bins[heap->size - 1];
    heap->size--;

    if (heap->size > 0)
    {
        heapifyDown(heap, 0);
    }

    return highest;
}

void displayHeap(MaxHeap *heap)
{
    if (isEmpty(heap))
    {
        printf("Heap is empty!\n");
        return;
    }

    printf("\nCurrent Bin Priority:\n");

    for (int i = 0; i < heap->size; i++)
    {
        printf("%s - %.2f%%\n",
               heap->bins[i].binID,
               heap->bins[i].fillPercentage);
    }
}