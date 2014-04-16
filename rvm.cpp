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
#include <boost/algorithm/string.hpp>
#include <vector>
#include <map>

const char * slash = "/";
const char * segExt = ".seg";
const char * logExt = ".log";

trans_t tid_count = 0;
int rid_count = 0;
int rvm_id_count = 0;

typedef struct segment_t
{
	char *segname;
	char *backingFile;
	int size;
	void *segbase;	
}segment_t;


typedef struct region_t
{
	/*Not sure if id is required*/
	int rid;
	void * segbase;
	const char* segname;
	char* logFile;
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
	char *directory;
}transaction_t;

list<transaction_t*> transactionList;
map<void*, const char*> segmap;
map<int, list<segment_t*> > rvmmap;

/*
Helper functions start here.
*/
list<segment_t*> getSegList(int rvm_id)
{
	list<segment_t*> seglist;
	for(map<int, list<segment_t*> >::iterator it = rvmmap.begin(); it != rvmmap.end(); it++)
	{
		if(it->first == rvm_id)
			return it->second;	
	}
	return seglist;	
}

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
	char*filename = (char*) malloc(sizeof(dir) + sizeof(slash) + sizeof(segname) + sizeof(segExt) + 1);
	strcpy(filename, dir);
	strcat(filename, slash);
	strcat(filename, segname);
	strcat(filename, segExt);
	return filename;
}

char* constructLogFileName(const char* segname, char* dir)
{	
	char*filename = (char*) malloc(sizeof(dir) + sizeof(slash) + sizeof(segname) + sizeof(logExt) + 1);
	strcpy(filename, dir);
	strcat(filename, slash);
	strcat(filename, segname);
	strcat(filename, logExt);
	return filename;
}

bool doesSegmentExist(const char* segname, rvm_t rvm)
{
	bool segmentExists = false;
	list<segment_t*> seglist = getSegList(rvm.id);
	for(list<segment_t *>::iterator it = seglist.begin(); it != seglist.end(); it++)
	{
		segment_t *seg = *it;
		if(strcmp(seg->segname, segname) == 0)
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

const char* getSegmentName(void* segbase)
{
	for(map<void*, const char*>::iterator it = segmap.begin(); it != segmap.end(); it++)
	{
		//seg = *it;
		if(it->first == segbase)
		{	
			return it->second;
		}
	}	
	return "";
}

void playLogToVirtualSegment(void *segbase, const char *segname, char* directory, int size)
{
	char * logFilename = constructLogFileName(segname, directory);
	if(doesFileExist(logFilename))
	{
		//cout << "Log file exists for the segment " << segname << endl;
		FILE * fd = fopen(logFilename, "r");
		if(fd == NULL)
		{
			printf("ERROR: while opening the logfile %s \n", logFilename);
			exit(1);	
		}
		/*Need some type of looping here*/
		while(fgetc(fd) != EOF)
		{
			char* line = NULL ;
			size_t len = 0;
			getline(&line, &len, fd);
			vector<string> strs;
			boost::split(strs, line, boost::is_any_of("\n="));
			int offset = atoi(strs.at(1).c_str());
			getline(&line, &len, fd);
			boost::split(strs, line, boost::is_any_of("\n="));
			int size = atoi(strs.at(1).c_str());
			fread((char*)segbase + offset, sizeof(char), size, fd);
			/*Read \n preceding the #*/
			fgetc(fd);
			/*Read #*/
			fgetc(fd);
			/*Read \n succeeding the #*/
			fgetc(fd);
		}
		fclose(fd);		
	}
}


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
	rvm.id = ++rvm_id_count;
	rvm.directory = (char*)malloc(strlen(directory) + 1);
	strcpy(rvm.directory, directory);
	list<segment_t*> seglist;
	rvmmap.insert(pair<int, list<segment_t*> >(rvm.id, seglist));
	return rvm;
}

/*
1. Destrop a segment completely - Erasing its backing store.
2. This function should not be called on a segment that is currently mapped.
*/
void rvm_destroy(rvm_t rvm, const char *segname)
{
	/*Check if this segment is mapped*/
	if(doesSegmentExist(segname, rvm))
	{
		printf("ERROR: Segment %s is currently mapped, so cannot be destroyed.\n", segname);	
		exit(1);
	}	
	else
	{
		char* backingFileName = constructSegmentFileName(segname, rvm.directory);
		remove(backingFileName);	
	}
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
	if(doesSegmentExist(segname, rvm))
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
		char * segFilename = constructSegmentFileName(segname, rvm.directory);
		if(doesFileExist(segFilename))
		{
			//cout << "Backing file exists" << endl;
			FILE * bfd;
			bfd = fopen(segFilename, "rb+");
			fread(segbase, 1, size_to_create, bfd);	
			/*TODO: Then apply logs to the virtual segment if any log file exists*/
			playLogToVirtualSegment(segbase, segname, rvm.directory, size_to_create);
//			cout << segbase << endl;
//			printf("From rvm_map : %s\n", (char*)segbase);
			fclose(bfd);
		}
		else
		{
			//cout << "Backing file does not exist" << endl;
			/*Create the seg file*/
			ofstream o(segFilename);
			o.close();			
		}

		segment_t *seg = new segment_t;
		seg->size = size_to_create;
		seg->backingFile = (char*)malloc(strlen(segFilename) + 1);
		strcpy(seg->backingFile, segFilename);
		seg->segname = (char*)malloc(strlen(segname) + 1);
		strcpy(seg->segname, segname);
		seg->segbase = segbase;

		/*We need to push it into the seglist corresponding to this rvm in the map*/
		list<segment_t*> seglist = getSegList(rvm.id);	
		//cout << "In map, seglist size = " << seglist.size() << endl;
		seglist.push_back(seg);
		rvmmap[rvm.id] = seglist;
		segmap.insert(pair<void *, const char*>(segbase, segname));
		//cout << "Returning" << endl;
		return segbase;
	}
}

/*
1. Unmap a segment from memory
2. Remove it from seglist.
3. remove it from segmap.
*/
void rvm_unmap(rvm_t rvm, void * segbase)
{	
	segment_t *seg;
	list<segment_t*> seglist = getSegList(rvm.id);
	for(list<segment_t*>::iterator it = seglist.begin(); it != seglist.end(); it++)
	{
		seg = *it;
		if(seg->segbase == segbase)
		{	
			seglist.erase(it);
			free(seg->segbase);
			break;
		}
	}
	rvmmap[rvm.id] = seglist;	
	for(map<void*, const char*>::iterator iter = segmap.begin(); iter != segmap.end(); iter++)
	{
		if(iter->first == segbase)
		{
			segmap.erase(iter);
			break;	
		}
	}
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
	transaction->directory = (char*)malloc(strlen(rvm.directory) + 1);
	strcpy(transaction->directory, rvm.directory);
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
	//cout << "In about to modify";
	region_t *region = new region_t;
	region->rid = ++rid_count;
	region->segbase = segbase;
	region->offset = offset;
	region->size = size;
	region->undoLog = (void*) malloc(size);
	region->segname = getSegmentName(segbase);
	memcpy(region->undoLog, (char*)segbase+offset, size);
	list<transaction_t*>::iterator it = searchTransaction(tid);
	transaction_t * transaction = *it;
	(transaction->regionList).push_back(region);
	//cout << "About to modify transaction.regionList.size() = " << (transaction->regionList).size() << endl;
}


/*
1. Apply the undoLog in reverse order of their creation.
2. Delete the regions
3. Delete the transaction from the transaction list.
*/
void rvm_abort_trans(trans_t tid)
{
	//cout << "In rvm_abort_trans" << endl;
	//transaction_t *transaction = searchTransaction(tid);
	list<transaction_t*>::iterator it = searchTransaction(tid);
	transaction_t *transaction = *it;
	//cout << "Transaction id = " << transaction->tid << endl;
	list<region_t*> region_list = transaction->regionList;
	//cout << "Abort_trans transaction.regionList.size() = " << (transaction->regionList).size() << endl;
	//cout << "region_list size  = " << region_list.size() << endl;
	/*traversing in reverse so that we can effectively reach the state of begin_transaction.*/
	for(list<region_t*>::reverse_iterator riter = region_list.rbegin(); riter != region_list.rend(); riter++)
	{
		//cout << "In for " << endl;
		region_t *region = *riter;
//		printf("region.undoLog = %s \n", (char*) region->undoLog);
		memcpy((char*)region->segbase + region->offset, region->undoLog, region->size);
	}
	/*Free up the region list*/
	transaction->regionList.clear();

	/*Delete the transaction from the transaction list*/
	transactionList.erase(it);
}

/*

*/
void rvm_commit_trans(trans_t tid)
{
	/*Get the transaction corresponding to the transaction id tid*/
	list<transaction_t*>::iterator it = searchTransaction(tid);
	transaction_t *transaction = *it;
	//cout << "Transaction id = " << transaction->tid << endl;

	/*Get the region_list for this transaction*/
	list<region_t*> region_list = transaction->regionList;
	//cout << "Commit trans transaction.regionList.size() = " << (transaction->regionList).size() << endl;
	//cout << "region_list size = " << region_list.size() << endl; 

	/*
	1. Traverse the region list.
	2. Fetch the segment name that this region belongs to.
	3. Push the changes to the logfile.
	*/
	for(list<region_t*>::iterator iter = region_list.begin(); iter != region_list.end(); iter++)
	{
		region_t *region = *iter;
		//const char * segname = getSegmentName(region->segbase);
		char *logFilename = constructLogFileName(region->segname, transaction->directory);
		FILE * fd = fopen(logFilename, "a");
		fprintf(fd, "offset=%d\n", region->offset);
		fprintf(fd, "size=%d\n", region->size);
		//fwrite(region->segbase + region->offset, sizeof(char), region->size, fd);
		fclose(fd);
		int myfd = open(logFilename, O_WRONLY);
		lseek(myfd, 0, SEEK_END);
		write(myfd, (char*)region->segbase + region->offset, region->size);
		write(myfd, "\n#\n", 3);
		//fprintf(fd, "\n#\n");
		close(myfd);

/*		char * offset;
		char * size;
		sprintf(offset, "offset=%d\n", region->offset);
		sprintf(size, "size=%d\n", region->size);
		int myfd = open(logFilename, O_CREAT | O_WRONLY);
		write(myfd, offset, strlen(offset));
		write(myfd, size, strlen(size));
		write(myfd, region->segbase + region->offset, region->size);
		write(myfd, "\n#\n", 3);
		close(myfd);
*/	}

	/*Free up the region list*/
	transaction->regionList.clear();
	/*Delete the transaction from the transaction list*/
	transactionList.erase(it);	
}

/*

*/
void rvm_truncate_log(rvm_t rvm)
{
	/*We need to do it segment by segment*/
	list<segment_t*> seglist = getSegList(rvm.id);
	for(list<segment_t*>::iterator it = seglist.begin(); it != seglist.end(); it++)
	{
		int bfd;
		segment_t* seg = *it;
		char* logFileName = constructLogFileName(seg->segname, rvm.directory);
		char* backingFileName = constructSegmentFileName(seg->segname, rvm.directory);
		//bfd = open(seg->backingFile, O_WRONLY);	
		bfd = open(backingFileName, O_CREAT | O_WRONLY, 0777);	
		if(bfd == -1)
		{
			printf("ERROR: error while opening the backing file for segment %s in write mode\n", seg->segname);
			exit(1);	
		}
		FILE * lfd = fopen(logFileName, "r");
		if(lfd == NULL)
		{
			printf("ERROR: while opening the logfile %s \n", logFileName);
			exit(1);	
		}
		/*Need some type of looping here*/
		while(fgetc(lfd) != EOF)
		{
			/*Reading one log block from the file*/
			char* line = NULL ;
			size_t len = 0;
			getline(&line, &len, lfd);
			vector<string> strs;
			boost::split(strs, line, boost::is_any_of("\n="));
			int offset = atoi(strs.at(1).c_str());
			getline(&line, &len, lfd);
			boost::split(strs, line, boost::is_any_of("\n="));
			int size = atoi(strs.at(1).c_str());
			void * buf = malloc(size);
			fread(buf, sizeof(char), size, lfd);
			/*Read \n preceding the #*/
			fgetc(lfd);
			/*Read #*/
			fgetc(lfd);
			/*Read \n succeeding the #*/
			fgetc(lfd);

			/*Write data to the backing file*/
			lseek(bfd, 0, SEEK_SET);
			lseek(bfd, offset, SEEK_SET);
			write(bfd, buf, size);
		}
		fclose(lfd);	
		close(bfd);
		/*Delete the corresponding logfile*/
		remove(logFileName);
	}		
}
