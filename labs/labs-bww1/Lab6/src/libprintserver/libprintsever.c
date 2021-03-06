#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>

#include "print_server_client.h"

/**
 * @brief     Send a print job to the print server daemon program.
 * @details   This function should send the given job to the print server program using
 *            your chosen method of IPC.
 * @param     handle
 *                 You may optionally implement this for extra credit.  If used this should return
 *                 a unique number that represents this print job and can be used to get information
 *                 about the job with other function calls.  If you choose not to implement this
 *                 you can safely ignore this param.
 * @param     driver
 *                 The name of the driver to print the job to.  Required.
 * @param     job_name
 *                 A name given to the job.  Required
 * @param     description
 *                 A discription of the job.  Optional, must handel being set to NULL if no
 *                 description is given.
 * @param     data
 *                 The actual print job to be printed in Postscript format.  Must start
 *                 with a "%" sign and end with "%EOF".
 * @return    This function should return 0 if the print server accepts the job as being valid.
 *            Note, this should return as soon as the print server accepts the job, but it should
 *            NOT wait for the server to finish printing the job.  It should return a number < 0
 *            on error.
 */
int printer_print(int* handle, char* driver, char* job_name, char* description, char* data)
{
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

	sem_t * semaphore = sem_open("/driver_mutex", O_CREAT, 0, 0);
	if(semaphore == SEM_FAILED) {
		perror("Opening Semaphore");
		return -1;
	}
	printf("Opened named semaphore\n");

	strcpy(shared_mem->driver_name, driver);
	strcpy(shared_mem->job_name, job_name);
	strcpy(shared_mem->description, description);
	strcpy(shared_mem->data,data);

	sem_post(semaphore);
	sem_close(semaphore);
	return 0;
}

/**
 * @brief     List the currently installed printer drivers from the print server
 * @details   This function should query the print server for a list of currently installed drivers
 *            and return them as a NULL terminated array of printer_driver_t objects.
 * @param     number
 *                 Returns the number of printer drivers currently installed in the print server daemon
 * @return    An array of number printer_driver_t* objects followed by NULL
 * @example
 *
 * int num;
 * printer_driver* list[] = printer_list_driver(&num);
 * printf("printer_name=%s", list[0]->printer_name);
 *
 */
printer_driver_t** printer_list_drivers(int *number)
{
	printer_driver_t** output_list = malloc((*number + 1) * sizeof(printer_driver_t *));
	int i;
	for(i = 0; i < *number; i++) {
		output_list[i] = malloc(sizeof(printer_driver_t *));
	}
	output_list[*number] = NULL;

	char buf[100];
	int fd = open("/tmp/printer_driver_fifo", O_RDWR);

    	write(fd, "list", sizeof(buf));
	sprintf(buf, "%d", *number);
	write(fd, buf, sizeof(buf));

	for(i = 0; i < *number; i++) {
		printf("receiving number %d\n", i);

		read(fd,buf,sizeof(buf));
		strcpy(output_list[i]->printer_name, buf);

		read(fd,buf,sizeof(buf));
		strcpy(output_list[i]->driver_name, buf);

		read(fd,buf,sizeof(buf));
		strcpy(output_list[i]->driver_version, buf);

		printf("received number %d\n", i);
	}
	close(fd);

	return output_list;
}


// Optional additional functions you may choose to implement for extra credit.
#if 0

/**
 * @brief     Install a new driver into the print server daemon.
 * @details   This function is used to hot-plug a new printer driver into the daemon at run time.
 * @param     driver_location
 *                 The file location of the printer driver (this will be in lab 7)
 * @param     driver
 *                 Details about the name and description of the driver.
 * @return    0 if successful, < 0 if something goes wrong.
 */
int printer_install_driver(char* driver_location, printer_driver_t driver);

/**
 * @brief     Uninstall a currently installed printer driver.
 * @param     driver
 *                 The driver to uninstall
 * @return    0 if successful, < 0 if something goes wrong
 */
int printer_uninstall_driver(printer_driver_t driver);

/**
 * @brief     Determine if a print job has finished yet
 * @param     handle
 *                 The handle to the print job returned by printer_print() function
 * @return    1 if the job has finished, 0 if it has not finished, and < 0 is something goes wrong
 */
int printer_is_finished(int handle);

/**
 * @brief     Wait for a print job to finish printing before continuing
 * @param     handle
 *                 The handle to the print job returned by printer_print() function
 * @return    0 if successful, < 0 if something goes wrong
 */
int printer_wait(int handle)
 
/**
 * @brief     Cancel an already submitted job if it has not already been sent to the printer.
 * @param     handle
 *                 The handle to the print job returned by printer_print() function
 * @return    0 if the job was successfully canceled, > 1 if the job has already printed, 
 *            < 0 if something goes wrong
 */
int printer_cancel_job(int handle);

/**
 * @brief     Pause an already submitted job if it has not already been sent to the printer.
 * @param     handle
 *                 The handle to the print job returned by printer_print() function
 * @return    0 if the job was successfully paused, > 1 if the job has already printed,
 *            < 0 if something goes wrong.
 */
int printer_pause_job(int handle);

/**
 * @brief     Resume printing a job that was paused
 * @param     handle
 *                 The handle to the print job returned by printer_print() function
 * @return    0 if the job was successfully resumed, < 0 if something goes wrong
 */
int printer_resume_job(int handle);

*/
#endif
