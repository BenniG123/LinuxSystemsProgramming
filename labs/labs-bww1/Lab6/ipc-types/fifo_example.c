#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char** argv) 
{

    int fd;
    char *myfifo = "/tmp/myfifo";
    char input[80];

    /* create the FIFO (named pipe) */
    mkfifo(myfifo, 0666);

    /* write to the FIFO */
    fd = open(myfifo, O_WRONLY);
    scanf("%s\n", input);
    write(fd, input, sizeof(input));
    close(fd);

    /* remove the FIFO */
    unlink(myfifo);

    return 0;

}
