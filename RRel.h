#if !defined(_RELATION_H_)

#define _RELATION_H_

class RTable {

	private:
		char*	relName;
		int		relnFlds;
		int		relNdxNo;			// data records index No
		int		relRecNo;			// data record number
		int		relRecStatus;		// status of current record
		int		relOwner;			// TRUE if 'this' allocated relFldLst
		RIndex*	relDataNdx;			// data item index
		RIndex*	relCurNdx;			// index last used for position
		RIndex*	relNdxRoot;			// linked list of indexes
		RField** relFldLst;			// array of Field object ptrs
		RBtree*	relNdx;				// Btree associated
		dbfID	relParent;			// dbf than 'owns' table
		RTable*	relLink;			// linked list pointer

	public:
		RTable();
		~RTable();

		int		MakeRelation(RBtree* btree, const char *relname, RField *fldlst[]);
		int		OpenRelation(dbfID dbf, RBtree* btree, const char* relname);
		int		CloseRelation();
		int		DropRelation(RBtree* btree, const char *relname);

		int		DbAddRecord();
		int		DbClearRecord();
		int		DbDeleteRecord();
		int		DbRefreshRecord();
		int		DbUpdateRecord();
		RField* DbGetField(const char* fldname, int offset = 0);

		int		DbFirstRecord(RIndex* ndx);
		int		DbNextRecord(RIndex* ndx);
		int		DbLastRecord(RIndex* ndx);
		int		DbPrevRecord(RIndex* ndx);
		int		DbSearchRecord(RIndex* ndx, const char* key);
//		int		DbSearchRecord(RIndex* ndx, RKey& key);


		int		ClearField(const char* fldname, int offset = 0);
		RField*	DbGetFieldObject(const char* fldname, int offset = 0);

		const	char*	DbGetChar(const char* fldname, int offset = 0);
		int		DbGetInt(const char* fldname, int offset = 0);
		float	DbGetFloat(const char* fldname, int offset = 0);
		double	DbGetDouble(const char* fldname, int offset = 0);

		int		DbGetField(const char* fldname, char* data, int offset = 0);
		int		DbGetField(const char* fldname, int* data, int offset = 0);
		int		DbGetField(const char* fldname, float* dat, int offset = 0);
		int		DbGetField(const char* fldname, double* data, int offset = 0);

		int		DbSetField(const char* fldname, const char* data, int offset = 0);
		int		DbSetField(const char* fldname, int data, int offset = 0);
		int		DbSetField(const char* fldname, float data, int offset = 0);
		int		DbSetField(const char* fldname, double data, int offset = 0);

		int		FetchData(const char* fldname, char* data, int offset = 0);
		int		FetchData(const char* fldname, int* data, int offset = 0);
		int		FetchData(const char* fldname, float* dat, int offset = 0);
		int		FetchData(const char* fldname, double* data, int offset = 0);
		int		StoreData(const char* fldname, const char* data, int offset = 0);
		int		StoreData(const char* fldname, int data, int offset = 0);
		int		StoreData(const char* fldname, float data, int offset = 0);
		int		StoreData(const char* fldname, double data, int offset = 0);

		int		DbDeleteIndex(const char* ndxname);
		RIndex*	DbMakeIndex(const char* ndxname, const char *tmplte, RField *fldlst[]);
		RIndex*	DbGetIndexObject(const char* ndxname);
		int		SetLink(RTable* rel);
		RTable*	GetLink() {return (relLink);}
		relID	GetParent() {return (relParent);}
		char*	GetRelname() {return (relName);}

		RField*	GetField(const char* fldname, int offset = 0);

	private:
		int		AddIndex(RIndex* ndx);
		int		GetHighestIndex(RIndex &ndxrel, RIndex &ndxndx);
//		RField*	GetField(const char* fldname, int offset = 0);
		int		DeleteFields();
		int		DeleteIndexes();
	};
#endif