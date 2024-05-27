#include "storage_mgr.c"


/* manipulating page files */
extern void initStorageManager (void){return RC_OK;}
extern RC createPageFile (char *fileName){return RC_OK;}
extern RC openPageFile (char *fileName, SM_FileHandle *fHandle){return RC_OK;}
extern RC closePageFile (SM_FileHandle *fHandle){return RC_OK;}
extern RC destroyPageFile (char *fileName){return RC_OK;}

/* reading blocks from disc */
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern int getBlockPos (SM_FileHandle *fHandle){return RC_OK;}
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}

/* writing blocks to a page file */
extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC appendEmptyBlock (SM_FileHandle *fHandle){return RC_OK;}
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){return RC_OK;}