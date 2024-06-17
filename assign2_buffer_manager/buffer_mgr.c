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

RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {

    // 1. Verificar si la página ya está en el buffer pool
    for (int i = 0; i < bm->numPages; i++) {
        if (bm->orderBuffer[i] == pageNum) {
            // Página encontrada en el buffer pool
            page->pageNum = pageNum;
            page->data = bm->mgmtData + (i * PAGE_SIZE);
            page->isDirty = false; // Puede ajustarse según tu lógica
            page->fixCount++; // Incrementar el contador de fijación
            return RC_OK;
        }
    }

    // 2. Si la página no está en el buffer, cargarla desde el archivo
    // Encontrar una posición libre en el buffer pool
    int freePage = -1;
    for (int i = 0; i < bm->numPages; i++) {
        if (bm->orderBuffer[i] == -1) {
            freePage = i;
            break;
        }
    }


return RC_OK;
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
