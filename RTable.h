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
		RTable*	relLink;			// linked list pointer

	public:
		RTable();
		~RTable();

		int		MakeRelation(RBtree* btree, const char *relname, RField *fldlst[]);
		int		OpenRelation(RBtree* btree, const char* relname);
		int		DropRelation(RBtree* btree, const char *relname);

		int		DbAddRecord();
		int		DbClearRecord();
		int		DbDeleteRecord();
		int		DbRefreshRecord();
		int		DbUpdateRecord();

		int		DbFirstRecord(RIndex* ndx);
		int		DbNextRecord(RIndex* ndx);
		int		DbLastRecord(RIndex* ndx);
		int		DbPrevRecord(RIndex* ndx);
		int		DbSearchRecord(RIndex* ndx, const char* key);

		RField*	DbGetFieldObject(const char* fldname, int offset = 0);

		const char* DbGetCharPtr(const char* fldname, int offset = 0);
		int		DbGetCharCopy(const char* fldname, char* data, int len, int offset);
		int		DbGetInt(const char* fldname, int offset = 0);
		float	DbGetFloat(const char* fldname, int offset = 0);
		double	DbGetDouble(const char* fldname, int offset = 0);

		int		DbSetField(const char* fldname, const char* data, int offset = 0);
		int		DbSetField(const char* fldname, int data, int offset = 0);
		int		DbSetField(const char* fldname, float data, int offset = 0);
		int		DbSetField(const char* fldname, double data, int offset = 0);

		int		DbDeleteIndex(const char* ndxname);
		RIndex*	DbMakeIndex(const char* ndxname, const char *tmplte, RField *fldlst[]);
		RIndex*	DbGetIndexObject(const char* ndxname);
		int		SetLink(RTable* rel);
		RTable*	GetLink() {return (relLink);}
		char*	GetRelname() {return (relName);}
		int		GetRecno();

	private:
		int		AddIndex(RIndex* ndx);
		int		AnyFieldChanged();
		int		GetHighestIndex(RIndex &ndxrel, RIndex &ndxndx);
		int		DeleteFields();
		int		DeleteIndexes();
		RField*	ScanFieldList(const char* fldname, int offset = 0);

};
#endif