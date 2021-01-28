#pragma once

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

	char*		GetChar();
	int			GetInt();
	float		GetFloat();
	double		GetDouble();
		
	int			SetData(const char *data);
	int			SetData(int data);
	int			SetData(float data);
	int			SetData(double data);
	int			ClearField();

	char*		GetName() { return (fldName); }
	int			GetLen() { return (fldLen); }
	int			GetType() { return (fldType); }
	};

class RIndex {
	};

class RTable {

public:
	int			DbAddRecord();
	int			DbClearRecord();
	int			DbDeleteRecord();
	int			DbRefreshRecord();
	int			DbUpdateRecord();

	int			DbFirstRecord(RIndex* ndx);
	int			DbLastRecord(RIndex* ndx);
	int			DbNextRecord(RIndex* ndx);
	int			DbPrevRecord(RIndex* ndx);
	int			DbSearchRecord(RIndex* ndx, const char* key);

	RIndex*		DbGetIndexObject(const char* ndxname);
	RField*		DbGetFieldObject(const char* fldname, int offset = 0);
	char*		DbGetChar(const char* fldname, int offset = 0);
	int			DbGetInt(const char* fldname, int offset = 0);
	float		DbGetFloat(const char* fldname, int offset = 0);
	double		DbGetDouble(const char* fldname, int offset = 0);
	int			DbSetField(const char* fldname, const char* data, int offset = 0);
	int			DbSetField(const char* fldname, int data, int offset = 0);
	int			DbSetField(const char* fldname, float data, int offset = 0);
	int			DbSetField(const char* fldname, double data, int offset = 0);


	RIndex*		DbMakeIndex(const char* ndxname, const char *tmplte, RField* fldlst[]);
	// ***************** dba only
	int			GetRecno();
	// ************************
	};

class RDbf {

public:
	int			DbMakeTable(const char* tabname, RField* fldlst[]);
	RTable*		DbOpenTable(const char* tabname);
	int			DbCloseTable(RTable* rel);
	int			DbDeleteTable(const char* relname);

	void		PrintTree();
	};


class Sapphire {


private:
	void*		dbDbfRoot;		// dbf linked list root

public:
	Sapphire();
	~Sapphire();

	RDbf*		DbCreateFile(const char* dbfname);
	RDbf*		DbLogin(const char* dbfname);
	int			DbLogout(RDbf* dbfile);

	int			DbGetErrno();
	};
#endif
