#if !defined(_DBF_H_)

#define _DBF_H_

class RDbf {

	private:
		int		dbfFd;			// DOS file #
		char*	dbfName;
		RBtree*	dbfBtree;		// Btree index for this DB
		RTable*	dbfRelRoot;		// relation linked list root
		RDbf*	dbfLink;
		NODE	dbfAvail;		// free allocated node (if any)

	public:
		RDbf(const char* dbname);
		~RDbf();

		int		Create(const char* dbfname);
		int		DbDeleteTable(const char* relname);
		int		Login(const char* dbfname);

		int		DbMakeTable(const char* relname, RField* fldlst[]);
		RTable*	DbOpenTable(const char* relname);
		int		DbCloseTable(RTable* rel);

		const char* GetName() { return dbfName;}
		RDbf*	GetLink() {return (dbfLink);}
		int		SetLink(RDbf* dbf);

		void	PrintTree();
	};
#endif