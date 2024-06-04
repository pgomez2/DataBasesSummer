#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include "dberror.h"

/************************************************************
 *                    handle data structures                *
 ************************************************************/
typedef struct SM_FileHandle {
	char *fileName;
	int totalNumPages;
	int curPagePos;
	void *mgmtInfo;
} SM_FileHandle;

typedef char* SM_PageHandle;

/************************************************************
 *                    interface                             *
 ************************************************************/
/* manipulating page files */
extern void initStorageManager (void); //made
extern RC createPageFile (char *fileName); //made
extern RC openPageFile (char *fileName, SM_FileHandle *fHandle); //made
extern RC closePageFile (SM_FileHandle *fHandle); //made 
extern RC destroyPageFile (char *fileName); //made 

/* reading blocks from disc */
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage); //made
extern int getBlockPos (SM_FileHandle *fHandle); //made
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage); //made
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage); //made
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage); //made
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage); //made
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage); //made

/* writing blocks to a page file */
extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage); //made 
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC appendEmptyBlock (SM_FileHandle *fHandle);
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle);

#endif
