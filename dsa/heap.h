#ifndef HEAP_H
#define HEAP_H

#define MAX_BINS 100

typedef struct
{
    char binID[10];
    float fillPercentage;

} Bin;

typedef struct
{
    Bin bins[MAX_BINS];
    int size;

} MaxHeap;

void initializeHeap(MaxHeap *heap);

void insertBin(MaxHeap *heap, Bin bin);

Bin extractMax(MaxHeap *heap);

void displayHeap(MaxHeap *heap);

int isEmpty(MaxHeap *heap);

#endif