#ifndef BTREE_MGR_H
#define BTREE_MGR_H

#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "dberror.h"
#include "tables.h"

typedef enum NodeType {
	Inner_NODE = 1,
	LEAF_NODE = 0
} NodeType;

typedef struct BTreeNode {
  NodeType type;
  Value **keys; // array[0...n]
  // leaf-node => RID
  // non-leaf-node => next level node
  void **ptrs; // array[0...n]
  int keyNums; // the count of key
  RID *records;
  struct BTreeNode *next; // sibling
  struct BTreeNode *parent; // parent
} BTreeNode;

// structure for accessing btrees
typedef struct BTreeHandle {
  DataType keyType;
  char *idxId;
  void *mgmtData;
} BTreeHandle;


typedef struct BTreeMtdt {
  int n; // maximum keys in each block
  int minLeaf;
  int minNonLeaf;
  int nodes; // the count of node
  int entries; // the count of entries
  DataType keyType;

  BTreeNode *root;
  
  BM_PageHandle *ph;
  BM_BufferPool *bm;
} BTreeMtdt;


typedef struct BT_ScanHandle {
  BTreeHandle *tree;
  void *mgmtData;
} BT_ScanHandle;

typedef struct BT_ScanMtdt {
  int keyIndex;
  BTreeNode *node;
} BT_ScanMtdt;



#define MAKE_TREE_HANDLE()				\
		((BTreeHandle *) malloc (sizeof(BTreeHandle)))

#define MAKE_TREE_MTDT()				\
		((BTreeMtdt *) malloc (sizeof(BTreeMtdt)))

#define MAKE_TREE_NODE()				\
		((BTreeNode *) malloc (sizeof(BTreeNode)))

#define MAKE_TREE_SCAN()				\
		((BT_ScanHandle *) malloc (sizeof(BT_ScanHandle)))

// init and shutdown index manager
extern RC initIndexManager (void *mgmtData);
extern RC shutdownIndexManager ();

// create, destroy, open, and close an btree index
extern RC createBtree (char *idxId, DataType keyType, int n); //changed
extern RC openBtree (BTreeHandle **tree, char *idxId);
extern RC closeBtree (BTreeHandle *tree); //changed
extern RC deleteBtree (char *idxId); //changed

// access information about a b-tree
extern RC getNumNodes (BTreeHandle *tree, int *result); //changed
extern RC getNumEntries (BTreeHandle *tree, int *result); //changed
extern RC getKeyType (BTreeHandle *tree, DataType *result); //changed

// index access
extern RC findKey (BTreeHandle *tree, Value *key, RID *result); //changed
extern RC insertKey (BTreeHandle *tree, Value *key, RID rid); //changed
extern RC deleteKey (BTreeHandle *tree, Value *key);//changed
extern RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle);//changed
extern RC nextEntry (BT_ScanHandle *handle, RID *result); //changed
extern RC closeTreeScan (BT_ScanHandle *handle); //changed

// b+tree
extern void insertIntoParentNode(BTreeNode*, Value *, BTreeMtdt *); //changed
extern RID* buildRID(RID *); //changed
extern Value *copyKey(Value *); 

// debug and test functions
extern char *printTree (BTreeHandle *tree);
RC writeStrToPage(char *idxId, int pageNum, char *str);

#endif // BTREE_MGR_H
