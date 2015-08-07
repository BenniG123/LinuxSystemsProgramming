#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

struct matrix_Data
{
	int **Matrix_A;
	int **Matrix_B;
	int x;
	int y;
	int size;
	int output;
};

void calculate_Cell(struct matrix_Data *md){
	
	int x, y;
	int *A_Array = malloc(md->size * sizeof(int));
	int *B_Array = malloc(md->size * sizeof(int));
	md->output = 0;
	for(x = 0;x < md->size; x++) {
		A_Array[x] = md->Matrix_A[x][md->y];
	}
	for(y = 0;y < md->size; y++) {
		B_Array[y] = md->Matrix_B[md->x][y];
	}
	for(x = 0; x < md->size; x++) {
		md->output += A_Array[x] * B_Array[x];
	}
}

int main(int argc, char **argv) {

	if(argc != 4) {
		printf("The input convention is Matrix A file, Matrix B file, Output File\n");
		return 0;
	}

	FILE *fr_A;
	//Attempt to open file A
	if((fr_A = fopen(argv[1], "r")) == NULL) {
		//Something went wrong
		perror("File reading error");
		return -1;
	}

	int counter = -2;
	int i;
	int A_Size = 0;

	//Buffer for reading the input file
	char line[2048];

	//Parse in Input file A
	while(fgets(line, 2048, fr_A) != NULL) {
		counter++;
		if(counter == -1) {
			//Parse the first line to see how big the matrix is
			char * A_Size_String = strrchr(line, ' ');
			A_Size = atoi(A_Size_String);
			break;
		}
	}

	fclose(fr_A);

	if(A_Size == 0) {
		printf("Size not specified properly in Matrix A\n");
		return 0;
	}	
	

	//2D Array for Matrix A
	int **Matrix_A = (int **) malloc(A_Size * sizeof(int*));

	//Allocate Memory for our first matrix
	for(i = 0; i < A_Size; i++) {
		Matrix_A[i] = (int *) malloc(A_Size * sizeof(int));
	}

	if((fr_A = fopen(argv[1], "r")) == NULL) {
		//Something went wrong
		perror("File reading error");
		return -1;
	}
	counter = -2;

	//Parse in Input file A
	while(fgets(line, 2048, fr_A) != NULL) {
		counter++;
		if(counter > - 1) {
			//Take in input
			for(i = 0; i < A_Size; i++) {
				//Code for getting intederminate sized integers (don't need to worry about yet)
				//char * after_Int = strchr(line, ' ');
				//char * next_Int;
				//int length = strlen(line) - strlen(after_Int);
				//strncpy(next_Int, line, length);
				//next_Int[length] = '\0';
				Matrix_A[counter][i] = line[i*2] - '0';
			}
		}
	}

	fclose(fr_A);

	//Read in Matrix B
	FILE *fr_B;

	if((fr_B = fopen(argv[2], "r")) == NULL) {
		//Something went wrong
		perror("File reading error");
		return -1;
	}

	counter = -2;
	int B_Size;

	//Parse in Input file A
	while(fgets(line, 2048, fr_B) != NULL) {
		counter++;
		if(counter == -1) {
			//Parse the first line to see how big the matrix is
			char * B_Size_String = strrchr(line, ' ');
			B_Size = atoi(B_Size_String);
			break;
		}
	}
	
	if(B_Size != A_Size) {
		printf("Array sizes must be identical, yo\n");
		return 0;
	}

	fclose(fr_B);
	

	//2D Array for Matrix B
	int **Matrix_B = (int **) malloc(A_Size * sizeof(int*));

	//Allocate Memory for our second Matrix
	for(i = 0; i < A_Size; i++) {
		Matrix_B[i] = (int *) malloc(A_Size * sizeof(int));
	}

	if((fr_B = fopen(argv[2], "r")) == NULL) {
		//Something went wrong
		perror("File reading error");
		return -1;
	}

	counter = -2;
	//Parse in Input file B
	while(fgets(line, 2048, fr_B) != NULL) {
		counter++;
		if(counter > - 1) {
			//Take in input
			for(i = 0; i < A_Size; i++) {
				Matrix_B[counter][i] = line[i*2] - '0';
			}
		}
	}

	fclose(fr_B);

	//TIME TO MULTITHREAD BITCH
	//Create a thread for every cell in the output - Max of 1024 pthreads/system

	//2D Array for Matrix C
	int **Matrix_C = (int **) malloc(A_Size * sizeof(int*));
	
	//Allocate Memory for our third Matrix
	for(i = 0; i < A_Size; i++) {
		Matrix_C[i] = (int *) malloc(A_Size * sizeof(int));
	}

	//Array for return values
	struct matrix_Data md[1024];

	//Array of threads
	pthread_t *Threads = malloc(1024 * sizeof(pthread_t));

	int err[1024];
	int j; 
	int t_C = 0; //Thread counter
	int final_Val = A_Size * A_Size;
	for(i = 0; i < A_Size; i++) {
		//Create 1024 threads
		for(j = 0; j < A_Size; j++) {

			md[t_C].Matrix_A = Matrix_A;
			md[t_C].Matrix_B = Matrix_B;
			md[t_C].x = i;
			md[t_C].y = j;
			md[t_C].size = A_Size;
			md[t_C].output = 0;

			err[t_C] = pthread_create(&Threads[t_C], NULL, (void *)calculate_Cell, &md[t_C]);

			if(err[t_C] != 0) {
				perror("pthread_create encountered an error");
				exit(1);
			} else{
				err[t_C] = 0;
			}

			t_C++;
			//Check for threshold number of threads or final thread
			if(t_C == 1024 || t_C == final_Val % 1024) {
				//Join all threads
				while(t_C > 0) {
					t_C--;
					if(pthread_join(Threads[t_C], NULL) != 0) {
						perror("Thread join error");
						return 0;
					}
					Matrix_C[md[t_C].x][md[t_C].y] = md[t_C].output;
				}
			}
		}	
	}

	//It works!  Write to the output file
	FILE *fr_C;

	//Attempt to open file A
	if((fr_C = fopen(argv[3], "w")) == NULL) {
		//Something went wrong
		perror("File C open error");
		return -1;
	}

	fprintf(fr_C, "#N = %d\n", A_Size);

	char output_String[1024];
	for(i = 0; i < A_Size; i++) {
		sprintf(output_String, "%d ", Matrix_C[i][0]);
		fprintf(fr_C, output_String);
		for(j = 1; j < A_Size - 1; j++) {
			sprintf(output_String, "%d ", Matrix_C[i][j]);
			fprintf(fr_C, output_String);
		}
		sprintf(output_String, "%d\n", Matrix_C[i][j]);
		fprintf(fr_C, output_String);
	}
	
	fclose(fr_C);
	return 0;
}
