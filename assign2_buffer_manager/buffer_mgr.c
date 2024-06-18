#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>


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


			
    // Initialize arrivalOrder as a Queue and usageOrder as an LRU Cache
    		bm->arrivalOrder = createQueue();
    		bm->usageOrder = createLRUCache(numPages);

			

			// print initBufferPool sucessfully if everything is fine 
			printf("initBufferPool sucessfully\n");
			return RC_OK;
			}



RC shutdownBufferPool(BM_BufferPool *const bm){return RC_OK;}
RC forceFlushPool(BM_BufferPool *const bm){return RC_OK;}

// Buffer Manager Interface Access Pages
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
    // Check if the page is in the buffer pool
    for (int i = 0; i < bm->numPages; i++) {
        if (bm->orderBuffer[i] == page->pageNum) {
            // Mark the page as dirty
            page->isDirty = true;
            return RC_OK;
        }
    }
    // If the page is not found in the buffer pool, return an error
    return RC_FILE_NOT_FOUND;
}

RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    // Check if the page is in the buffer pool
    for (int i = 0; i < bm->numPages; i++) {
        if (bm->orderBuffer[i] == page->pageNum) {
            // Decrement the fix count
            if (page->fixCount > 0) {
                page->fixCount--;
            }
            return RC_OK;
        }
    }
    // If the page is not found in the buffer pool, return an error
    return RC_FILE_NOT_FOUND;
}

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){return RC_OK;}

#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <stdlib.h>
#include <string.h>

// Helper function to find a page in the buffer pool
int findPageInBufferPool(BM_BufferPool *const bm, PageNumber pageNum) {
    for (int i = 0; i < bm->numPages; i++) {
        if (bm->orderBuffer[i] == pageNum) {
            return i;
        }
    }
    return -1;
}

// Helper function to find a free frame or a victim frame
int findFreeOrVictimFrame(BM_BufferPool *const bm) {
    // Check for a free frame
    for (int i = 0; i < bm->numPages; i++) {
        if (bm->orderBuffer[i] == -1) {
            return i;
        }
    }

    // No free frame, use replacement strategy to find a victim
    int victimFrame = -1;
    switch (bm->strategy) {
        case RS_FIFO:
            victimFrame = dequeue(bm->arrivalOrder);
            break;
        case RS_LRU:
            victimFrame = dequeue(bm->usageOrder);
            break;
        // Add cases for other strategies if needed
        default:
            victimFrame = dequeue(bm->arrivalOrder);
            break;
    }
    return victimFrame;
}

// Helper function to write a dirty page back to disk
RC writeDirtyPageToDisk(BM_BufferPool *const bm, int frameIndex) {
    if (bm->ph[frameIndex].isDirty) {
        SM_FileHandle fh = bm->fh;
        SM_PageHandle ph = bm->ph[frameIndex].dataBuffer;
        if (writeBlock(bm->ph[frameIndex].pageNum, &fh, ph) != RC_OK) {
            return RC_WRITE_FAILED;
        }
        bm->writeIO++;
        bm->ph[frameIndex].isDirty = false;
    }
    return RC_OK;
}

// Implement the pinPage function
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {
    // Check if the page is already in the buffer pool
    int frameIndex = findPageInBufferPool(bm, pageNum);
    if (frameIndex != -1) {
        // Page is already in buffer pool, increment fix count and return
        bm->ph[frameIndex].fixCount++;
        page->pageNum = pageNum;
        page->data = bm->ph[frameIndex].dataBuffer;
        return RC_OK;
    }

    // Page is not in buffer pool, find a free frame or a victim frame
    frameIndex = findFreeOrVictimFrame(bm);
    if (frameIndex == -1) {
        return RC_FILE_NOT_FOUND;
    }

    // If the victim page is dirty, write it back to disk
    if (bm->orderBuffer[frameIndex] != -1) {
        if (writeDirtyPageToDisk(bm, frameIndex) != RC_OK) {
            return RC_WRITE_FAILED;
        }
    }

    // Read the requested page into the selected frame
    SM_FileHandle fh = bm->fh;
    SM_PageHandle ph = (SM_PageHandle)malloc(PAGE_SIZE);
    if (readBlock(pageNum, &fh, ph) != RC_OK) {
        free(ph);
        return RC_FILE_NOT_FOUND;
    }
    bm->readIO++;

    // Update the buffer pool metadata
    bm->ph[frameIndex].pageNum = pageNum;
    bm->ph[frameIndex].dataBuffer = ph;
    bm->ph[frameIndex].fixCount = 1;
    bm->ph[frameIndex].isDirty = false;
    bm->orderBuffer[frameIndex] = pageNum;

    // Enqueue the frame in the appropriate queue based on the replacement strategy
    if (bm->strategy == RS_FIFO) {
        enqueue(bm->arrivalOrder, frameIndex);
    } else if (bm->strategy == RS_LRU) {
        enqueue(bm->usageOrder, frameIndex);
    }

    // Set the page handle to point to the newly loaded page
    page->pageNum = pageNum;
    page->data = ph;

    return RC_OK;
}

}



// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm){return RC_OK;}
bool *getDirtyFlags (BM_BufferPool *const bm){return RC_OK;}
int *getFixCounts (BM_BufferPool *const bm){return RC_OK;}
int getNumReadIO (BM_BufferPool *const bm){return RC_OK;}
int getNumWriteIO (BM_BufferPool *const bm){return RC_OK;}




Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}

void enqueue(Queue* q, int pageNum) {
    Node* temp = (Node*)malloc(sizeof(Node));
    temp->pageNum = pageNum;
    temp->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        q->size++;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
    q->size++;
}

int dequeue(Queue* q) {
    if (q->front == NULL)
        return -1;
    int pageNum = q->front->pageNum;
    Node* temp = q->front;
    q->front = q->front->next;
    if (q->front == NULL)
        q->rear = NULL;
    free(temp);
    q->size--;
    return pageNum;
}

bool isQueueEmpty(Queue* q) {
    return q->front == NULL;
}



LRUCache* createLRUCache(int capacity) {
    LRUCache* cache = (LRUCache*)malloc(sizeof(LRUCache));
    cache->capacity = capacity;
    cache->count = 0;
    cache->head = cache->tail = NULL;
    cache->hashTable = (LRUNode**)malloc(capacity * sizeof(LRUNode*));
    for (int i = 0; i < capacity; i++)
        cache->hashTable[i] = NULL;
    return cache;
}

void moveToHead(LRUCache* cache, LRUNode* node) {
    if (cache->head == node)
        return;
    if (cache->tail == node)
        cache->tail = node->prev;
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    node->next = cache->head;
    node->prev = NULL;
    if (cache->head)
        cache->head->prev = node;
    cache->head = node;
    if (!cache->tail)
        cache->tail = node;
}

void addToCache(LRUCache* cache, int pageNum) {
    LRUNode* node = (LRUNode*)malloc(sizeof(LRUNode));
    node->pageNum = pageNum;
    node->prev = node->next = NULL;
    if (cache->count < cache->capacity) {
        moveToHead(cache, node);
        cache->hashTable[pageNum % cache->capacity] = node;
        cache->count++;
    } else {
        LRUNode* tail = cache->tail;
        cache->tail = tail->prev;
        if (cache->tail)
            cache->tail->next = NULL;
        free(tail);
        moveToHead(cache, node);
        cache->hashTable[pageNum % cache->capacity] = node;
    }
}
