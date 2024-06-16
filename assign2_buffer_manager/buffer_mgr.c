#include "buffer_mgr.h"
#include "storage_mgr.h"

// initBufferPool(bm, "testbuffer.bin", 3, RS_FIFO, NULL)

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		const int numPages, ReplacementStrategy strategy,
		void *stratData){
			
			// First we have to check if the file exist and also get the pointer 
			FILE *file = fopen(pageFileName, "rb+");
			if(file ==NULL){
				return RC_FILE_NOT_FOUND;
			}
			
			/* Now we have to allocate all the info */
			
			//save the pointer to the file 
			bm->pageFile = file;
			//saving the number of pages
			bm->numPages = numPages;
			//saving the strategy
			bm->strategy = strategy;
			// k parameter 
			if(stratData != NULL){
			bm->mgmtData = stratData;
			}
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


			return RC_OK;
			}

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm){return RC_OK;}
bool *getDirtyFlags (BM_BufferPool *const bm){return RC_OK;}
int *getFixCounts (BM_BufferPool *const bm){return RC_OK;}
int getNumReadIO (BM_BufferPool *const bm){return RC_OK;}
int getNumWriteIO (BM_BufferPool *const bm){return RC_OK;}