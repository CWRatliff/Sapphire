// 180323

typedef	void*	dbfID;
typedef	void*	relID;
typedef	void*	ndxID;
//typedef	int		fldID;

// data types / index descriptors

#define MSKNOCASE	0x40
#define MSKDESC		0x80
#define ASCENDING	0
#define INT			1
#define FP			2
#define	DP			3
#define STRNUMERIC	4
#define	STRING		0x20
#define STRNOCASE	0x60

int MakeSearchKey(char* key, const char *tmplte, ...);

#if !defined(_DB_H_)

#define _DB_H_

class RField {

	private:
		char*	fldName;
		char*	fldData;		// data item
		int		fldType;		// ref ndbdefs.h
		int		fldLen;			// max fld length
		int		fldChg;			// change status TRUE/FALSE
		int		fldOwner;		// data area owned by object -> TRUE

	public:
		RField();
		RField(const char* fldname, char* flddata, int fldtype, int fldlen);
		RField(const char* fldname, int fldtype, int fldlen);
		RField(const char* fldname, int fldtype);
		~RField();
/*
		int		ClearField();
		char*	GetFieldName() {return (fldName);}
		int		GetFieldLen() {return (fldLen);}
		int		GetFieldType() {return (fldType);}
		char*	GetFieldData() {return (fldData);}
		int		GetFieldStatus() {return (fldChg);}
		int		GetFieldDataInt();
		int		SetFieldData(const char *data);
		int		SetFieldData(int data);
		int		SetFieldData(float data);
		int		SetFieldData(double data);
		void	ClearFieldStatus() {fldChg = 0;}
		*/
	};
class RFieldC : public RField {
public:
	const char*	Data();
};
class RFieldI : public RField {
public:
	int		Data();
};
class RFieldF : public RField {
public:
	float	Data();
};
class RFieldD : public RField {
public:
	double	Data();
};

class Sapphire {

	private:
		dbfID	dbDbfRoot;		// dbf linked list root

	public:
		Sapphire();
		~Sapphire();

		dbfID	DbCreate(const char* dbfname);
		dbfID	DbLogin(const char* dbfname);
		int		DbLogout(dbfID dbf);

		int		DbDeleteRelation(dbfID dbf, const char* relname);
		int		DbMakeRelation(dbfID dbf, const char* relname, RField* fldlst[]);
		relID	DbOpenRelation(dbfID dbf, const char* relname);
		int		DbCloseRelation(relID rel);

		int		DbDeleteIndex(relID rel, const char* ndxname);
		ndxID	DbMakeIndex(relID rel, const char* ndxname, const char *tmplte, RField* fldlst[]);
		ndxID	DbGetIndex(relID rel, const char* ndxname);

		int		DbFirst(relID rel, ndxID ndx);	
		int		DbLast(relID rel, ndxID ndx);
		int		DbNext(relID rel, ndxID ndx);
		int		DbPrev(relID rel, ndxID ndx);
		int		DbSearch(relID rel, ndxID ndx, const char* key);

		int		DbClearField(relID relid, const char* fldname, int offset = 0);

		const char*	DbGetChar(relID relid, const char* fldname, int offset = 0);
		int		DbGetInt(relID relid, const char* fldname, int offset = 0);
		float	DbGetFloat(relID relid, const char* fldname, int offset = 0);
		double	DbGetDouble(relID relid, const char* fldname, int offset = 0);

		int		DbGetField(relID relid, const char* fldname, char* data, int offset = 0);
		int		DbGetField(relID relid, const char* fldname, int* data, int offset = 0);
		int		DbGetField(relID relid, const char* fldname, float* data, int offset = 0);
		int		DbGetField(relID relid, const char* fldname, double* data, int offset = 0);

		int		DbSetField(relID relid, const char* fldname, const char* data, int offset = 0);
		int		DbSetField(relID relid, const char* fldname, int data, int offset = 0);
		int		DbSetField(relID relid, const char* fldname, float data, int offset = 0);
		int		DbSetField(relID relid, const char* fldname, double data, int offset = 0);


		int		DbAddRecord(relID rel);
		int		DbClearRecord(relID rel);
		int		DbDeleteRecord(relID rel);
		int		DbRefreshRecord(relID rel);
		int		DbUpdateRecord(relID rel);

		int		DbGetErrno();
		void	PrintTree(relID rel);
	};
#endif
