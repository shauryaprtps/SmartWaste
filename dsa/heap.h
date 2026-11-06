#ifndef HEAP_H
#define HEAP_H

#define MAX_BINS 100

typedef struct
{
    char binID[10];
    float fillPercentage;

} HeapBin;

typedef struct
{
    HeapBin bins[MAX_BINS];
    int size;

} MaxHeap;

void initializeHeap(MaxHeap *heap);

void insertBin(MaxHeap *heap, HeapBin bin);

HeapBin extractMax(MaxHeap *heap);

void displayHeap(MaxHeap *heap);

int isEmpty(MaxHeap *heap);

#endif