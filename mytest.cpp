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
	char * seg1 = (char*) malloc(100 * sizeof(char));
	char * helloworld = "Hello World";
	char * aabhabiyani = "Aabha Biyani";
	size_t size = 0;
	strcpy(seg, helloworld);
	strcpy(seg1, aabhabiyani);
	cout << seg << endl;
	int myfd = open("mytest.log", O_WRONLY, 0777);
	lseek(myfd, 100, SEEK_SET);
	printf("fd = %d\n", myfd);
	size = write(myfd, seg, 100);
	size = write(myfd, seg1, 100);
//	size = write(myfd, seg, 100);
	printf("size = %d\n", (int)size);
	printf("errno = %d\n", errno);
	close(myfd);	
}
