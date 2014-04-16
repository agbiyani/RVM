using namespace std;
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
int main()
{
	char * seg = (char*) malloc(100 * sizeof(char));
	char * helloworld = "Hello World";
	size_t size = 0;
	strcpy(seg, helloworld);
	cout << seg << endl;
		FILE * fd = fopen("test.log", "a");
		fprintf(fd, "offset=0\n");
		fprintf(fd, "size=100\n");
		//fwrite(region->segbase + region->offset, sizeof(char), region->size, fd);
		fclose(fd);
		//int fd = open("test.log", O_CREAT|O_WRONLY, 0777);
	int myfd = open("test.log", O_WRONLY);
	lseek(myfd, 0, SEEK_END);
	printf("fd = %d\n", myfd);
	size = write(myfd, seg, 100);
	size = write(myfd, seg, 100);
	size = write(myfd, seg, 100);
	printf("size = %d\n", (int)size);
	printf("errno = %d\n", errno);
	close(myfd);	
}
