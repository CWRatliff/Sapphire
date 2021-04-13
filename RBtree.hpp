#if !defined(_BTREE_H_)

#define _BTREE_H_

#define	LEVELS	20

class RBtree {

	private:
		int			btFd;						// database file handle
		int			btLev;						// stack level
		RNode		*btRoot;					// root node
		RNode		*btWork;					// second node (non-root)
		RNode		*btNode;					// work node
		NODE		btPath[LEVELS];				// stack
			     
	public:
		RBtree(const int fileHandle, NODE fileAvail);			// constructor
		~RBtree();								// destructor

		int			Add(NDX_ID* ndxid, RKey &key, RData &data);
		int			Delete(NDX_ID* ndxid);
		const char*		GetData(NDX_ID* ndxid);
	    int			GetRecno(NDX_ID* ndxid);
		int			MoveNext(NDX_ID* ndxid);
		int			MovePrevious(NDX_ID* ndxid);
		void		PrintTree();
		int			Search(NDX_ID* ndxid, RKey &key);
		int			Seek(NDX_ID* ndxid, RKey &key);

	private:
		NODE		AddKey(const int insert, NODE theNode, RKey *key, int level);
		int			AddLoop(NODE theNode, RKey *key, int level);
		int			MergeBalance(int level);
		int			Adjust(NODE nnNode, RKey *key, int level);
		void		AdjustStack(NODE nnNode, int level, int direction);
		void		SetNode(const int depth, const NODE nnNode);
	};

#endif
