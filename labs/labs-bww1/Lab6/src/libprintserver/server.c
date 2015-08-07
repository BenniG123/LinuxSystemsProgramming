#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>

#include "print_server_client.h"

int main(void)
{
	int fd, i;
	char *myfifo = "/tmp/printer_driver_fifo";
	char input[80];
	char buf[100];

	/* create the FIFO (named pipe) */
	mkfifo(myfifo, 0666);

	printer_driver_t* list[100];
	for(i = 0; i < 100; i++) {
		list[i] = malloc(sizeof(printer_driver_t*));
	}
	for(i = 0; i < 100; i++) {
		strcpy(list[i]->driver_name, "TEST");
	}

	fd = open(myfifo, O_RDWR);
	
	while(1) {
    		if(read(fd, buf, 100) == 0) {
			//buf[0] = '\0';
		} else {
			if(strncmp(buf, "list", 4) == 0) {

				printf("Sending driver list\n");

				read(fd, buf, 100);
				long length = strtol(buf, NULL, 10);
				printf("length = %d\n", length);

				for(i = 0; i < length; i++) {
					printf("sending number %d\n", i);
					send(myfifo, list[i]->printer_name, sizeof(char[100]));
					send(myfifo, list[i]->driver_name, sizeof(char[100]));
					send(myfifo, list[i]->driver_version, sizeof(char[100]));
					printf("sent number %d\n", i);
				}
			}
		}
	}

    	/*
	* Printer server code for printer_print

	int fd;
	fd = shm_open("/print_job_mem", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP); // open the shared memory area creating it if it doesn't exist
	if(!fd)
	{
		perror("shm_open\n");
		return -1;
	}
	if(ftruncate(fd, sizeof(struct PRINT_JOB_STRUCT)))
	{
		perror("ftruncate\n");
		return -1;
	}
	struct PRINT_JOB_STRUCT* shared_mem;
	shared_mem = mmap(NULL, sizeof(struct PRINT_JOB_STRUCT), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(!shared_mem)
	{
		perror("mmap\n");
		return -1;
	}
	if(close(fd))
	{
		perror("close\n");
		return -1;
	}
	if(sem_unlink("/driver_mutex") < 0) {
		perror("Destroy Semaphore error:");
	}

	//
	sem_t * semaphore = sem_open("/driver_mutex", O_CREAT, 0666, 0);
	if(semaphore == SEM_FAILED) {
		perror("Constructing Semaphore");
		exit(1);
	}
	printf("Created named semaphore\n");
	while(1) {
		sem_wait(semaphore);

		printf("%s\n", shared_mem->driver_name);
		printf("%s\n", shared_mem->job_name);
		printf("%s\n", shared_mem->description);
		printf("%s\n",shared_mem->data);
	}
	sem_close(semaphore);*/
}
