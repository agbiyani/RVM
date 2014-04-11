#ifndef __LIBRVM__
#define __LIBRVM__

//#include "rvm_internal.h"

typedef int trans_t;
typedef struct rvm_t
{
	char* directory;

}rvm_t;

rvm_t rvm_init(const char *directory);

void *rvm_map(rvm_t rvm, const char *segname, int size_to_create);
void rvm_unmap(rvm_t rvm, void *segbase);
void rvm_destroy(rvm_t rvm, const char *segname);
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases);
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size);
//void rvm_commit_trans(trans_t tid);
void rvm_abort_trans(trans_t tid);
/*void rvm_truncate_log(rvm_t rvm);
*/
#endif