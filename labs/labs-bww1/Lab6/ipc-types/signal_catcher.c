#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void my_quit_handler(int i)
{
	printf("\nSignal Recieved!\n");
	exit(-1);
	fflush(stdout);
}

int main(int argc, char** argv)
{
	signal(42, my_quit_handler);
	while(1)
	{
		printf(".");
		fflush(stdout);
		sleep(1);
	}
	return 0;
}



