/* basic.c - test that basic persistency works */
#ifndef RVM_H
#define RVM_H
#include "rvm.h"
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define TEST_STRING "hello, world"
#define OFFSET2 1000


int main(int argc, char **argv)
{
     rvm_t rvm;
     trans_t trans;
     char* segs[1];
     rvm = rvm_init("rvm_segments");
//     rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     printf("Seg[0] = %s \n", segs[0]);

     
     trans = rvm_begin_trans(rvm, 1, (void **) segs);
     printf("1. trans = %d \n", trans);
     rvm_about_to_modify(trans, segs[0], 0, 100);


     //sprintf(segs[0], TEST_STRING);
	strcat(segs[0], TEST_STRING);
     
     printf("after writing Seg[0] = %s \n", segs[0]);
  /*   rvm_about_to_modify(trans, segs[0], OFFSET2, 100);
     sprintf(segs[0]+OFFSET2, TEST_STRING);
*/
//     rvm_abort_trans(trans);     
//     printf("after abort_trans  Seg[0] = %s \n", segs[0]);
  /*   rvm_commit_trans(trans);

     abort();
     */
     return 0;
     }
