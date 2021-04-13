//
// 180306 - modified split to use size instead of count
//			problem with combo of huge and tiny key:data 
// added errno check after I/O
// 180612 - DeleteNode & NewNode reuse free nodes
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <errno.h>
#include <assert.h>
#include "RField.h"
#include "RKey.hpp"
#include "Ndbdefs.h"
#include "RPage.hpp"
#include "RData.hpp"
#include "RNode.hpp"

extern	errno_t	err;

/* Symbolic index node format
	+----+----+----+----+----+-------+----+----+
	| P0 | LS | RS | K1 | P1 |  ...  | Kn | Pn |
 	+----+----+----+----+----+-------+----+----+   */

// Ki:Pi are composites - Ki consists of a Key part Pi is a node #
// LS - Left Sibling node #
// RS - Right Sibling node #
// so that n keys will have n+1 pointers to other nodes
// node # is a positive integer corresponding to a nodesize space on disk
// there is no node zero, the root node is always node # 1

// di data items exists only on leaf level nodes
// Note that nodes treat keys as ordinals (i.e. 1 to n), pages are zero based
// leaf nodes have no Pi items
/*
	+----+----+----+-------+-------+-------+
	| 0  | LS | RS | k1:d1 |  ...  | kn:dn |
 	+----+----+----+-------+-------+-------+   */

// since RNode owns the nodeMemory it is allowed to read from it directly,
// even though it is managed exclusively by RPage
// almost a class friend situation
//==================================================================
RNode::RNode(const int fileHandle, const NODE fileAvail) {
	nodeFd = fileHandle;
	nodeCurr = 0;
	nodeAvail = fileAvail;
	Clear();
}
//==================================================================
RNode::RNode(const int fileHandle) {
	nodeFd = fileHandle;
	nodeCurr = 0;
	Clear();
}
//==================================================================
// See if node has enough empty space to hold a specified key
int RNode::BigEnough(const int len) {
	return (nodePage.BigEnough(len));
	}	
//==================================================================
// Clear a node back to initial conditions
void RNode::Clear() {
	nodeLeftSibling = 0;
	nodeRightSibling = 0;
	nodeP0 = 0;
	memset(&nodeMemory[0], 0, KEYSPACE);
	nodePage.Initialize(&nodeMemory[0], KEYSPACE);
	}
//==================================================================
// delete a key
int RNode::DeleteKey(const int keyNo) {
	nodePage.Delete(keyNo-1);				// just delete it
	return 0;
	}
//==================================================================
// delete an index key

/*	      +----+----+----+----+------+----+----+
	  P0  | K1 | P1 | K2 | P2 | ...  | Kn | Pn |
          +----+----+----+----+------+----+----+
*/

int RNode::DeleteIndex(const int keyNo) {
	NODE			nnNode;
	int			len;

/*	case 1: delete lowest node: K1
	
	      +----+----+------+----+----+
	  P1  | K2 | P2 | ...  | Kn | Pn |
 	      +----+----+------+----+----+
*/
	if (keyNo == 0) {							// if P0 is deleted, 
		nodeP0 = GetPi(1);						// grab P1 for new P0
		DeleteKey(1);							// delete K1,P1
		}
/*
	case 2: delete intermediate node ie: K2
	
	      +----+----+------+----+----+
	  P0  | K1 | P2 | ...  | Kn | Pn |
          +----+----+------+----+----+	
*/

	else if (keyNo < nodePage.GetSlots()) {
		nnNode = GetPi(keyNo+1);				// extract ptr
		DeleteKey(keyNo+1);						// remove key
		len = nodePage.GetDataLen(keyNo-1) - LNODE;
		nodePage.Insert((char *)&nnNode, LNODE, keyNo-1, len);
		}
/*	 case 3: delete RHE node

	      +----+----+----+----+------+------+------+
	  P0  | K1 | P1 | K2 | P2 | ...  | Kn-1 | Pn-1 |
 	      +----+----+----+----+------+------+------+
*/
	else										// last key
		DeleteKey(keyNo);

	return 0;
	}
//==================================================================
// delete a node from the file (and recycle it's disk space ???)

int RNode::DeleteNode() {
	RNode	work(nodeFd);

	// revise left/right node links
	if (nodeLeftSibling) {
		work.Read(nodeLeftSibling);
		work.nodeRightSibling = nodeRightSibling;
		work.Write();
		}
	if (nodeRightSibling) {
		work.Read(nodeRightSibling);
		work.nodeLeftSibling = nodeLeftSibling;
		work.Write();
		}
	
	nodeP0 = nodeAvail;					// possibly 0
	nodeLeftSibling = nodeAvail;
	nodeRightSibling = nodeAvail;

	_set_errno(0);
	_lseek(nodeFd, nodeCurr*NODESIZE, SEEK_SET);	// put avail pointer in deleted node
	_get_errno(&err);
	assert(errno == 0);
	_write(nodeFd, &nodeP0, 3 * sizeof(NODE));
	_get_errno(&err);
	assert(errno == 0);
	nodeAvail = nodeCurr;
	_lseek(nodeFd, 0L, SEEK_SET);					// make just deleted node first in avail list
	_get_errno(&err);
	assert(errno == 0);
	_write(nodeFd, &nodeCurr, sizeof(NODE));
	_get_errno(&err);
	assert(errno == 0);

	return 0;
	}
//==================================================================
// returns a pointer to the data part of a key:data item

const char* RNode::GetData(const int keyno) {
	const char*	item;
	RKey	key;

	item = GetKi(keyno);
	GetKey(&key, keyno);
	item = item + key.GetKeyLen();
	return (item);
	}
//==================================================================
// access a key from the node and make a Key object from it

void RNode::GetKey(RKey *key, const int keyno) {
	const char	*item;

	if (keyno > GetCount())				//0522
		item = GetKi(GetCount());
	else
		item = GetKi(keyno);
	key->SetKey(item);
	}
//==================================================================
// access RHE key from the node and make a Key object from it

void RNode::GetLastKey(RKey *key) {
	const char	*item;
	int		kn;

	kn = nodePage.GetSlots();
	item = GetKi(kn);
	key->SetKey(item);
	}
//==================================================================
// get the pointer part of an index key

NODE RNode::GetPi(const int i) {
	RKey	tkey;
	const char	*item;
	int		klen;
	NODE	Pi;

	if (i == 0)
		return nodeP0;
	item = GetItem(i);						// LHE address
	tkey = item;							// make a key object
	klen = tkey.GetKeyLen();				// key length
	memcpy(&Pi, item+klen, LNODE);
	return (Pi);
	}
//==================================================================
// get the RHE index pointer
NODE RNode::GetPn() {
	int	kn;

	kn = nodePage.GetSlots();				// last item in page
	return (GetPi(kn));
	}
//==================================================================
// insert a key into the pile (if there is room in the inn)
int RNode::InsertKey(const int keyNo, RKey &key) {
	int klen = key.GetKeyLen();
	if (nodePage.BigEnough(klen)) {
		nodePage.Allocate(keyNo-1, klen);
		nodePage.Insert(key.GetKeyStr(), klen, keyNo-1);
		return (1);
		}
	return (0);		
	}	
//==================================================================
// insert a key & data into the pile (if there is room in the inn)
int RNode::InsertKeyData(const int keyNo, RKey &key, RData &data) {

	int klen = key.GetKeyLen();
	int dlen =  data.GetDataLen();

	if (nodePage.BigEnough(klen + dlen)) {
		nodePage.Allocate(keyNo-1, klen+dlen);
		nodePage.Insert(key.GetKeyStr(), klen, keyNo-1);
		if (dlen > 0)
			nodePage.Insert(data.GetDataStr(), dlen, keyNo-1, klen);
		return (1);
		}
	return (0);		
	}	
//==================================================================
// insert a key into the pile (if there is room in the inn)
int RNode::InsertKeyPtr(const int keyNo, RKey &key, NODE node) {
	int klen = key.GetKeyLen();
	int len = klen + LNODE;
	if (nodePage.BigEnough(len)) {
		nodePage.Allocate(keyNo-1, len);
		nodePage.Insert(key.GetKeyStr(), klen, keyNo-1);
		nodePage.Insert((char *)&node, LNODE, keyNo-1, klen);
		return (1);
		}
	return (0);		
	}	
//==================================================================
// create a new node on disk
NODE RNode::NewNode() {
	int		rawpos;
	NODE	avail;

	_set_errno(0);
	if (nodeAvail) {								// if allocated free space is available
		_lseek(nodeFd, nodeAvail*NODESIZE, SEEK_SET);
		_get_errno(&err);
		assert(errno == 0);
		_read(nodeFd, &avail, sizeof(NODE));		// read it's avail link
		_get_errno(&err);
		assert(errno == 0);
		_lseek(nodeFd, 0L, SEEK_SET);				// rewrite node 0 and 1st avail node #
		_get_errno(&err);
		assert(errno == 0);
		_write(nodeFd, &avail, sizeof(NODE));
		_get_errno(&err);
		assert(errno == 0);
		nodeCurr = nodeAvail;
		nodeAvail = avail;
	}
	else {
		rawpos = _lseek(nodeFd, 0L, SEEK_END);		// EOF
		_get_errno(&err);
		assert(errno == 0);
		_write(nodeFd, &nodeP0, NODESIZE);
		_get_errno(&err);
		assert(errno == 0);
		nodeCurr = rawpos / NODESIZE;				// compute node number
		}
//	printf("node %d created\n", nodeCurr);
	return(nodeCurr);
	}
//==================================================================
// Advance a key within the node
int RNode::NextKey(int keyno) {
	if (keyno < GetCount())
		return keyno + 1;
	return 0;
	}
//==================================================================
// Advance to right sibling
NODE RNode::NextNode() {
	NODE nnNext;

	if (nnNext = nodeRightSibling) {
		Read(nodeRightSibling);
		return nnNext;
		}
	return 0;
	}
//==================================================================
// Backup a key - not very useful
int RNode::PrevKey(int keyno) {
	return --keyno;
	}
//==================================================================
// Advance to left sibling
NODE RNode::PrevNode() {
	NODE nnPrev;

	if (nnPrev = nodeLeftSibling) {
		Read(nodeLeftSibling);
		return nnPrev;
		}
	return 0;
	}
//==================================================================
// no RNode function should know about leafs, indexes, or roots,
// just if Ptrs are used, and maybe not even that
void RNode::PrintNode() {
	RKey	key;
	RData	*data;
	int		recno;
	int		len;
	
	printf("Dump of node # %d", nodeCurr);
	if (nodeLeftSibling == 0 && nodeRightSibling == 0)
		printf(" (Root) ");
	else {
		if (nodeP0 == 0)
			printf(" (Leaf) ");
		else
			printf(" (Index) ");
		printf(" L/R Sibs: %d / %d", nodeLeftSibling, nodeRightSibling);
		}

	if (nodeP0 != 0)
		printf(" P0: %d", nodeP0);
	printf("\n");;
	for (int i = 1; i <= nodePage.GetSlots(); i++) {
//		cout << " Key# " << i << "  ";	// expanded form
		printf(" %d ", i );			// compact form
		GetKey(&key, i);
		key.PrintKey();
		if (nodeP0) {
			recno = GetPi(i);
			printf(" @%d", recno);
			}
		else {
			len = key.GetKeyLen();
			data = new RData(GetKi(i) + len);
			data->PrintData();
			delete data;
			}
		printf("\n");
		}
//	printf("\n");
	}
//==================================================================
// scan a node for leftmost matching key using a binary search

// returns:
//		0 if key < K1
//		i if key == Ki
//		-i if Ki < key < Ki+1 or Kn < key

// note: for a right to left seek, set lo to i at if res == 0

int RNode::ScanNode(RKey &key) {
	int		lo, hi;
	int		i;
	int		res;
	
	hi = GetCount();
	if (hi <= 0)
		return (0);								// empty node
	lo = 1;	
	while (lo <= hi) {
		i = (lo + hi) / 2;
		res = key.KeyCompare(GetKi(i));			// res = -1 < 0 < 1 less:equal:greater
		if (lo == hi) {							// end of scan
			if (res < 0)						// but key < Ki[hi]
				hi--;
			break;
			}
		if (res > 0)
			lo = i + 1;
		else if (res == 0)						// a hit becomes hi
			hi = i;								// there may be lower exact hits
		else
			hi = i - 1;							// hi eventually = 0 if key < K1
		}
	if (res == 0)
		return (hi);							// direct hit
	return (-hi);								// between Ki & Ki+1
//	res = (res == 0) ? hi : -hi;				// hit or between?
	return (res);
 	} 
//==================================================================
// scan index node for an entry that points down to a descendant
int RNode::ScanNodeforP(NODE desc) {
	if (desc == GetPi(0))
		return 0;

	for (int i = 0; i <= nodePage.GetSlots(); i++) {
		if (desc == GetPi(i))
			return i;
		}
	return -1;									// no find
	}
//==================================================================
// move right half of the keys of this node into node2

// I wonder if this function belongs in the node object. Maybe its function
// should be entirely in the tree object

int RNode::Split(RNode *node2) {
	RNode	work(nodeFd);
	NODE	nnRight;
	int		lKeys, rKeys;
	const char	*item;
	int		len, nlen;
	int		ttllen;
	int		half;
	
	// Split node into two nodes
	// split by memory size
	nlen = nodePage.GetSlots();
	ttllen = 0;
	half = nodePage.GetUsed() / 2;				// middle aiming point
	for (lKeys = 0; lKeys < nlen; lKeys++) {
		ttllen += nodePage.GetDataLen(lKeys);		// add up cum size so far
		if (ttllen >= half)
			break;
		}
	rKeys = nlen - lKeys;

//	rKeys = nlen / 2;		// split by slots
//	lKeys = nlen - rKeys;
	
	// move right half of node into new node
	for (int i = 0; i < rKeys; i++) {
		item = nodePage.GetDataItem(lKeys+i);
		len = nodePage.GetDataLen(lKeys+i);
		node2->nodePage.Allocate(i, len);
		node2->nodePage.Insert(item, len, i);
		} 
	nodePage.Delete(lKeys, nlen-1);

	nnRight = GetRightSibling();				//	reconnect sibling pointers
	node2->SetRightSibling(nnRight);
	node2->SetLeftSibling(nodeCurr);
	SetRightSibling(node2->nodeCurr);
	if (nnRight) {								// if right exists, point to new node
		work.Read(nnRight);
		work.SetLeftSibling(node2->nodeCurr);
		work.Write();
		}

//	cout << "new left node\n";
//	nodePage.Dump();
//	cout << "new right node\n";
//	node2->nodePage.Dump();
	return (lKeys);
	}
//==================================================================
// Read a node from disk into a node object
int RNode::Read(NODE node) {
	int bytes;   

	if (nodeCurr == node)
		return 1;
	_set_errno(0);
	_lseek(nodeFd, node*NODESIZE, SEEK_SET);
	_get_errno(&err);
	assert(errno == 0);
	bytes = _read(nodeFd, &nodeP0, NODESIZE);
	_get_errno(&err);
	assert(errno == 0);
	if (bytes < NODESIZE)
		return 0;
	nodePage.Reset(&nodeMemory[0], KEYSPACE);
	nodeCurr = node; 
	return 1;
	}
//==================================================================
// Write a node object onto disk
int RNode::Write() {
	_set_errno(0);
	_lseek(nodeFd, nodeCurr*NODESIZE, SEEK_SET);
	_get_errno(&err);
	assert(errno == 0);
	_write(nodeFd, &nodeP0, NODESIZE);
	_get_errno(&err);
	assert(errno == 0);
	return 0;
	}
