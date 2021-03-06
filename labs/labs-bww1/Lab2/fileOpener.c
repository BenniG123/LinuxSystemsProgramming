#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>

int main( int argc, const char* argv[] )
{

	//Determine which type of file was selected
	int length = strlen(argv[1]);
	int status = 0;
	char fileType[3];
	strncpy(fileType,argv[1] + length - 3, length - 1);

	//Check for correctness of input ( period in the right place)
	if(argv[1][length - 4] != 46) {
		//No valid file type has been entered
		perror("Incorrect input");
	}
		if(strcmp(fileType, "doc") == 0){
			pid_t child;
			child = fork();
			if(child == 0) {
				execlp("libreoffice", "-o", argv[1], NULL);
			} else if(child > 0) {
				wait(&status);
				printf("child process is done, status is: %dnn", status);
				return 0;
			} else {
				// fork failed, handle error here
				perror("fork");
			}			
		}
		else if(strcmp(fileType, "odt") == 0){
			pid_t child;
			child = fork();
			if(child == 0) {
				execlp("libreoffice", "-o", argv[1], NULL);
			} else if(child > 0) {
				wait(&status);
				printf("child process is done, status is: %dnn", status);
				return 0;
			} else {
				// fork failed, handle error here
				perror("fork");
			}			
		}
		else if(strcmp(fileType, "png") == 0){
			pid_t child;
			child = fork();
			if(child == 0) {
				execlp("eog", argv[1], NULL);
			} else if(child > 0) {
				wait(&status);
				printf("child process is done, status is: %dnn", status);
				return 0;
			} else {
				// fork failed, handle error here
				perror("fork");
			}			
		}
		else if(strcmp(fileType, "txt") == 0){
			pid_t child;
			child = fork();
			if(child == 0) {
				execlp("libreoffice", "-o", argv[1], NULL);
			} else if(child > 0) {
				wait(&status);
				printf("child process is done, status is: %dnn", status);
				return 0;
			} else {
				// fork failed, handle error here
				perror("fork");
			}			
		}
		else if(strcmp(fileType, "pdf") == 0){
			pid_t child;
			child = fork();
			if(child == 0) {
				execlp("libreoffice", "-view", argv[1], NULL);
			} else if(child > 0) {
				wait(&status);
				printf("child process is done, status is: %dnn", status);
				return 0;
			} else {
				// fork failed, handle error here
				perror("fork");
			}			
		}
		else if(strcmp(fileType, "mp3") == 0){
			pid_t child;
			child = fork();
			if(child == 0) {
				execlp("rhythmbox-client", "--enqueue", argv[1], NULL);
			} else if(child > 0) {
				wait(&status);
				printf("child process is done, status is: %dnn", status);
				return 0;
			} else {
				// fork failed, handle error here
				perror("fork");
			}			
		}
		else {
			//invalid file type
			perror("Incorrect file type.  Valid file types include: .doc, .odt, .txt, .png, .mp3, .pdf");
		}
	
}
