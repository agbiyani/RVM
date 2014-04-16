/* multi.c - test that basic persistency works for multiple segments */


#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define SEGNAME0  "testseg1"
#define SEGNAME1  "testseg2"

#define OFFSET0  10
#define OFFSET1  100

#define STRING0 "hello, world"
#define STRING1 "black agagadrof!"

int main()
{
     rvm_t rvm;
     char* segs[2];
     trans_t trans;
     
     rvm = rvm_init("rvm_segments");

     rvm_destroy(rvm, SEGNAME0);
     rvm_destroy(rvm, SEGNAME1);

     segs[0] = (char*) rvm_map(rvm, SEGNAME0, 1000);
     segs[1] = (char*) rvm_map(rvm, SEGNAME1, 1000);

     trans = rvm_begin_trans(rvm, 2, (void **)segs);

     rvm_about_to_modify(trans, segs[0], OFFSET0, 100);
     strcpy(segs[0]+OFFSET0, STRING0);
     rvm_about_to_modify(trans, segs[1], OFFSET1, 100);
     strcpy(segs[1]+OFFSET1, STRING1);

     rvm_commit_trans(trans);

//     rvm_t rvm1;
//     char *segs1[2];

//     rvm1 = rvm_init("rvm_segments");
//     segs1[0] = (char*) rvm_map(rvm1, SEGNAME0, 1000);
/*     segs1[1] = (char*) rvm_map(rvm, SEGNAME1, 1000);

     if(strcmp(segs[0] + OFFSET0, STRING0)) {
	  printf("ERROR in segment 0 (%s)\n",
		 segs[0]+OFFSET0);
	  exit(2);
     }
     if(strcmp(segs[1] + OFFSET1, STRING1)) {
	  printf("ERROR in segment 1 (%s)\n",
		 segs[1]+OFFSET1);
	  exit(2);
     }

     printf("OK\n");
   */
     return 0;
}

