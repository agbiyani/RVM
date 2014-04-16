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
     char* segs[2];
     rvm = rvm_init("rvm_segments");
     segs[0] = (char *) rvm_map(rvm, "testseg1", 10000);
     segs[1] = (char *) rvm_map(rvm, "testseg2", 10000);
     
     trans = rvm_begin_trans(rvm, 2, (void **) segs);

     rvm_about_to_modify(trans, segs[0], 0, 100);
     sprintf(segs[0], TEST_STRING);
     rvm_about_to_modify(trans, segs[1], OFFSET2, 100);
     sprintf(segs[1]+OFFSET2, TEST_STRING);
     printf("Segs[0] = %s", segs[0]);
     printf("Segs[1] = %s", segs[1]);

     rvm_commit_trans(trans);

   rvm_truncate_log(rvm);
    
/*     rvm_unmap(rvm, segs[0]);
     rvm_destroy(rvm, "testseg");
  */   return 0;
}
