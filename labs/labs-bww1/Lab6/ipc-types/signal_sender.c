#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char** argv)
{
	pid_t pid = strtol(argv[1],NULL,10);
	kill(pid,42);
	return 0;
}



