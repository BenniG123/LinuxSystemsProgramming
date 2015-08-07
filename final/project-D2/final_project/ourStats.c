#include <pthread.h>
#include <math.h>
#include <stdlib.h>

#include "ourStats.h"

SORTTYPE ourMax(SORTTYPE* arr, long long size) {
	int i;
	pthread_t* threads;
	SearchParams_t* params;
	int minBlocksize;
	int numThreads = initStats(arr, size, &threads, &params, &minBlocksize);

	for (i = 0; i < numThreads; i++) {
		params[i].max = arr[params[i].start];
		pthread_create(&threads[i], NULL, &maxFind, &params[i]);
	}

	pthread_join(threads[0], NULL);
	SORTTYPE globalMax = params[0].max;
	for (i = 1; i < numThreads; i++) {
		pthread_join(threads[i], NULL);
		if (params[i].max > globalMax) {
			globalMax = params[i].max;
		}
	}
	
	cleanupStats(threads, params);

	return globalMax;
}

SORTTYPE ourMin(SORTTYPE* arr, long long size) {
	int i;
	pthread_t* threads;
	SearchParams_t* params;
	int minBlocksize;
	int numThreads = initStats(arr, size, &threads, &params, &minBlocksize);

	for (i = 0; i < numThreads; i++) {
		params[i].min = arr[params[i].start];
		pthread_create(&threads[i], NULL, &minFind, &params[i]);
	}

	pthread_join(threads[0], NULL);
	SORTTYPE globalMin = params[0].min;
	for (i = 1; i < numThreads; i++) {
		pthread_join(threads[i], NULL);
		if (params[i].min < globalMin) {
			globalMin = params[i].min;
		}
	}
	
	cleanupStats(threads, params);

	return globalMin;
}

SORTTYPE ourPre(SORTTYPE* arr, long long size, SORTTYPE target, int* error) {
	int i;
	pthread_t* threads;
	SearchParams_t* params;
	int minBlocksize;
	int numThreads = initStats(arr, size, &threads, &params, &minBlocksize);
	*error = 0;

	for (i = 0; i < numThreads; i++) {
		params[i].preTarget = target;
		params[i].locPre = target;
		pthread_create(&threads[i], NULL, &preFind, &params[i]);
	}

	pthread_join(threads[0], NULL);
	SORTTYPE globalPre = params[0].locPre;
	for (i = 1; i < numThreads; i++) {
		pthread_join(threads[i], NULL);
		if ((target - params[i].locPre != 0) && (target - params[i].locPre < target - globalPre)) {
			globalPre = params[i].locPre;
		}
	}

	// If there was no successor
	if (globalPre == target) {
		*error = 1;
	}

	cleanupStats(threads, params);

	return globalPre;
}

SORTTYPE ourSuc(SORTTYPE* arr, long long size, SORTTYPE target, int* error) {
	int i;
	pthread_t* threads;
	SearchParams_t* params;
	int minBlocksize;
	int numThreads = initStats(arr, size, &threads, &params, &minBlocksize);
	*error = 0;

	for (i = 0; i < numThreads; i++) {
		params[i].sucTarget = target;
		params[i].locSuc = target;
		pthread_create(&threads[i], NULL, &sucFind, &params[i]);
	}

	pthread_join(threads[0], NULL);
	SORTTYPE globalSuc = params[0].locSuc;
	for (i = 1; i < numThreads; i++) {
		pthread_join(threads[i], NULL);
		if (params[i].locSuc - target != 0 && params[i].locSuc - target < globalSuc - target) {
			globalSuc = params[i].locSuc;
		}
	}

	if (globalSuc == target) {
		*error = 1;
	}
	
	// If there was no successor
	if (globalSuc == target) {
		*error = 1;
	}

	cleanupStats(threads, params);

	return globalSuc;
}

void ourSort(SORTTYPE* arr, long long size) {
	int i;
	pthread_t* threads;
	SearchParams_t* params;
	int minBlocksize;
	int numThreads = initStats(arr, size, &threads, &params, &minBlocksize);
	SORTTYPE* arr2 = (SORTTYPE*) malloc(sizeof(SORTTYPE) * size);

	for (i = 0; i < numThreads; i++) {
		params[i].arr2 = arr2;
		pthread_create(&threads[i], NULL, &sort, &params[i]);
	}

	pthread_join(threads[0], NULL);
	for (i = 1; i < numThreads; i++) {
		pthread_join(threads[i], NULL);
	}
	
	int mergeCount = 1; // Number of times we have merged
	int curNumThreads = (int) ceil(numThreads/2.0); // Current number of threads (numThreads/(2^mergeCount))
	
	while (curNumThreads > 0) {
		// Create new threads
		for (i = 0; i < curNumThreads; i++) {
			params[i].arr = arr;
			params[i].start = i*minBlocksize*ceil(pow(2, mergeCount));
			params[i].end = ((i+1)*minBlocksize*ceil(pow(2, mergeCount)))-1;
			
			// Start of right parent
			params[i].mid = ((2*i + 1)*minBlocksize*ceil(pow(2, mergeCount-1)));
			
			if (params[i].end > size) {
				params[i].end = size;
			}
			
			params[i].threadID = i;
			params[i].arr2 = arr2;
			pthread_create(&threads[i], NULL, &threadMerge, &params[i]);
		}
		
		// Join
		for (i = 0; i < curNumThreads; i++) {
			pthread_join(threads[i], NULL);
		}
		
		if (curNumThreads/2 == 0) {
			break;
		}
		curNumThreads = (int) ceil(curNumThreads/2.0);
		mergeCount++;
	}

	free(arr2);
	cleanupStats(threads, params);
}

SORTTYPE ourMean(SORTTYPE* arr, long long size) {
	int i;
	pthread_t* threads;
	SearchParams_t* params;
	int minBlocksize;
	int numThreads = initStats(arr, size, &threads, &params, &minBlocksize);

	for (i = 0; i < numThreads; i++) {
		params[i].mean = 0;
		pthread_create(&threads[i], NULL, &meanFind, &params[i]);
	}

	double mean = 0;
	for (i = 0; i < numThreads; i++) {
		pthread_join(threads[i], NULL);
		mean += params[i].mean;
	}
	mean /= numThreads;
	
	cleanupStats(threads, params);

	return mean;
}

SORTTYPE ourMed(SORTTYPE* arr, long long size) {
	ourSort(arr, size);

	if (size % 2 == 0) {
		return arr[size/2];
	}
	return (arr[size/2] + arr[size/2 + 1])/2;
}

SORTTYPE ourStdDev(SORTTYPE* arr, long long size) {
	double mean = ourMean(arr, size);

	int i;
	pthread_t* threads;
	SearchParams_t* params;
	int minBlocksize;
	int numThreads = initStats(arr, size, &threads, &params, &minBlocksize);

	for (i = 0; i < numThreads; i++) {
		params[i].squareSum = 0;
		params[i].mean = mean;
		pthread_create(&threads[i], NULL, &stdDevFind, &params[i]);
	}

	double stddev = 0;
	for (i = 0; i < numThreads; i++) {
		pthread_join(threads[i], NULL);
		stddev += params[i].squareSum;
	}
	stddev /= size;
	stddev = sqrt(stddev);
	
	cleanupStats(threads, params);

	return stddev;
}
