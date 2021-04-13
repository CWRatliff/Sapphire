#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "RField.h"
#include "RKey.hpp"
#include "Ndbdefs.h"
#include "RPage.hpp"
#include "RData.hpp"
#include "RNode.hpp"
#include "RBtree.hpp"

#define ROOT	1L

// all Btree usage is done using NDX_ID typed structures. There is
// only one RBtree per RDbf but there can be a large number
// of indexes using that RBtree object. The ndx_id structure allows
// that concurrent access

//typedef	struct	ndxID_str {
//	int		ndxKeyNo;				// key in page
//	NODE	ndxNode;				// node number
//	int		ndxStatus;				// status bits
//	RKey	ndxKey;					// NB. ndxno present
//	} NDX_ID;


// root node is never moved, root splits maintain root location
//==================================================================
RBtree::RBtree(const int fileHandle, NODE fileAvail) {
	btFd = fileHandle;
	btRoot = new RNode(btFd, fileAvail);
	btRoot->Read(ROOT);
	btRoot->SetCurrNode(ROOT);
	btWork = new RNode(btFd);
	btNode = btRoot;
	}
//==================================================================
RBtree::~RBtree() {
	delete btRoot;
	delete btWork;
	}
//==================================================================
// Add a key to the leaf level of a Btree

// returns 0 if all OK
// return -1 if error

int RBtree::Add(NDX_ID* ndxid, RKey &key, RData &data) {
	RNode*	rNode2;
	RKey	wkey;
	NODE	nnNew;
	int		len;
	int		rc;

	int res = Seek(ndxid, key);
	if (res > 0)
		return -1;								// can't add, key already there

	if (ndxid->ndxKeyNo == 0)					// 0519 if empty btree
		ndxid->ndxKeyNo++;

	len = key.GetKeyLen() + data.GetDataLen();
	if (btNode->BigEnough(len)) {
		btNode->InsertKeyData(ndxid->ndxKeyNo, key, data);
		btNode->Write();
		ndxid->ndxKey = key;
		return 0;								// no split, continue
		}
		
	// else NOT big enough, spliting action needed
	rNode2 = new RNode(btFd);					// create new node object
	nnNew = rNode2->NewNode();					// create new node on disk
	int lmark = btNode->Split(rNode2);			// xfer 50% of keys to new node
												// lmark = number in left side
	// insert the new key in the appropriate node
   if (ndxid->ndxKeyNo <= (lmark+1)) {       	// always try to put new key on left
		rc = btNode->InsertKeyData(ndxid->ndxKeyNo, key, data);
		assert(rc > 0);							// insert failed
		btPath[btLev] = btNode->GetCurrNode();
		lmark++;
		}
	else {										// else put new key into the right node
		ndxid->ndxKeyNo = ndxid->ndxKeyNo-lmark;
		rc = rNode2->InsertKeyData(ndxid->ndxKeyNo, key, data);
		assert(rc > 0);							// insert failed
		btPath[btLev] = nnNew;
		ndxid->ndxNode = nnNew;
		}
 
	btNode->Write();
	rNode2->Write();
	btNode->GetKey(&wkey, lmark);				// get left RHE key for new index 
	delete rNode2;
	ndxid->ndxStatus = ONKEY;
	rc = AddLoop(nnNew, &wkey, btLev);
	ndxid->ndxKey = key;
	return (rc);
	}
//==================================================================
// Delete the current key in the current node

// If current key:item is not at RHE of leaf node, then delete it and we're done
// get right sibling (if any) of current node and alter path to point to it
// if current node empty, delete node
// TBD if leaf node emptied, work back up to root, deleting ptr's to leaf or precedants
// TBD if RHE deleted, work back up to root, replacing any references to old RHE with new RHE

// return 0 for error, 1 for AOK

int RBtree::Delete(NDX_ID* ndxid) {
	NODE	nnLow;								// lower node
	NODE	nnUpper;							// upper node
	int		res;								// result
	int		level = btLev;						// current stack level

	res = Seek(ndxid, ndxid->ndxKey);
	if (ndxid->ndxStatus != ONKEY)
		return 0;								// we aren't sitting on a record, error
	SetNode(btLev, ndxid->ndxNode);
	btNode->DeleteKey(ndxid->ndxKeyNo);			// delete leaf level key
	btNode->Write();
	ndxid->ndxStatus = BETWEEN;
	int count = btNode->GetCount();

	if (count > 0 && ndxid->ndxKeyNo <= count)	// delete inside leaf node
		return 1;								// just a routine leaf deletion

	if (count == 0 || ndxid->ndxKeyNo > count) {// if RHE key was deleted or empty tree
		ndxid->ndxKeyNo = 1;					// advance to next node
		ndxid->ndxNode = btNode->GetRightSibling();
		if (ndxid->ndxNode)
			AdjustStack(ndxid->ndxNode, level, 1);
//		else
//			ndxid->ndxStatus = EOI;
		if (count > 0)							// RHE key gone, node still occupied
			return 1;
		}

	if (level == 0)								// on root
		return 1;								// exit delete, all is well

	// leaf node / index node empty
	do {
		nnLow = btNode->GetCurrNode();
		btNode->DeleteNode();					// delete the newly emptied node
		nnUpper = btPath[--level];				// get the next higher node from path
		SetNode(level, nnUpper);

		res = btNode->ScanNodeforP(nnLow);		// get pointer to deleted node
		btNode->DeleteIndex(res);				// delete item:ptr to deleted lower node
		btNode->Write();
		if (btNode->GetCount() > 0)
			return 1;							// not empty, bail out AOK
		if (level == 0) {						// if root node was emptied, new root
			nnLow = btNode->GetP0();				// get sole survivor descendant
			btRoot->Read(nnLow);
			btRoot->SetCurrNode(ROOT);			// place into Root
			btRoot->Write();
			for (int i = 1; i < btLev; i++)		// reduce path stack by one level
				btPath[i] = btPath[i+1];
			btLev--;
			btWork->Read(nnLow);
			btWork->DeleteNode();				// delete the newly emptied node
			return 1;							// exit OK
			}

		res = MergeBalance(level);
		} while (res < 0);

	return 1;
	}
//==================================================================
int RBtree::GetRecno(NDX_ID* ndxid) {
	int	recno;

	if (ndxid->ndxStatus == ONKEY || ndxid->ndxStatus == BETWEEN)
		recno = ndxid->ndxKey.GetKeyTail();
	else
		recno = -1;
	return (recno);
	}
//==================================================================
const char* RBtree::GetData(NDX_ID* ndxid) {
	return (btNode->GetData(ndxid->ndxKeyNo));
	}
//==================================================================
// position on the next key in the Btree
// return -1 if no right sibling node

int RBtree::MoveNext(NDX_ID* ndxid) {
	int		keyno;

	keyno = ndxid->ndxKey.GetKeyHead();
	Seek(ndxid, ndxid->ndxKey);
	ndxid->ndxKeyNo = btNode->NextKey(ndxid->ndxKeyNo);
	if (ndxid->ndxKeyNo == 0) {					// if we went past EONode
		ndxid->ndxNode = btNode->NextNode();
		ndxid->ndxKeyNo = 1;
		}
	if (ndxid->ndxNode == 0)					// off into never-never land
		return -1;
	btNode->GetKey(&ndxid->ndxKey, ndxid->ndxKeyNo);	// make key object using node's key
	if (keyno != ndxid->ndxKey.GetKeyHead())		// change of keyno
		return -1;
	return 1;
	}
//==================================================================
// position to the previous key

int RBtree::MovePrevious(NDX_ID* ndxid) {

	Seek(ndxid, ndxid->ndxKey);
	ndxid->ndxKeyNo = btNode->PrevKey(ndxid->ndxKeyNo);
	if (ndxid->ndxKeyNo <= 0) {					// if we went past EONode
		ndxid->ndxNode = btNode->PrevNode();
		if (ndxid->ndxNode > 0)
			ndxid->ndxKeyNo = btNode->GetCount();
		else
			return -1;
		}

	btNode->GetKey(&ndxid->ndxKey, ndxid->ndxKeyNo);
	return 1;
	}
//==================================================================
void RBtree::PrintTree() {
	NODE	nnNode;
	NODE	leftNode[10];
	
	printf("\nTree printout of index # ");
	printf("Root = %d\n", ROOT);
//	btWork->Read(ROOT);
	btNode = btRoot;
	btNode->PrintNode();
	for (int i = 0;; i++) {
		nnNode = btNode->GetP0();
		leftNode[i] = nnNode;
		btNode = btWork;
		while (nnNode) {
			btNode->Read(nnNode);
			btNode->PrintNode();
			nnNode = btNode->GetRightSibling();
			}
		if (btNode->GetP0() == 0)
			break;
		btNode->Read(leftNode[i]);
		}
	}
//==================================================================
// Search and return status
int RBtree::Search(NDX_ID* ndxid, RKey &key) {

	Seek(ndxid, key);
	if (ndxid->ndxStatus == ONKEY)
		return 0;
	if (ndxid->ndxStatus == BETWEEN)
		return 1;
	return -1;
	}
//==================================================================
// Seek for a key within the tree
 
// return n if key == Kn
// return 0 if key < K1 (also tree empty)
// return -i if Ki < key < Ki+1 or Kn < key

// modifies:
// ndxKeyNo = tree key that is <= search key
//			N.B. can be > Kn if no <= condition exists (IEOF)
// ndxStatus = ONKEY, BETWEEN, IEOF, or UNPOSITIONED
// ndxNode
// ndxKey

int RBtree::Seek(NDX_ID* ndxid, RKey &key) {
	int		res;								// scan result
	NODE	nnNode;								// node # of working node
	
	if (btRoot->GetCount() == 0) {				// empty tree
		ndxid->ndxKeyNo = 0;
		ndxid->ndxStatus = UNPOSITIONED;
		return (0);
		}
	// descend levels of the tree
	for (nnNode = ROOT, btNode = btRoot, btLev = 0;;btLev++) {
		btPath[btLev] = nnNode;
		res = btNode->ScanNode(key);
		if (btNode->GetP0() == 0)				// if this is a leaf node, quit
			break;
		// if res > 0 ==> exact hit, use Pi-1
		//        < 0 ==> between, use Pi
		//        = 0 ==> less than K1, use P0
		res = (res > 0) ? res-1 : -res;
		nnNode = btNode->GetPi(res);
		btNode = btWork;
		btNode->Read(nnNode);
		}

	// at leaf level
	// if res > 0 then exact hit
	//        = 0 then key smaller than K1
	//        < 0 then key smaller than K(1+|res|)
	if (res <= 0) {								// no hit, between or > last key
		ndxid->ndxKeyNo = -res + 1;
		if (ndxid->ndxKeyNo > btNode->GetCount()) // beware key > Kn
			ndxid->ndxStatus = IEOF;
//			ndxid->ndxNode = 0; //151125 undecided
//			ndxid->ndxKeyNo = 0;
//			return (0);
		else
			ndxid->ndxStatus = BETWEEN;
		}
	else {
		ndxid->ndxKeyNo = res;
		ndxid->ndxStatus = ONKEY;
		}

	ndxid->ndxNode = nnNode;
	btNode->GetKey(&ndxid->ndxKey, ndxid->ndxKeyNo);
	return res;
	}
//=====================================================================
// P R I V A T E    M E T H O D S
//=====================================================================
// Insert a key into a node

// assumes that "node" object is active and is nnNode
// if a split occurs, returns new split node # &
//		modifies "key" to hold promoted split key

NODE RBtree::AddKey(const int insert, NODE nnNode, RKey *key, int level) {   
	RNode	*rNode2;							// split node object
	NODE	nnNew;								// right split node

	if (btNode->BigEnoughPtr(key->GetKeyLen())) { // key + LNODE
		btNode->InsertKeyPtr(insert, *key, nnNode);
		btNode->Write();
		return 0;								// no split, continue
		}
		
	// else NOT big enough, spliting action needed
	rNode2 = new RNode(btFd);					// create new node object
	nnNew = rNode2->NewNode();					// create new node in 2nd storage
	int lmark = btNode->Split(rNode2);			// xfer 50% of keys to new node
												// lmark = number in left side
	// insert the new key in the appropriate node
	if (insert <= (lmark+1)) {       			// always try to put new key on left
		btNode->InsertKeyPtr(insert, *key, nnNode);
		btPath[level] = btNode->GetCurrNode();
		lmark++;
		}
	else {										// else put new key into the right node
		rNode2->InsertKeyPtr(insert-lmark, *key, nnNode);
		btPath[level] = nnNew;
		}
 
	btNode->GetKey(key, lmark);					// get left RHE key for new index 

	rNode2->SetP0(btNode->GetPi(lmark));		// (right) node P0 point to the RHE
	btNode->DeleteKey(lmark);
//	printf("Btree:AddKey after ndx split new node\n");
//	rNode2->PrintNode();
	btNode->Write();
	rNode2->Write();							// key of the left node
	delete rNode2;
	return nnNew;
	}
//==================================================================
// work back up tree, modifying root if necessary

//int RBtree::AddLoop(const int insert, NODE nnNode, RKey *key, int level) {
int RBtree::AddLoop(NODE nnNode, RKey *key, int level) {
	NODE	nnLeft;
	NODE	nnRight;
	NODE	nnUpper;
	int		res;

	while (nnNode) {							// if a split occurred
		if (level == 0) {						// did the root node split?
			nnLeft = btNode->NewNode();			// write left node in new spot
			btNode->Write();
			if (btPath[0] == ROOT)
				btPath[0] = nnLeft;
			nnRight = btNode->GetRightSibling();
			btWork->Read(nnRight);				// fix right sibling's left pointer
			btWork->SetLeftSibling(nnLeft);
			btWork->Write();
			btRoot->Clear();					// clean out root (aka btNode here)
			btRoot->SetCurrNode(ROOT);
			btRoot->InsertKeyPtr(1, *key, nnNode);// insert the new root key
			btRoot->SetP0(nnLeft);				// set P0
			btRoot->Write();					// write root back into it's spot
			for (int i = ++btLev; i > 0; i--)	// scootchie stack up
				btPath[i] = btPath[i-1];
			btPath[0] = ROOT;
			break;
			}
		nnUpper = btPath[--level];
		SetNode(level, nnUpper);
		res = btNode->ScanNode(*key);			// TBD - what if we hit?
		nnNode = AddKey(-res+1, nnNode, key, level);
		}

	return 0;
	}
//==================================================================
// Adjust upper inode key after a lower node's RHE was changed

//	if RHE of upper node points to changed node, go up another level
// if RHE key of RHE leaf node changed, do nothing

// modifies btNode

int RBtree::Adjust(NODE nnNode, RKey *key, int level) {
	NODE		nnUpper;						// node above current node
	NODE		nnLower;						// next lower node
	int		res;

	while (level > 0) {
		nnUpper = btPath[--level];
		SetNode(level, nnUpper);
		res = btNode->ScanNodeforP(nnNode)+1;	// which index key points to us?
		if (res <= btNode->GetCount()) {		// if it isn't the RHE
			nnLower = btNode->GetPi(res);		// save key's ptr
			btNode->DeleteKey(res);
			nnLower = AddKey(res, nnLower, key, level);
			AddLoop(nnLower, key, level);		// replace key in node
			break;
			}
		nnNode = nnUpper;
		}
	return 0;
	}
//==================================================================
// When the leaf level changes nodes, work back up the stack
// making sure all the root and index nodes are correctly in the path

void RBtree::AdjustStack(NODE nnNode, int level, int direction) {
	RNode	rPar(btFd);
	NODE	nnUpper;
	int	rc;

	btPath[level] = nnNode;
	while(level > 1) {							// root+1
		nnUpper = btPath[--level];
		rPar.Read(nnUpper);
		rc = rPar.ScanNodeforP(nnNode);
		if (rc >= 0)							// if lower node found
			return;
		if (direction > 0)
			nnNode = rPar.NextNode();
		else
			nnNode = rPar.PrevNode();
		btPath[level] = nnNode;
		}
	}
//==================================================================
// If an index node was emptied, then try to do one of the following:
//		1) merge remaining pointer into right sibling
//		2) borrow one item from the right sibling
//		3) merge into the left sibling
//		4) borrow one item from the left sibling

// returns:	1 if all done (by borrowing)
//				0 nothing accomplished (tree reduction indicated)
//				-1 if a deletion was done (btNode now empty)

int RBtree::MergeBalance(int level) {
	RNode	rSib(btFd);							// sibling work node object
	RNode	rDesc(btFd);							// descendant work node object
	RKey	tkey;
	NODE	nnDesc;								// descendant node
	NODE	nnNode;
	NODE	nnSource;							// 

	NODE nnSib = btNode->GetRightSibling();
	if (nnSib) {								// if there IS a right sibling
		rSib.Read(nnSib);
		nnDesc = btNode->GetP0();
		nnSource = rSib.GetP0();
		rDesc.Read(nnDesc);
		rDesc.GetLastKey(&tkey);
		if (rSib.BigEnoughPtr(tkey.GetKeyLen())) {// APPEND RIGHT if room
			rSib.InsertKeyPtr(1, tkey, nnSource);// add it at front of sibling
			rSib.SetP0(nnDesc);
			rSib.Write();
			AdjustStack(nnSib, level, 1);
			return (-1);
			}
		else {									// BORROW RIGHT if sib is full
			btNode->InsertKeyPtr(1, tkey, nnSource);// Add to our tail
			btNode->Write();
			nnSource = rSib.GetPi(1);
			rSib.SetP0(nnSource);				// it's P0 is the borrowed node
			rSib.GetKey(&tkey, 1);				// get the key
			rSib.DeleteKey(1);					// all borrowed up
			rSib.Write();
			nnNode = btNode->GetCurrNode();
			Adjust(nnNode, &tkey, level);
			return 1;
			}
		}

	nnSib = btNode->GetLeftSibling();
	if (nnSib) {								// if there IS a left sibling
		rSib.Read(nnSib);
		nnDesc = rSib.GetPn();
		rDesc.Read(nnDesc);
		rDesc.GetLastKey(&tkey);
		nnSource = btNode->GetP0();
		int n = rSib.GetCount();
		if (rSib.BigEnoughPtr(tkey.GetKeyLen())) {	// APPEND LEFT if room
			rSib.InsertKeyPtr(n+1, tkey, nnSource);// add it to sibling RHE
			rSib.Write();
			AdjustStack(nnSib, level, -1);
			return (-1);
			}
		else {									// BORROW LEFT  if sib is full
			btNode->InsertKeyPtr(1, tkey, nnSource);// add to our head
			btNode->SetP0(nnDesc);				// P0 is former Pn
			btNode->Write();
			rSib.DeleteKey(n);					// delete Pn
			rSib.Write();
			nnNode = btNode->GetCurrNode();		// fix parent pointer
			nnDesc = rSib.GetPn();
			rDesc.Read(nnDesc);
			rDesc.GetLastKey(&tkey);
			Adjust(nnSib, &tkey, level);
			return 1;
			}
		}

	return 0;									// no siblings
	}

//==================================================================
// set btNode pointer to the btRoot object or btWork based on stack depth

void RBtree::SetNode(const int depth, const NODE nnNode) {

	if (depth == 0)
		btNode = btRoot;
	else {
		btNode = btWork;
		btNode->Read(nnNode);
		}
	}
