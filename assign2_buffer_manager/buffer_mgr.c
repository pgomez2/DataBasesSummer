#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <stdlib.h>
#include <string.h>

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		const int numPages, ReplacementStrategy strategy,
		void *stratData){
			initStorageManager();
			
			SM_FileHandle fh; 
			// First we have to check if the file exist and also get the pointer 
			openPageFile((char*)pageFileName, &fh);

			/* Now we have to allocate all the info */
			
			//this is the Buffer Manager itself, the memeory, the place where we save the pages
			char *BufferManagerMemory  = (char*)calloc(PAGE_SIZE*numPages, sizeof(char));

			//save the page file name
			bm->pageFile = (char*)malloc(strlen(pageFileName) + 1);
			strcpy(bm->pageFile, pageFileName);

			//saving the number of pages
			bm->numPages = numPages;
			//saving the strategy
			bm->strategy = strategy;
			// k parameter 
			if(stratData != NULL){
			bm->mgmtData = BufferManagerMemory;
			}
			// Inicializate the statistics of the buffer in 0
			bm->readIO =0;  //number of times a page  is read
			bm->writeIO = 0; // number of times a page is writed 
			
			// Inicializate the order with -1 to ensure there are no mistakes 
			//memory assignation
			int* orderBuffer = (int*)malloc(numPages* sizeof(int));
			//inicializate in -1 to minimiate mistakes 
			for (int i = 0; i<numPages; i++){ orderBuffer[i]=-1;}
			// save in the metadatada 
			bm->orderBuffer = orderBuffer; 

			//save the data strcutures 
			bm->fh = fh;
			bm->ph = (SM_PageHandle) malloc(PAGE_SIZE);
			

			// print initBufferPool sucessfully if everything is fine 
			printf("initBufferPool sucessfully");
			return RC_OK;
			}



RC shutdownBufferPool(BM_BufferPool *const bm){return RC_OK;}
RC forceFlushPool(BM_BufferPool *const bm){return RC_OK;}

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){return RC_OK;}
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){return RC_OK;}
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){return RC_OK;}

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
		const PageNumber pageNum){
			readBlock(pageNum,&bm->fh, bm->ph);
			
			return RC_OK;
			}

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm){return RC_OK;}
bool *getDirtyFlags (BM_BufferPool *const bm){return RC_OK;}
int *getFixCounts (BM_BufferPool *const bm){return RC_OK;}
int getNumReadIO (BM_BufferPool *const bm){return RC_OK;}
int getNumWriteIO (BM_BufferPool *const bm){return RC_OK;}