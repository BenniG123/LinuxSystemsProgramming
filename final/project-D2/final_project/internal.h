#include <pthread.h>

#define SORTTYPE unsigned char
#define NUMTHREADS 100
#define ITERS 1000
#define TYPESPEC "%d"

typedef struct {
	int a;
	int b;
	int c;
} OverheadType_t;

typedef struct {
	SORTTYPE* arr;
	SORTTYPE* arr2;
	long long start;
	long long mid;
	long long end;
	SORTTYPE max;
	SORTTYPE min;
	SORTTYPE locPre;
	SORTTYPE preTarget;
	SORTTYPE locSuc;
	SORTTYPE sucTarget;
	double mean;
	double squareSum;
	int threadID;
} SearchParams_t;

void* overheadRoutine(void* arg);
void* maxFind(void* arg);
void* minFind(void* arg);
void* preFind(void* arg);
void* sucFind(void* arg);
void* meanFind(void* arg);
void* stdDevFind(void* arg);
int isSorted(SORTTYPE* arr, int start, int end);
void copyArr(SORTTYPE* arr2, int begin, int end, SORTTYPE* arr);
void merge(SORTTYPE* arr, int begin, int mid, int end, SORTTYPE* arr2);
void* threadMerge(void* arg);
void subsort(SORTTYPE* arr, int begin, int end, SORTTYPE* arr2);
void* sort(void* arg);
int findOverheadTime();
int findMinBlocksize(int overheadTime);
int initStats(SORTTYPE* arr, long long size, pthread_t** threads, SearchParams_t** params, int* minBS);
void cleanupStats(pthread_t* threads, SearchParams_t* params);
