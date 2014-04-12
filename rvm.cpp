using namespace std;
#ifndef RVM_H
#define RVM_H
#include "rvm.h"
#endif
#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <list>
#include <fstream>
#include <sys/mman.h>

const char * slash = "/";
const char * ext = ".seg";

trans_t tid_count = 0;
int rid_count = 0;

typedef struct segment_t
{
	char* segname;
	char* backingFile;
	int size;
	void *segbase;
}segment_t;

typedef struct region_t
{
	/*Not sure if id is required*/
	int rid;
	void * segbase;
	int offset;
	int size;
	void * undoLog;
}region_t;

typedef struct transaction_t
{
	trans_t tid;
	list<region_t*> regionList;
	void **segbases;
	int numSegments;
}transaction_t;

list<segment_t> seglist;
list<transaction_t*> transactionList;

/*
Helper functions start here.
*/
bool doesFileExist(char * filename)
{
	bool exists = false;
	struct stat st;
	if(stat(filename, &st) == 0)
		exists = true;
	return exists;
}

bool doesDirectoryExist(const char* directory)
{
	bool dirExists = false;
	/*Checking if Directory Exists*/
	struct stat st;
	if(stat(directory, &st) == 0)
		if((st.st_mode) & (S_IFDIR != 0))
			dirExists = true;
	return dirExists;
}

char* constructSegmentFileName(const char* segname, char* dir)
{	
	char*filename = (char*) malloc(sizeof(dir) + sizeof(slash) + sizeof(segname) + sizeof(ext) + 1);
	strcpy(filename, dir);
	strcat(filename, slash);
	strcat(filename, segname);
	strcat(filename, ext);
	return filename;
}

bool doesSegmentExist(const char* segname)
{
	bool segmentExists = false;
	for(list<segment_t>::iterator it = seglist.begin(); it != seglist.end(); it++)
	{
		segment_t seg = *it;
		if(strcmp(seg.segname, segname) == 0)
		{	
			segmentExists = true;	
			return segmentExists;
		}
	}	
	return segmentExists;
}

/*
Checks if the specified segments are already being modified by any transaction.
Returns false if any of the specified segments is already being modified (false meaning not okay to create the transaction)
Returns true otherwise.
*/
bool okayToCreateTransaction(int numsegs, void **segbases)
{
	bool okayToCreate = true;
	for(list<transaction_t*>::iterator it = transactionList.begin(); it != transactionList.end(); it++)
	{
		transaction_t *transaction = *it;
		int transNumSegs = transaction->numSegments;
		int count1 = 0, count2 = 0;
		while(count1 < transNumSegs)
		{
			while(count2 < numsegs)
			{
				if(segbases[count2] == transaction->segbases[count1])
				{
					okayToCreate = false;
					return okayToCreate;
				}
				count2++;	
			}
			count1++;
		}
	}
	return okayToCreate;	
}

list<transaction_t*>::iterator searchTransaction(trans_t tid)
{
	transaction_t *transaction;
	list<transaction_t*>::iterator it;
	for(it = transactionList.begin(); it != transactionList.end(); it++)
	{
		transaction = *it;
		if(transaction->tid == tid)
		{	
			//			return transaction;
			return it;
		}
	}	
//	return transaction;
	return it;
}

/*segment_t searchSegment(void* segbase)
{
	segment_t seg;
	for(list<segment_t>::iterator it = seglist.begin(); it != seglist.end(); it++)
	{
		seg = *it;
		if(seg.segbase == segbase)
		{	
			return seg;
		}
	}	
	return seg;
}
*/
/*
Helper functions end here.
*/

/*
1. Initialize the library with the particular directory as backing store.
*/
rvm_t rvm_init(const char*directory)
{
	rvm_t rvm;
	/*If directory does not exist then we create one*/
	if(!doesDirectoryExist(directory))
		mkdir(directory, 0700);
	/*Load the rvm object with the corresponding directory name*/		
	rvm.directory = (char*)malloc(sizeof(directory));
	strcpy(rvm.directory, directory);
	return rvm;
}

/*
1. Destrop a segment completely - Erasing its backing store.
2. This function should not be called on a segment that is currently mapped.
*/
void rvm_destroy(rvm_t rvm, const char *segname)
{
	/*Check if this segment is mapped
	if(mapped)
		error
	else
		delete the corresponding backing file plus log file.
	*/
}

/*
1. Map a segment from disc to memory.
2. If the segment does not already exist then create it and give it size size_to_create.
3. If the segment exists but is shorter than size_to_create, then extend it until it is long enough.
4. It is an error to map a same segment twice.
*/
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create)
{
	void *segbase;
	/*Check if the segment exists in list
		So we error out*/
	if(doesSegmentExist(segname))
	{
		cout << "ERROR: Segment " << segname << "is already mapped and cannot be mapped again." << endl;
		exit(1);	
	}		
	/*So, if the segment does not exist.*/
	else
	{
		/*Malloc memory*/
		segbase = malloc(size_to_create);
		/*Check if the segment backing file exists*/
		char * filename = constructSegmentFileName(segname, rvm.directory);
		if(doesFileExist(filename))
		{
			cout << "Backing file exists" << endl;
			FILE * bfd;
			bfd = fopen(filename, "rb+");
			fread(segbase, 1, size_to_create, bfd);	
			/*TODO: Then apply logs if any log file exists*/
//			cout << segbase << endl;
//			printf("From rvm_map : %s\n", (char*)segbase);
			fclose(bfd);
		}
		else
		{
			cout << "Backing file does not exist" << endl;
			/*Create the seg file*/
			ofstream o(filename);
			o.close();			
		}

		segment_t seg;
		seg.size = size_to_create;
		seg.backingFile = (char*)malloc(sizeof(filename) + 1);
		strcpy(seg.backingFile, filename);
		seg.segname = (char*)malloc(sizeof(segname) + 1);
		strcpy(seg.segname, segname);
		seg.segbase = segbase;
		seglist.push_back(seg);
		cout << "Returning" << endl;
		return segbase;
	}
}

/*
1. Unmap a segment from memory
2. Remove it from seglist.
*/
void rvm_unmap(rvm_t rvm, void * segbase)
{	
	segment_t seg;
	for(list<segment_t>::iterator it = seglist.begin(); it != seglist.end(); it++)
	{
		seg = *it;
		if(seg.segbase == segbase)
		{	
			seglist.erase(it);
		}
	}	
	free(seg.segbase);
}

/*
1. Create a transaction if none of the specified functions are being modified by any other transaction.
2. Populate it appropriately.
3. Add the created transaction to the transactionList.
*/
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases)
{
	/*TODO: Check if the segbases are mapped. If unmapped then error out. Not explicitly metnioned to check this. But would be a sane thing to do.*/
	/*Check if any other transaction is using any of these segments in segbases*/
	if(!okayToCreateTransaction(numsegs, segbases))
		return -1;
	/*Create an populate transaction_t*/
	transaction_t *transaction = new transaction_t;
	transaction->tid = ++tid_count;
	transaction->numSegments = numsegs;
	transaction->segbases = segbases;

	/*Add tansaction_t to the transactionList*/
	transactionList.push_back(transaction);

	/*Return the transaction id*/
	return transaction->tid;
}

/*
1. Create a region for the range specified in this method.
2. Create a duplicate copy in undoLog, if you need to abort.
*/
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size)
{
	cout << "In about to modify";
	region_t *region = new region_t;
	region->rid = ++rid_count;
	region->segbase = segbase;
	region->offset = offset;
	region->size = size;
	region->undoLog = (void*) malloc(size);
	memcpy(region->undoLog, segbase+offset, size);
	list<transaction_t*>::iterator it = searchTransaction(tid);
	transaction_t * transaction = *it;
	(transaction->regionList).push_back(region);
	cout << "About to modify transaction.regionList.size() = " << (transaction->regionList).size() << endl;
}


/*
1. Apply the undoLog.
2. Delete the regions
3. Delete the transaction from the transaction list.
*/
void rvm_abort_trans(trans_t tid)
{
	cout << "In rvm_abort_trans" << endl;
	//transaction_t *transaction = searchTransaction(tid);
	list<transaction_t*>::iterator it = searchTransaction(tid);
	transaction_t *transaction = *it;
	cout << "Transaction id = " << transaction->tid << endl;
	list<region_t*> region_list = transaction->regionList;
	cout << "Abort_trans transaction.regionList.size() = " << (transaction->regionList).size() << endl;
	cout << "region_list size  = " << region_list.size() << endl;
	for(list<region_t*>::iterator iter = region_list.begin(); iter != region_list.end(); iter++)
	{
		cout << "In for " << endl;
		region_t *region = *iter;
		printf("region.undoLog = %s \n", region->undoLog);
		memcpy(region->segbase + region->offset, region->undoLog, region->size);
	}
	/*Free up the region list*/
	transaction->regionList.clear();

	/*Delete the transaction from the transaction list*/
	transactionList.erase(it);
	tid_count--;
}
