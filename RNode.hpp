#if !defined(_NODE_H_)

#define _NODE_H_
                       
#define	NODESIZE	4096                       
#define KEYSPACE	4084
//#define		NODESIZE	162                      
//#define		KEYSPACE	150

class RNode {

	private:
		RPage		nodePage;
		int			nodeFd;
		NODE		nodeCurr;
		NODE		nodeAvail;

		NODE		nodeP0;						//////
		NODE		nodeLeftSibling;				// NODE
		NODE		nodeRightSibling;				//
		char		nodeMemory[KEYSPACE];		//////
                                
	public:
		RNode(const int fd);
		RNode(const int fd, NODE fdAvail);

		int			BigEnough(const int len);
		int			BigEnoughPtr(const int len) {return (BigEnough(len + LNODE));}
		void		Clear();
		int			DeleteKey(const int keyNo);
		int			DeleteIndex(const int keyNo);
		int			DeleteNode();
		int			GetCount() {return (nodePage.GetSlots());}
		NODE		GetCurrNode() {return nodeCurr;}
		char*		GetData(int keyno);
		void		GetKey(RKey *key, const int keyno);
		void		GetLastKey(RKey *key);
		NODE		GetLeftSibling() {return (nodeLeftSibling);}
		NODE		GetP0() {return (nodeP0);}
		NODE		GetPi(const int i);
		NODE		GetPn();
  		NODE		GetRightSibling() {return (nodeRightSibling);}
		int			InsertKey(const int keyNo, RKey &key);
		int			InsertKeyPtr(const int keyNo, RKey &key, NODE node);
		int			InsertKeyData(const int keyNo, RKey &key, RData &data);
		NODE		NewNode();
		int			NextKey(int keyno);
		NODE		NextNode();
		int			PrevKey(int keyno);
		NODE		PrevNode();
		int         Read(NODE);
		void		PrintNode();
 		int 		ScanNode(RKey &key);
		int			ScanNodeforP(NODE desc);
		void		SetCurrNode(NODE i) {nodeCurr = i;}
		void		SetLeftSibling(NODE rnode) {nodeLeftSibling = rnode;}
		void		SetP0(NODE node) {nodeP0 = node;}
		void		SetRightSibling(NODE rnode) {nodeRightSibling = rnode;}
		int			Split(RNode *node2);
		int			Write();

	private:
		char*		GetKi(const int i) {return (nodePage.GetItem(i-1));}
		char*		GetItem(const int i) {return (nodePage.GetItem(i-1));}
	};
#endif
