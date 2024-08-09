#include "dberror.h"
#include "tables.h"
#include <math.h>
#include "btree_mgr.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "record_mgr.h"
#include "string.h"
#include <string.h>
#include <stdlib.h>





// init and shutdown index manager//
RC initIndexManager (void *mgmtData){
    initStorageManager();

    // Log the action, useful for debugging and verifying that the storage manager started correctly.
    printf("Storage manager has been initialized.\n");
    return RC_OK;
}



RC shutdownIndexManager (){
    shutdownStorageManager();

    // Log this action to provide a traceable output of the system's shutdown process.
    printf("Storage manager has been shut down.\n");
    
    return RC_OK;
}


char *serializeBtreeHeader(BTreeMtdt *mgmtData) {
    char *header = calloc(1, PAGE_SIZE);
    int offset = 0;
    *(int *) (header + offset) = mgmtData->n;
    offset += sizeof(int);
    *(int *) (header + offset) = mgmtData->keyType;
    offset += sizeof(int);
    *(int *) (header + offset) = mgmtData->nodes;
    offset += sizeof(int);
    *(int *) (header + offset) = mgmtData->entries;
    offset += sizeof(int);
    return header;
}


BTreeMtdt *deserializeBtreeHeader(char *header) {
    int offset = 0;
    BTreeMtdt *mgmtData = MAKE_TREE_MTDT();
    mgmtData->n = *(int *) (header + offset);
    offset += sizeof(int);
    mgmtData->keyType = *(int *) (header + offset);
    offset += sizeof(int);
    mgmtData->nodes = *(int *) (header + offset);
    offset += sizeof(int);
    mgmtData->entries = *(int *) (header + offset);
    offset += sizeof(int);
    return mgmtData;
}


RC createBtree(char *idxId, DataType keyType, int n) {
    // Allocate memory for management data
    BTreeMtdt *mgmtData = MAKE_TREE_MTDT();
    if (!mgmtData) return RC_ERROR;

    // Initialize management data
    mgmtData->n = n;
    mgmtData->keyType = keyType;
    mgmtData->nodes = 0;
    mgmtData->entries = 0;
    mgmtData->root = NULL; // Initially, the tree has no nodes
    mgmtData->bm = MAKE_POOL();
    mgmtData->ph = MAKE_PAGE_HANDLE();

    // Initialize buffer pool for the B-tree
    initBufferPool(mgmtData->bm, idxId, 10, RS_LRU, NULL);

    // Serialize and write the tree metadata to disk
    char *header = serializeBtreeHeader(mgmtData);
    RC status = writeStrToPage(idxId, 0, header);
    free(header);

    // Handle potential write errors
    if (status != RC_OK) {
        free(mgmtData->bm);
        free(mgmtData->ph);
        free(mgmtData);
        return status;
    }

    // Successful creation
    return RC_OK;
}

/*
RC openBtree (BTreeHandle **tree, char *idxId) {
    int offset = 0;
    *tree = MAKE_TREE_HANDLE();
    BM_BufferPool *bm = MAKE_POOL();
	BM_PageHandle *ph = MAKE_PAGE_HANDLE();
	BM_PageHandle *phHeader = MAKE_PAGE_HANDLE();
  	initBufferPool(bm, idxId, 10, RS_LRU, NULL);
	pinPage(bm, phHeader, 0);  
    BTreeMtdt *mgmtData = deserializeBtreeHeader(phHeader->data);

    if (mgmtData->nodes == 0) {
        mgmtData->root = NULL;
    }
    mgmtData->minLeaf = (mgmtData->n + 1) / 2;
    mgmtData->minNonLeaf = (mgmtData->n + 2) / 2 - 1;
    mgmtData->ph = ph;
    mgmtData->bm = bm;
    (*tree)->keyType = mgmtData->keyType;
    (*tree)->mgmtData = mgmtData;
    (*tree)->idxId = idxId;
    free(phHeader);
    return RC_OK;
}
*/
// Opens a B-Tree structure from storage identified by idxId and initializes the tree handle.
RC openBtree(BTreeHandle **tree, char *idxId) {
    // Validate input parameters
    if (tree == NULL || idxId == NULL) {
        printf("Error: Null pointer provided to openBtree.\n");
        return RC_ERROR;
    }

    // Allocate and initialize tree handle and buffer management structures
    *tree = MAKE_TREE_HANDLE();
    if (*tree == NULL) {
        printf("Error: Failed to allocate memory for BTreeHandle.\n");
        return RC_ERROR;
    }

    BM_BufferPool *bm = MAKE_POOL();
    if (bm == NULL) {
        printf("Error: Failed to create buffer pool.\n");
        free(*tree);
        return RC_ERROR;
    }

    BM_PageHandle *ph = MAKE_PAGE_HANDLE();
    BM_PageHandle *phHeader = MAKE_PAGE_HANDLE();
    if (ph == NULL || phHeader == NULL) {
        printf("Error: Failed to allocate page handles.\n");
        free(bm);
        free(*tree);
        return RC_ERROR;
    }

    // Initialize the buffer pool with LRU strategy for the given index ID
    initBufferPool(bm, idxId, 10, RS_LRU, NULL);

    // Pin the page containing the B-Tree's metadata
    RC status = pinPage(bm, phHeader, 0);
    if (status != RC_OK) {
        printf("Error: Failed to pin header page.\n");
        free(bm);
        free(ph);
        free(phHeader);
        free(*tree);
        return status;
    }

    // Deserialize B-Tree metadata from the header page
    BTreeMtdt *mgmtData = deserializeBtreeHeader(phHeader->data);
    if (mgmtData == NULL) {
        printf("Error: Failed to deserialize B-Tree metadata.\n");
        unpinPage(bm, phHeader);
        free(bm);
        free(ph);
        free(phHeader);
        free(*tree);
        return RC_ERROR;
    }

    // Configure B-Tree metadata parameters
    mgmtData->root = (mgmtData->nodes == 0) ? NULL : mgmtData->root;
    mgmtData->minLeaf = (mgmtData->n + 1) / 2;
    mgmtData->minNonLeaf = (mgmtData->n + 2) / 2 - 1;
    mgmtData->ph = ph;
    mgmtData->bm = bm;

    // Setup tree handle properties
    (*tree)->keyType = mgmtData->keyType;
    (*tree)->mgmtData = mgmtData;
    (*tree)->idxId = idxId;

    // Clean up and release resources
    free(phHeader);
    return RC_OK;
}


// Close a B-tree index
RC closeBtree(BTreeHandle *tree) {
    BTreeMtdt *mgmtData = (BTreeMtdt*) tree->mgmtData;

    // Flush all pages and shutdown the buffer pool
    shutdownBufferPool(mgmtData->bm);

    // Free all allocated memory
    free(mgmtData->ph);
    free(mgmtData);
    free(tree);

    return RC_OK;
}


// Delete a B-tree index
RC deleteBtree(char *idxId) {
    // Destroy the page file associated with the B-tree
    return destroyPageFile(idxId);
}

// access information about a b-tree//

// Retrieve the number of nodes in the B-tree
RC getNumNodes(BTreeHandle *tree, int *result) {
    if (tree == NULL || result == NULL) {
        return RC_ERROR;
    }

    // Get management data from the tree handle
    BTreeMtdt *mgmtData = (BTreeMtdt*) tree->mgmtData;

    // Set result to the number of nodes
    *result = mgmtData->nodes;

    return RC_OK;
}



// Retrieve the number of entries (keys) in the B-tree
RC getNumEntries(BTreeHandle *tree, int *result) {
    if (tree == NULL || result == NULL) {
        return RC_ERROR;
    }

    // Get management data from the tree handle
    BTreeMtdt *mgmtData = (BTreeMtdt*) tree->mgmtData;

    // Set result to the number of entries
    *result = mgmtData->entries;

    return RC_OK;
}



// Retrieve the key type of the B-tree
RC getKeyType(BTreeHandle *tree, DataType *result) {
    if (tree == NULL || result == NULL) {
        return RC_ERROR;
    }

    // Assign the key type from the tree handle to result
    *result = tree->keyType;

    return RC_OK;
}


int compareValue(Value *key, Value *sign) {
    int result;
    switch (key->dt)
    {
        case DT_INT:
            if (key->v.intV == sign->v.intV) {
                result = 0;
            } else {
                result = (key->v.intV > sign->v.intV) ? 1 : -1;
            }
            break;
        case DT_FLOAT:
            if (key->v.floatV == sign->v.floatV) {
                result = 0;
            } else {
                result = (key->v.floatV > sign->v.floatV) ? 1 : -1;
            }
            break;
        case DT_STRING:
            result = strcmp(key->v.stringV, sign->v.stringV);
            break;
        case DT_BOOL:
            // Todo: not confirm
            result = (key->v.boolV == sign->v.boolV) ? 0 : -1;
        default:
            break;
    }
    return result;
}


BTreeNode* findLeafNode(BTreeNode *node, Value *key) {
    if (node->type == LEAF_NODE) {
        return node;
    }
    for (int i = 0; i < node->keyNums; i++) {
    
        if (compareValue(key, node->keys[i]) < 0) {
            return findLeafNode((BTreeNode *) node->ptrs[i], key);
        }
    }
    return findLeafNode((BTreeNode *) node->ptrs[node->keyNums], key);
}


RID *findEntryInNode(BTreeNode *node, Value *key) {
    for (int i = 0; i < node->keyNums; i++) {
        if (compareValue(key, node->keys[i]) == 0) {
            return (RID *) node->ptrs[i];
        }
    }
    return NULL;
}


RC findKey (BTreeHandle *tree, Value *key, RID *result) {
    if (tree == NULL || key == NULL || result == NULL) {
        printf("Error: One or more input parameters are NULL.\n");
        return RC_IM_KEY_NOT_FOUND;
    }

    BTreeMtdt *mgmtData = (BTreeMtdt *) tree->mgmtData;
    if (mgmtData == NULL || mgmtData->root == NULL) {
        printf("Error: Tree management data or root is NULL.\n");
        return RC_IM_KEY_NOT_FOUND;
    }

    // 1. Identify the precise leaf node L for key k
    BTreeNode* leafNode = findLeafNode(mgmtData->root, key);
    if (leafNode == NULL) {
        printf("Leaf node not found for the provided key.\n");
        return RC_IM_KEY_NOT_FOUND;
    }

    // 2. Utilize binary search to locate the key's position in the node
    RID *r = findEntryInNode(leafNode, key);
    if (r == NULL) {
        printf("Key not found in the leaf node.\n");
        return RC_IM_KEY_NOT_FOUND;
    }

    *result = *r;
    printf("Key found: RID(Page: %d, Slot: %d)\n", result->page, result->slot);
    return RC_OK;
}


BTreeNode *createNode(BTreeMtdt *mgmtData) {
    mgmtData->nodes += 1;
    BTreeNode * node = MAKE_TREE_NODE();
    // insert first and then split
    node->keys = malloc((mgmtData->n+1) * sizeof(Value *));
    node->keyNums = 0;
    node->next = NULL;
    node->parent = NULL;
    return node;
}


BTreeNode *createLeafNode(BTreeMtdt *mgmtData) {
    BTreeNode * node = createNode(mgmtData);
    node->type = LEAF_NODE;
    node->ptrs = (void *)malloc((mgmtData->n+1) * sizeof(void *));
    return node;
}


BTreeNode *createNonLeafNode(BTreeMtdt *mgmtData) {
    BTreeNode * node = createNode(mgmtData);
    node->type = Inner_NODE;
    node->ptrs = (void *)malloc((mgmtData->n+2) * sizeof(void *));
    return node;
}


int getInsertPos(BTreeNode* node, Value *key) {
    int insert_pos = node->keyNums;
    for (int i = 0; i < node->keyNums; i++) {
        if (compareValue(node->keys[i], key) >= 0) {
            insert_pos = i;
            break;
        }
    }
    return insert_pos;
}

RID *buildRID(RID *rid) {
    // Check if the input RID pointer is NULL
    if (rid == NULL) {
        printf("Error: NULL input pointer provided to buildRID.\n");
        return NULL; // Return NULL to indicate failure due to invalid input
    }

    // Allocate memory for a new RID structure
    RID *newRID = (RID *)malloc(sizeof(RID));
    // Check if memory allocation succeeded
    if (newRID == NULL) {
        printf("Error: Memory allocation failed in buildRID.\n");
        return NULL; // Return NULL to indicate failure due to memory allocation error
    }

    // Copy the page and slot information from the input RID to the newly allocated RID
    newRID->page = rid->page;
    newRID->slot = rid->slot;

    // Return the pointer to the newly created RID structure
    return newRID;
}




void insertIntoLeafNode(BTreeNode* node,  Value *key, RID *rid, BTreeMtdt *mgmtData) {
    int insert_pos = getInsertPos(node, key);
    for (int i = node->keyNums; i >= insert_pos; i--) {
        node->keys[i] = node->keys[i-1];
        node->ptrs[i] = node->ptrs[i-1];
    }
    node->keys[insert_pos] = key;
    node->ptrs[insert_pos] = buildRID(rid);
    node->keyNums += 1;
    mgmtData->entries += 1;
}


BTreeNode *splitLeafNode(BTreeNode* node, BTreeMtdt *mgmtData) {
    BTreeNode * new_node = createLeafNode(mgmtData);
    // split right index
    int rpoint = (node->keyNums + 1) / 2;
    for (int i = rpoint; i < node->keyNums; i++) {
        int index = node->keyNums - rpoint - 1;
        new_node->keys[index] = node->keys[i];
        new_node->ptrs[index] = node->ptrs[i];
        node->keys[i] = NULL;
        node->ptrs[i] = NULL;
        new_node->keyNums += 1;
        node->keyNums -= 1;
    }
    new_node->next = node->next;
    node->next = new_node;
    new_node->parent = node->parent;
    return new_node;
}

void splitNonLeafNode(BTreeNode* node, BTreeMtdt *mgmtData) {
    BTreeNode * sibling = createNonLeafNode(mgmtData);
    // split right index
    int mid = node->keyNums / 2;
    // 5, [1.2] 3 [4.5]
    // 4, [1.2] 3 [4]
    Value *pushKey = copyKey(node->keys[mid]);

    int index = 0;
    for (int i = mid + 1; i < node->keyNums; i++) {
        sibling->keys[index] = node->keys[i];
        sibling->ptrs[index + 1] = node->ptrs[i+1];
        node->keys[i] = NULL;
        node->ptrs[i+1] = NULL;
        sibling->keyNums += 1;
        node->keyNums -= 1;
        index += 1;
    }
    sibling->ptrs[0] = node->ptrs[mid + 1];
    node->ptrs[mid + 1] = NULL;
    node->keyNums -= 1;// pushKey
    node->keys[mid] = NULL;

    sibling->parent = node->parent;
    node->next = sibling;
    
    insertIntoParentNode(node, pushKey, mgmtData);
}

/*
void insertIntoParentNode(BTreeNode* lnode, Value *key, BTreeMtdt *mgmtData) {
    BTreeNode* rnode = lnode->next;
    BTreeNode *parent = lnode->parent;
    if (parent == NULL) {
        parent = createNonLeafNode(mgmtData);
        mgmtData->root = lnode->parent = rnode->parent = parent;
        rnode->parent->ptrs[0] = lnode;
    }
    int insert_pos = getInsertPos(parent, key);

    for (int i = parent->keyNums; i > insert_pos; i--) {
        parent->keys[i] = parent->keys[i-1];
        parent->ptrs[i+1] = parent->ptrs[i];
    }
    parent->keys[insert_pos] = key;
    parent->ptrs[insert_pos + 1] = rnode;
    parent->keyNums += 1;

    if (parent->keyNums > mgmtData->n) {
        splitNonLeafNode(parent, mgmtData);
    }
}
*/

void insertIntoParentNode(BTreeNode* lnode, Value *key, BTreeMtdt *mgmtData) {
    // Validate input parameters to ensure they are not null
    if (lnode == NULL || key == NULL || mgmtData == NULL) {
        printf("Error: Null argument provided to insertIntoParentNode.\n");
        return; // Early exit if any input parameter is null
    }

    // Assign right node from the left node's next pointer and fetch parent node
    BTreeNode* rnode = lnode->next;
    BTreeNode* parent = lnode->parent;

    // If no parent exists, create a new one and update tree metadata
    if (parent == NULL) {
        parent = createNonLeafNode(mgmtData);
        if (parent == NULL) {
            printf("Error: Failed to create a new non-leaf node.\n");
            return; // Early exit if failed to create a parent node
        }
        mgmtData->root = lnode->parent = rnode->parent = parent;
        parent->ptrs[0] = lnode; // Establish the first child link to the left node
    }

    // Calculate the correct position for the new key in the parent node
    int insert_pos = getInsertPos(parent, key);
    int current_pos = parent->keyNums; // Initialize position for rearranging elements

    // Shift keys and pointers rightward to free up the insert position
    while (current_pos > insert_pos) {
        parent->keys[current_pos] = parent->keys[current_pos - 1]; // Move keys right
        parent->ptrs[current_pos + 1] = parent->ptrs[current_pos]; // Move pointers right
        current_pos--; // Decrement to continue shifting
    }

    // Place the new key and pointer in the cleared position
    parent->keys[insert_pos] = key;
    parent->ptrs[insert_pos + 1] = rnode;
    parent->keyNums++; // Increment the count of keys in the parent node

    // Split the parent node if it exceeds the maximum allowed keys
 if (parent->keyNums > mgmtData->n) {
        splitNonLeafNode(parent, mgmtData);


        printf("Parent node split successfully.\n"); // Confirm success if split is successful
    }

}

Value *copyKey(Value *key) {
    // Check if the input key pointer is NULL
    if (key == NULL) {
        printf("Error: NULL input pointer provided to copyKey.\n");
        return NULL; // Return NULL to indicate failure due to invalid input
    }

    // Allocate memory for a new Value structure
    Value *new_key = (Value *) malloc(sizeof(Value));
    // Check if memory allocation succeeded
    if (new_key == NULL) {
        printf("Error: Memory allocation failed in copyKey.\n");
        return NULL; // Return NULL to indicate failure due to memory allocation error
    }

    // Copy the content of the key to the newly allocated Value
    memcpy(new_key, key, sizeof(*new_key));

    // Return the pointer to the newly created Value structure
    return new_key;
}


RC insertKey (BTreeHandle *tree, Value *key, RID rid) {
    if (!tree || !key) {
        printf("Error: Tree or key pointer is null.\n");
        return RC_ERROR;
    }

    BTreeMtdt *mgmtData = (BTreeMtdt *) tree->mgmtData;
    if (!mgmtData) {
        printf("Error: B-tree management data is null.\n");
        return RC_ERROR;
    }

    // Initialize the root node if it is not present
    if (!mgmtData->root) {
        mgmtData->root = createLeafNode(mgmtData);
        printf("Created new root leaf node as none was present.\n");
    }

    // 1. Locate the leaf node that should hold the new key
    BTreeNode* leafNode = findLeafNode(mgmtData->root, key);
    if (!leafNode) {
        printf("Error: Failed to find or create leaf node for the key.\n");
        return RC_IM_KEY_NOT_FOUND;
    }

    // Verify if the key already exists in the B-tree
    if (findEntryInNode(leafNode, key)) {
        printf("Error: Duplicate key insertion attempted.\n");
        return RC_IM_KEY_ALREADY_EXISTS;
    }

    // 2. Insert the key in the found leaf node in a sorted manner
    insertIntoLeafNode(leafNode, key, &rid, mgmtData);
    printf("Key successfully inserted into leaf node.\n");

    // Check space availability in the leaf node after insertion
    if (leafNode->keyNums <= mgmtData->n) {
        printf("Insertion complete; no further action required.\n");
        return RC_OK;
    }

    // Node splitting required if space is insufficient
    BTreeNode *rnode = splitLeafNode(leafNode, mgmtData);
    printf("Leaf node split due to overflow; redistributing entries.\n");

    // Redistribute entries and manage new indices
    Value *new_key = copyKey(rnode->keys[0]);
    insertIntoParentNode(leafNode, new_key, mgmtData);
    printf("New key promoted to parent node.\n");

    return RC_OK;
}



void deleteFromLeafNode(BTreeNode *node, Value *key, BTreeMtdt *mgmtData) {
    int insert_pos = getInsertPos(node, key);
    for (int i = insert_pos; i < node->keyNums; i++) {
        node->keys[i] = node->keys[i+1];
        node->ptrs[i] = node->ptrs[i+1];
    }
    node->keyNums -= 1;
    mgmtData->entries -= 1;
}


bool isEnoughSpace(int keyNums, BTreeNode *node, BTreeMtdt *mgmtData) {
    int min = node->type == LEAF_NODE ? mgmtData->minLeaf : mgmtData->minNonLeaf;
    if (keyNums >= mgmtData->minLeaf && keyNums <= mgmtData->n) {
        return true;
    }
    return false;
}


void deleteParentEntry(BTreeNode *node) {
    BTreeNode *parent = node->parent;
    for (int i = 0; i <= parent->keyNums; i++) {
        if (parent->ptrs[i] == node) {
            int keyIndex = i-1 < 0 ? 0 : i-1;
            parent->keys[keyIndex] = NULL;
            parent->ptrs[i] = NULL;
            for (int j = keyIndex; j < parent->keyNums - 1; j++) {
                parent->keys[j] = parent->keys[j+1];
            }
            for (int j = i; j < parent->keyNums; j++) {
                parent->ptrs[j] = parent->ptrs[j+1];
            }
            parent->keyNums -= 1;
            break;
        }
    }
}


void updateParentEntry(BTreeNode *node, Value *key, Value *newkey) {
    // equal = no update
    if (compareValue(key, newkey) == 0) {
        return;
    }
    
    bool isDelete = false;
    BTreeNode *parent = node->parent;
    // 1. delete key
    for (int i = 0; i < parent->keyNums; i++) {
        if (compareValue(key, parent->keys[i]) == 0) {
            for (int j = i; j < parent->keyNums - 1; j++) {
                parent->keys[j] = parent->keys[j+1];
            }
            parent->keys[parent->keyNums - 1] = NULL;
            parent->keyNums -= 1;
            isDelete = true;
            break;
        }
    }
    if (isDelete == false) {
        return;
    }
    // 2. add new key
    int insert_pos = getInsertPos(parent, newkey);
    for (int i = parent->keyNums; i >= insert_pos; i--) {
        parent->keys[i] = parent->keys[i-1];
    }
    parent->keys[insert_pos] = newkey;
    parent->keyNums += 1;
}




bool redistributeFromSibling(BTreeNode *node, Value *key, BTreeMtdt *mgmtData) {
    BTreeNode *parent = node->parent;
    BTreeNode *sibling = NULL;
    int index;
    
    for (int i = 0; i <= parent->keyNums; i++) {
        if (parent->ptrs[i] != node) {
            continue;
        }
        // perfer left sibling
        sibling = (BTreeNode *) parent->ptrs[i-1];
        if (sibling && isEnoughSpace(sibling->keyNums - 1, sibling, mgmtData) == true) {
            index = sibling->keyNums - 1;
            break;
        }
        sibling = (BTreeNode *) parent->ptrs[i+1];
        if (sibling && isEnoughSpace(sibling->keyNums - 1, sibling, mgmtData) == true) {
            index = 0;
            break;
        }
        sibling = NULL;
        break;
    }
    if (sibling) {
        insertIntoLeafNode(node, copyKey(sibling->keys[index]), sibling->ptrs[index], mgmtData);
        updateParentEntry(node, key, copyKey(sibling->keys[index]));
        sibling->keyNums -= 1;
        free(sibling->keys[index]);
        free(sibling->ptrs[index + 1]);
        sibling->keys[index] = NULL;
        sibling->ptrs[index + 1] = NULL;
        return true;
    }
    // redistribute fail
    return false;
}


BTreeNode *checkSiblingCapacity(BTreeNode *node, BTreeMtdt *mgmtData) {
    BTreeNode *sibling = NULL;
    BTreeNode * parent = node->parent;
    for (int i = 0; i <= parent->keyNums; i++) {
        if (parent->ptrs[i] != node) {
            continue;
        }
        if (i - 1 >= 0) {
            // perfer left sibling
            sibling = (BTreeNode *) parent->ptrs[i-1];
            if (isEnoughSpace(sibling->keyNums + node->keyNums, sibling, mgmtData) == true) {
                break;
            }
        }
        if (i + 1 <= parent->keyNums + 1) {
            sibling = (BTreeNode *) parent->ptrs[i+1];
            if (isEnoughSpace(sibling->keyNums + node->keyNums, sibling, mgmtData) == true) {
                break;
            }
        }
        sibling = NULL;
        break;
    }
    return sibling;
}


void mergeSibling(BTreeNode *node, BTreeNode *sibling, BTreeMtdt *mgmtData) {
    BTreeNode *parent = node->parent;
    
    int key_count = sibling->keyNums + node->keyNums; 
    if (key_count > sibling->keyNums) {
        int i = 0, j = 0, curr = 0;
        Value **newkeys = malloc((key_count) * sizeof(Value *));
        void **newptrs = malloc((key_count + 1) * sizeof(void *));
        for (curr = 0; curr < key_count; curr++) {
            if (j >= node->keyNums || compareValue(sibling->keys[i], node->keys[j]) <= 0) {
                newkeys[curr] = sibling->keys[i];
                newptrs[curr] = sibling->ptrs[i];
                i += 1;
            } else {
                newkeys[curr] = sibling->keys[j];
                newptrs[curr] = sibling->ptrs[j];
            }
        }
        free(sibling->keys);
        free(sibling->ptrs);
        sibling->keys = newkeys;
        sibling->ptrs = newptrs;
    }
    // update parent
    deleteParentEntry(node);
    mgmtData->nodes -= 1;
    free(node->ptrs);
    free(node->keys);
    free(node);
    // Todo: recurisve
    if (sibling->keyNums < mgmtData->minNonLeaf) {}
}


RC deleteKey (BTreeHandle *tree, Value *key) {
    if (!tree || !key) {
        printf("Error: Tree or key pointer is null.\n");
        return RC_ERROR;
    }

    BTreeMtdt *mgmtData = (BTreeMtdt *) tree->mgmtData;
    if (!mgmtData || !mgmtData->root) {
        printf("Error: B-tree management data or root is null.\n");
        return RC_ERROR;
    }

    // 1. Locate the leaf node that should contain the key
    BTreeNode* leafNode = findLeafNode(mgmtData->root, key);
    if (!leafNode) {
        printf("Error: No appropriate leaf node found for the key.\n");
        return RC_IM_KEY_NOT_FOUND;
    }

    // Check if the key exists in the tree
    if (!findEntryInNode(leafNode, key)) {
        printf("Error: Key not found in the B-tree.\n");
        return RC_IM_KEY_NOT_FOUND;
    }

    // 2. Remove the entry from the node
    deleteFromLeafNode(leafNode, key, mgmtData);
    printf("Key has been removed from the leaf node.\n");

    // Check if the leaf node is sufficiently filled post-deletion
    if (leafNode->keyNums >= mgmtData->minLeaf) {
        Value *newKey = copyKey(leafNode->keys[0]);
        updateParentEntry(leafNode, key, newKey);
        printf("Parent node updated after deletion.\n");
        return RC_OK;
    }

    // If leaf node is under-filled, attempt to balance the tree
    BTreeNode *sibling = checkSiblingCapacity(leafNode, mgmtData);
    if (sibling) {
        mergeSibling(leafNode, sibling, mgmtData);
        printf("Merged with sibling due to insufficient capacity.\n");
    } else {
        redistributeFromSibling(leafNode, key, mgmtData);
        printf("Redistributed entries from sibling.\n");
    }

    return RC_OK;
}


RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
    // Check for null input pointers to ensure they are valid
    if (!tree || !handle) {
        printf("Error: Null tree or handle pointer provided.\n");
        return RC_ERROR;
    }

    BTreeMtdt *mgmtData = (BTreeMtdt *) tree->mgmtData;
    // Ensure the management data and root are not null
    if (!mgmtData || !mgmtData->root) {
        printf("Error: Management data or root node is null.\n");
        return RC_IM_KEY_NOT_FOUND;
    }

    BTreeNode *node = (BTreeNode *) mgmtData->root;
    // Traverse down to the first leaf node
    while (node && node->type == Inner_NODE) {
        node = node->ptrs[0];
    }
    if (!node) {
        printf("Error: Failed to find any leaf nodes in the B-tree.\n");
        return RC_IM_KEY_NOT_FOUND;
    }

    // Allocate memory for scan metadata
    BT_ScanMtdt *scanMtdt = (BT_ScanMtdt *) malloc(sizeof(BT_ScanMtdt));
    if (!scanMtdt) {
        printf("Error: Memory allocation for scan metadata failed.\n");
        return RC_ERROR;
    }
    // Allocate memory for handle
    (*handle) = malloc(sizeof(BT_ScanHandle));
    if (!(*handle)) {
        printf("Error: Memory allocation for scan handle failed.\n");
        free(scanMtdt);  // Clean up previously allocated memory
        return RC_ERROR;
    }

    // Initialize scan metadata
    scanMtdt->keyIndex = 0;
    scanMtdt->node = node;
    (*handle)->mgmtData = scanMtdt;
    printf("B-tree scan initialized successfully.\n");

    return RC_OK;
}


RC nextEntry(BT_ScanHandle *handle, RID *result) {
    if (!handle || !result) {
        printf("Error: Null pointer provided for scan handle or result.\n");
        return RC_ERROR;
    }

    BT_ScanMtdt *scanMtdt = (BT_ScanMtdt *) handle->mgmtData;
    if (!scanMtdt) {
        printf("Error: Scan management data is null.\n");
        return RC_ERROR;
    }

    BTreeNode* node = scanMtdt->node;
    int keyIndex = scanMtdt->keyIndex;

    // Check if current node has more keys to return
    if (keyIndex < node->keyNums) {
        // Assuming node->ptrs is an array where each position corresponds to an RID pointer for the key at the same index
        RID *rid = (RID *) node->ptrs[keyIndex];  // Correct pointer retrieval method based on the typical structure
        *result = *rid;  // Dereference and copy RID to result
        scanMtdt->keyIndex += 1;  // Increment key index for the next call
        printf("Retrieved key at index %d from current node.\n", keyIndex);
        return RC_OK;
    } else {
        // If no more keys in the current node, move to the next node
        if (!node->next) {
            printf("End of entries, no more nodes to scan.\n");
            return RC_IM_NO_MORE_ENTRIES;
        }
        scanMtdt->node = node->next;  // Move to the next node
        scanMtdt->keyIndex = 0;  // Reset key index
        printf("Moved to the next node in the scan.\n");
        return nextEntry(handle, result);  // Recursive call to continue from the next node
    }
}




RC closeTreeScan(BT_ScanHandle *handle) {
    // Check if the handle is not null
    if (!handle) {
        printf("Error: Null scan handle provided. Cannot close scan.\n");
        return RC_ERROR;
    }

    // Safely free the management data if it exists
    if (handle->mgmtData) {
        free(handle->mgmtData);
        handle->mgmtData = NULL;  // Set to NULL to prevent dangling pointer
    } else {
        printf("Warning: Scan handle management data already null.\n");
    }

    // Free the handle itself
    free(handle);
    printf("Tree scan successfully closed.\n");

    return RC_OK;
}


/*
char *printTree (BTreeHandle *tree) {
    BTreeMtdt *mgmtData = (BTreeMtdt *) tree->mgmtData;
    BTreeNode *root = mgmtData->root;
    if (root == NULL) {
        return NULL;
    }
    BTreeNode **queue = malloc((mgmtData->nodes) * sizeof(BTreeNode *));
    int level = 0;
    int count = 1;
    int curr = 0;

    queue[0] = root;
    while(curr < mgmtData->nodes) {
        BTreeNode * node = queue[curr];
        printf("(%i)[", level);

        for (int i = 0; i < node->keyNums; i++) {
            if (node->type == LEAF_NODE) {
                RID *rid = (RID *)node->ptrs[i];
                printf("%i.%i,", rid->page, rid->slot);
            } else {
                printf("%i,", count);
                queue[count] = (BTreeNode *)node->ptrs[i];
                count += 1;
            }
            if (node->type == LEAF_NODE && i == node->keyNums - 1) {
                printf("%s", serializeValue(node->keys[i]));
            } else {
                printf("%s,", serializeValue(node->keys[i]));
            }
        }
        if (node->type == Inner_NODE) {
            printf("%i", count);
            queue[count] = (BTreeNode *)node->ptrs[node->keyNums];
            count += 1;
        }
        level += 1;
        curr += 1;
        printf("]\n");
    }
    return RC_OK;
}
*/

char *printTree(BTreeHandle *tree) {
    if (tree == NULL || tree->mgmtData == NULL) {
        printf("Error: Tree or tree management data is null.\n");
        return NULL;
    }

    BTreeMtdt *mgmtData = (BTreeMtdt *)tree->mgmtData;
    BTreeNode *root = mgmtData->root;
    if (root == NULL) {
        printf("The tree is empty.\n");
        return NULL;
    }

    // Allocate memory for the queue to hold pointers to the tree nodes for level-order traversal
    BTreeNode **queue = malloc(mgmtData->nodes * sizeof(BTreeNode *));
    if (queue == NULL) {
        printf("Memory allocation failed for the queue.\n");
        return NULL;
    }

    int front = 0;
    int rear = 0;
    // Initialize queue
    queue[rear++] = root;

    // For accumulating the result string
    char *result = malloc(10000 * sizeof(char)); // Assuming a large enough buffer
    if (result == NULL) {
        printf("Memory allocation failed for result string.\n");
        free(queue);
        return NULL;
    }
    result[0] = '\0'; // Initialize empty string

    // Process each level
    while (front < rear) {
        BTreeNode *node = queue[front++];
        char buffer[1024]; // Temporary buffer to hold node's keys and children info
        int length = sprintf(buffer, "Level %d: [", front - 1);

        // Process each key in the node
        for (int i = 0; i < node->keyNums; ++i) {
            char *keyString = serializeValue(node->keys[i]);
            if (node->type == LEAF_NODE) {
                RID *rid = (RID *)node->ptrs[i];
                length += sprintf(buffer + length, "%s(Page: %d, Slot: %d), ", keyString, rid->page, rid->slot);
            } else {
                length += sprintf(buffer + length, "%s, ", keyString);
                if (i < node->keyNums) {
                    queue[rear++] = (BTreeNode *)node->ptrs[i];
                }
            }
        }

        // Add last pointer for non-leaf nodes
        if (node->type == Inner_NODE) {
            queue[rear++] = (BTreeNode *)node->ptrs[node->keyNums];
        }

        // Finish off this level's entry
        buffer[length - 2] = ']';  // Overwrite last comma
        buffer[length - 1] = '\0';
        strcat(result, buffer);
        strcat(result, "\n");
    }

    free(queue);
    return result;
}
