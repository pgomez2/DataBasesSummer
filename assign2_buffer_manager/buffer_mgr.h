#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H
#include "storage_mgr.h"

// Include return codes and methods for logging errors
#include "dberror.h"

// Include bool DT
#include "dt.h"

// Replacement Strategies
typedef enum ReplacementStrategy {
	RS_FIFO = 0,
	RS_LRU = 1,
	RS_CLOCK = 2,
	RS_LFU = 3,
	RS_LRU_K = 4
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1

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
	Queue *arrivalOrder; // vector to track the order of arrival of pages
	LRUCache *usageOrder; // vector to track the order of usage of pages

} BM_BufferPool;

typedef struct BM_PageHandle {
	PageNumber pageNum; // position of page located inside of the page file 
	char *data; //pointer to the page inside of the memory 
	PageNumber pageNumBuffer; // position of the page inside of the buffer 
	char *dataBuffer; // pointer to the page inside of the Buffer
	bool isDirty; // variable to check if the page is dirty 
	int fixCount; // variable to check the number of clients are calling for the page
} BM_PageHandle;

typedef struct LRUNode {
    int pageNum;
    struct LRUNode *prev, *next;
} LRUNode;

typedef struct LRUCache {
    int capacity, count;
    LRUNode *head, *tail;
    LRUNode** hashTable; // to quickly access nodes
} LRUCache;


typedef struct Node {
    int pageNum;
    struct Node* next;
} Node;

typedef struct Queue {
    Node *front, *rear;
    int size;
} Queue;




// convenience macros
#define MAKE_POOL()					\
		((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()				\
		((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		const int numPages, ReplacementStrategy strategy,
		void *stratData);
RC shutdownBufferPool(BM_BufferPool *const bm);
RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
		const PageNumber pageNum);

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm);
bool *getDirtyFlags (BM_BufferPool *const bm);
int *getFixCounts (BM_BufferPool *const bm);
int getNumReadIO (BM_BufferPool *const bm);
int getNumWriteIO (BM_BufferPool *const bm);


#endif
