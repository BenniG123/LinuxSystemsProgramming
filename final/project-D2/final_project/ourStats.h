#include "internal.h"

SORTTYPE ourMax(SORTTYPE* arr, long long size);
SORTTYPE ourMin(SORTTYPE* arr, long long size);
SORTTYPE ourPre(SORTTYPE* arr, long long size, SORTTYPE target, int* error);
SORTTYPE ourSuc(SORTTYPE* arr, long long size, SORTTYPE target, int* error);
void ourSort(SORTTYPE* arr, long long size);
SORTTYPE ourMean(SORTTYPE* arr, long long size);
SORTTYPE ourMed(SORTTYPE* arr, long long size);
SORTTYPE ourStdDev(SORTTYPE* arr, long long size);
