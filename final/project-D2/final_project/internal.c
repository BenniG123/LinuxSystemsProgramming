#include <math.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include "internal.h"

void* overheadRoutine(void* arg) {
	OverheadType_t* myArg = arg;
	myArg->c = myArg->a + myArg->b;
}

void* maxFind(void* arg) {
	SearchParams_t* myArg = arg;
	long long i;
	for (i = myArg->start; i < myArg->end; i++) {
		if (myArg->max < myArg->arr[i]) {
			myArg->max = myArg->arr[i];
		}
	}
}

void* minFind(void* arg) {
	SearchParams_t* myArg = arg;
	long long i;
	for (i = myArg->start; i < myArg->end; i++) {
		if (myArg->min > myArg->arr[i]) {
			myArg->min = myArg->arr[i];
		}
	}
}

void* preFind(void* arg) {
	SearchParams_t* myArg = arg;
	long long i;
	for (i = myArg->start; i < myArg->end; i++) {
		if ((myArg->preTarget > myArg->arr[i]) && (myArg->preTarget - myArg->arr[i] != 0) && ((myArg->preTarget - myArg->arr[i] < myArg->preTarget - myArg->locPre) || (myArg->preTarget == myArg->locPre))) {
			myArg->locPre = myArg->arr[i];
		}
	}
}

void* sucFind(void* arg) {
	SearchParams_t* myArg = arg;
	long long i;
	for (i = myArg->start; i < myArg->end; i++) {
		if ((myArg->sucTarget < myArg->arr[i]) && (myArg->arr[i] - myArg->sucTarget != 0) && ((myArg->arr[i] - myArg->sucTarget < myArg->locSuc - myArg->sucTarget) || (myArg->sucTarget == myArg->locSuc))) {
			myArg->locSuc = myArg->arr[i];
		}
	}
}

void* meanFind(void* arg) {
	SearchParams_t* myArg = arg;
	long long i;
	for (i = myArg->start; i < myArg->end; i++) {
		myArg->mean += myArg->arr[i];
	}
	myArg->mean /= (myArg->end - myArg->start);
}

void* stdDevFind(void* arg) {
	SearchParams_t* myArg = arg;
	long long i;

	myArg->squareSum = 0;
	for (i = myArg->start; i < myArg->end; i++) {
		myArg->squareSum += pow(myArg->arr[i] - myArg->mean, 2);;
	}
}

/**
 * Helper function to determine if subarray is sorted
 */
int isSorted(SORTTYPE* arr, int start, int end) {
	int i;
	for (i = start; i < end - 1; i++) {
		if (arr[i] > arr[i+1]) {
			return 0;
		}
	}
	return 1;
}

void copyArr(SORTTYPE* arr2, int begin, int end, SORTTYPE* arr) {
	int i;
	for (i = begin; i < end; i++) {
		arr[i] = arr2[i];
	}
}

void merge(SORTTYPE* arr, int begin, int mid, int end, SORTTYPE* arr2) {
	int i;
	int i0 = begin;
	int i1 = mid;
	for (i = begin; i < end; i++) {
		if (i0 < mid && (i1 >= end || arr[i0] <= arr[i1])) {
			arr2[i] = arr[i0];
			i0++;
		}
		else {
			arr2[i] = arr[i1];
			i1++;
		}
	}
}

void* threadMerge(void* arg) {
	SearchParams_t* myArg = arg;
	merge(myArg->arr, myArg->start, myArg->mid, myArg->end + 1, myArg->arr2); // Merge two subarrays
	copyArr(myArg->arr2, myArg->start, myArg->end + 1, myArg->arr); // Copy merged runs back to A
}

void subsort(SORTTYPE* arr, int begin, int end, SORTTYPE* arr2) {
	if (end - begin < 2) {
		return;
	}
	int mid = (end + begin)/2;
	subsort(arr, begin, mid, arr2);
	subsort(arr, mid, end, arr2);
	merge(arr, begin, mid, end, arr2); // Merge two subarrays
	copyArr(arr2, begin, end, arr); // Copy merged runs back to A
}

void* sort(void* arg) {
	SearchParams_t* myArg = arg;
	subsort(myArg->arr, myArg->start, myArg->end + 1, myArg->arr2); // End is inclusive
}

int findOverheadTime() {
	int i, j;
	pthread_t threads[100];
	OverheadType_t arg = {1, 2, 0};
	struct timespec start;
	struct timespec end;
	long long avg = 0;
	
	for (i = 0; i < ITERS; i++) {
		long locAvg = 0;
		clock_gettime(CLOCK_REALTIME, &start);

		for (j = 0; j < NUMTHREADS; j++) {
			pthread_create(&threads[j], NULL, &overheadRoutine, &arg);
		}
		
		clock_gettime(CLOCK_REALTIME, &end);
		locAvg = end.tv_nsec - start.tv_nsec;
		if (end.tv_nsec - start.tv_nsec < 0) { // Need this in case tv_nsec overflowed; assuming that each loop doesn't run for more than 1 second
			locAvg += 1000000000;
		}
		
		for (j = 0; j < NUMTHREADS; j++) {
			pthread_join(threads[j], NULL);
		}
		avg += locAvg;
	}
	return  ceil((avg/(NUMTHREADS*ITERS))/1000.0)*1000;
}

int findMinBlocksize(int overheadTime) {
	int i, j, k;
	struct timespec start;
	struct timespec end;
	long long runTime;
	long long max;
	long long avg = 0;
	
	for (i = 0; i < ITERS; i++) { // Do for several iterations
		for (j = 1; 1; j *= 2) { // Keep increasing the size of arr by a factor of 2
			long long* arr = (long long*) malloc(sizeof(long long) * j);
			for (k = 0; k < j; k++) { // Fill array
				arr[k] = rand();
			}
	
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	
			// Search for max
			max = arr[0];
			for (k = 1; k < j; k++) {
				if (max < arr[k]) {
					max = arr[k];
				}
			}
		
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
			runTime = end.tv_nsec - start.tv_nsec;
			if (end.tv_nsec - start.tv_nsec < 0) { // Need this in case tv_nsec overflowed; assuming that each loop doesn't run for more than 1 second
				runTime += 1000000000;
			}
			
			free(arr);
			
			if (runTime > overheadTime) {
				break;
			}
		}
		avg += j;
	}
	return avg/ITERS;
}

int initStats(SORTTYPE* arr, long long size, pthread_t** threads, SearchParams_t** params, int* minBS) {
	// Calculate overhead time
	int overheadTime = findOverheadTime();

	// Calculate minimum blocksize
	int minBlocksize = findMinBlocksize(overheadTime);
	*minBS = minBlocksize;

	int numThreads = size/minBlocksize + 1;
	*threads = (pthread_t*) malloc(sizeof(pthread_t) * numThreads);
	*params = (SearchParams_t*) malloc(sizeof(SearchParams_t) * numThreads);

	int i;
	for (i = 0; i < numThreads; i++) {
		(*params)[i].arr = arr;
		(*params)[i].start = i*minBlocksize;
		(*params)[i].end = ((i+1)*minBlocksize)-1;
		if ((*params)[i].end > size) {
			(*params)[i].end = size;
		}
		(*params)[i].threadID = i;
	}

	return numThreads;
}

void cleanupStats(pthread_t* threads, SearchParams_t* params) {
	free(threads);
	free(params);
}