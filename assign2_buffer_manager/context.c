
typedef struct BM_BufferPool {
	char *pageFile; // pointer to the page file 
	int numPages; //number of pages of the buffer can hold at the same time 
	ReplacementStrategy strategy; //strategy to change pages in the buffer pool
	void *mgmtData; // use this one to store the bookkeeping info your buffer
	// manager needs for a buffer pool
	int readIO; //number of times a page is read 
	int writeIO; // number of times a page is writed 
	int *orderBuffer; // vector to see the order of the pages in the buffer 
	SM_FileHandle fh; //use the previous datastructures to be able to use the previous functions  
	SM_PageHandle ph;
	//Queue *arrivalOrder; // vector to track the order of arrival of pages
	//LRUCache *usageOrder; // vector to track the order of usage of pages

} BM_BufferPool;

typedef struct BM_PageHandle {
	PageNumber pageNum; // position of page located inside of the page file 
	char *data; //pointer to the page inside of the memory 
	PageNumber pageNumBuffer; // position of the page inside of the buffer 
	char *dataBuffer; // pointer to the page inside of the Buffer
	bool isDirty; // variable to check if the page is dirty 
	int fixCount; // variable to check the number of clients are calling for the page
} BM_PageHandle;


typedef struct SM_FileHandle {
	char *fileName;
	int totalNumPages;
	int curPagePos;
	void *mgmtInfo;
} SM_FileHandle;

typedef char* SM_PageHandle;


extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
    
    // Inicializate a File pointer with the direction of the file is gonna be use it.
    FILE* file = (FILE*)fHandle -> mgmtInfo;

    //with fseek get the direction of the pointer, the -1 is because C start to count in zero, so the first block is 0
    if (fseek(file, PAGE_SIZE*(pageNum-1), SEEK_SET) != 0) {
        return RC_FILE_NOT_FOUND; // fseek failed to set position to the beginning
    }

    
    //read the file and copy the elements top, the info is storage in the
    //memory direction of memPage 
    fread(memPage,PAGE_SIZE,1, file);
    
    return RC_OK;    
    }


extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
            
        
    // Inicializate a File pointer with the direction of the file is gonna be use it.
    FILE* file = (FILE*)fHandle -> mgmtInfo;

    //with fseek get the direction of the pointer, so the first block is 0 
    if (fseek(file, PAGE_SIZE*((fHandle->curPagePos)-1), SEEK_SET) != 0) {
        return RC_FILE_NOT_FOUND; // fseek failed to set position to the beginning
    }

    
    //with the position that have to be modified, modfied it, the information is in memPage
    size_t writeResult = fwrite(memPage,sizeof(char) ,PAGE_SIZE, file); 
    if (writeResult < PAGE_SIZE) {
        return RC_WRITE_FAILED;  // fwrite did not write the expected amount of data
    }
    
    
    return RC_OK;
    }


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


			// Allocate and initialize arrivalOrder and usageOrder
			int* arrivalOrder = (int*)malloc(10000 * sizeof(int));
			int* usageOrder = (int*)malloc(10000 * sizeof(int));
			for (int i = 0; i < numPages; i++){
				arrivalOrder[i] = -1;
				usageOrder[i] = -1;
			}
			bm->arrivalOrder = arrivalOrder;
			bm->usageOrder = usageOrder;
			

			// print initBufferPool sucessfully if everything is fine 
			printf("initBufferPool sucessfully\n");
			return RC_OK;
			}
Esto es contexto, no quiero que hagas nada 