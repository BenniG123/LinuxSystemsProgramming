// gcc final.c -o run -L./ -lstats -lrt -lm -pthread

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ourStats.h"

int main(int argc, char* argv[]) {
	int i;
	long long numElements = 0;
	SORTTYPE* arr;
	SORTTYPE preOrSuc = 0;
	char operation[7];
	
	if (argc < 2) {
		fprintf(stderr, "Not enough arguments passed.\n");
		return -1;
	}

	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0) {
			strncpy(operation, argv[++i], 6);
			operation[6] = '\0';
			if (strcmp(operation, "pre") == 0 || strcmp(operation, "suc") == 0) {
				preOrSuc = atoi(argv[++i]);
			}
		}
	}

	// Fill array
	scanf("%lld", &numElements);
	arr = (SORTTYPE*) malloc(sizeof(SORTTYPE) * numElements);
	for (i = 0; i < numElements; i++) {
		arr[i] = rand();
	}

	if (strcmp(operation, "max") == 0) {
		printf("The max is: " TYPESPEC "\n", ourMax(arr, numElements));
	} else if (strcmp(operation, "min") == 0) {
		printf("The min is: " TYPESPEC "\n", ourMin(arr, numElements));
	} else if (strcmp(operation, "pre") == 0) {
		int error;
		int tmp = ourPre(arr, numElements, preOrSuc, &error);
		if (error) {
			printf("There is no predecessor of " TYPESPEC "\n", preOrSuc);
		}
		else {
			printf("The predecessor of " TYPESPEC " is: " TYPESPEC "\n", preOrSuc, tmp);
		}
	} else if (strcmp(operation, "suc") == 0) {
		int error;
		int tmp = ourSuc(arr, numElements, preOrSuc, &error);
		if (error) {
			printf("There is no successor of " TYPESPEC "\n", preOrSuc);
		}
		else {
			printf("The successor of " TYPESPEC " is: " TYPESPEC "\n", preOrSuc, tmp);
		}
	} else if (strcmp(operation, "sort") == 0) {
		ourSort(arr, numElements);
		for (i = 0; i < numElements; i++) {
			printf(TYPESPEC, arr[i]);
			if (i != numElements - 1) {
				printf(", ");
			} else {
				printf("\n");
			}
		}
	} else if (strcmp(operation, "mean") == 0) {
		printf("The mean is: " TYPESPEC "\n", ourMean(arr, numElements));
	} else if (strcmp(operation, "med") == 0) {
		printf("The median is: " TYPESPEC "\n", ourMed(arr, numElements));
	} else if (strcmp(operation, "stddev") == 0) {
		printf("The standard deviation is: " TYPESPEC "\n", ourStdDev(arr, numElements));
	}
	
	// Free Memory
	free(arr);
	
	return 0;
}
