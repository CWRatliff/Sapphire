// 180214 - implemented offsetted fields in Fetch set
// 180214 - field type agreement checked in Fetch set (ret -2 on missmatch)
// 180302 - offsets in Store set
// 180328 - SearchRecord - rc from Find stepped on by Fetch

#include "dbdef.h"
#include "RField.h"
#include "RPage.hpp"
#include "RKey.hpp"
#include "RData.hpp"
#include "RKey.hpp"
#include "Ndbdefs.h"
#include "RNode.hpp"
#include "RBtree.hpp"
#include "RIndex.h"
#include "RTable.h"
#include "utility.h"
#include <string.h>
#include <stdarg.h> 

// sysrel
#define	NFLDSREL	3
RField fldrel1("relname", STRNOCASE, MAXNAME+1);
RField fldrel2("relnflds", INT, sizeof(int));
RField fldrel3("relndxno", INT, sizeof(int));
RField* fldrellst[] = {&fldrel1, &fldrel2, &fldrel3, NULL};

// sysatr
#define	NFLDSFLD	4
RField fldfld1("relname", STRNOCASE, MAXNAME+1);
RField fldfld2("fldname", STRNOCASE, MAXNAME+1);
RField fldfld3("fldtype", INT, sizeof(int));
RField fldfld4("fldlen", INT, sizeof(int));
RField* fldfldlst[] = {&fldfld1, &fldfld2, &fldfld3, &fldfld4, NULL};

// sysndx
#define	NFLDSNDX	9
RField fldndx1("relname", STRNOCASE, MAXNAME+1);
RField fldndx2("ndxname", STRNOCASE, MAXNAME+1);
RField fldndx3("relndxno", INT, sizeof(int));
RField fldndx4("fldasdec", STRNOCASE, MAXKEY+1);
RField fldndx5("fldname1", STRNOCASE, MAXNAME+1);
RField fldndx6("fldname2", STRNOCASE, MAXNAME+1);
RField fldndx7("fldname3", STRNOCASE, MAXNAME+1);
RField fldndx8("fldname4", STRNOCASE, MAXNAME+1);
RField fldndx9("fldname5", STRNOCASE, MAXNAME+1);
RField* fldndxlst[] = {&fldndx1, &fldndx2, &fldndx3, &fldndx4,
	&fldndx5, &fldndx6, &fldndx7, &fldndx8, &fldndx9, NULL};
//
// consider that ndxxxxX RIndexes might not be necessary
// would require linear searches for fields and indexes
// but lessened complexity might make it worthwhile

//============================================================================
RTable::RTable() {
	relNdxRoot = NULL;
	relLink = NULL;
	relName = NULL;
	relnFlds = 0;
	relRecNo = 0;
	relOwner = FALSE;
	}
//============================================================================
RTable::~RTable() {

	if (relName != NULL) {
		if (strncmp(relName, "sys", 3))		// sysxxx have static fldlst's
			DeleteFields();					// so dont delete them
		delete [] relName;
		}
	if (relOwner)
		delete [] relFldLst;
	DeleteIndexes();
	}

//===========================================================================
/*
DbAddRecord	- Add a new record to the data relation

Adds a new record to the specified data relation, writes the content of the data
relation's record buffer to the new record and updates all indexes on the data relation.

Use DbClearRecord to empty the record in the record buffer.  Use DbSetField to put
data into the empty record.  Use DbAdd to write the new record to the data relation.

The next available record number is automatically assigned to the record number field.

Parameters
relid - the handle to an open relation

Returns
-1 if relid is invalid
*/
//============================================================================
// add a new data record and add all associated indexes
int	RTable::DbAddRecord() {
	RData	data;
	RIndex*	ndx;
	int		rc;
	char	str[DATAMAX];

	ItemBuild(str, relnFlds, relFldLst);	// assemble record for fields
	data.SetData(str);
	rc = relDataNdx->WriteData(data, ++relRecNo);
	if (rc < 0)
		return rc;
	for (ndx = relNdxRoot; ndx; ndx = ndx->GetLink())	// create and add index record
		ndx->Add(relRecNo);					// to each index of current table
	return 0;
}
//============================================================================
int RTable::DbClearRecord() {
	int		i;

	for (i = 0; i < relnFlds; i++) {
		relFldLst[i]->ClearField();
		relFldLst[i]->SetFieldStatus();
	}
	relRecStatus = CHANGED;
	return 0;
}
//============================================================================
int RTable::DbDeleteRecord() {
	RIndex* ndx;
	RKey	key;

	relCurNdx->GetCurrentKey(&key);	// get current key
	for (ndx = relNdxRoot; ndx; ndx = ndx->GetLink())
		ndx->Delete();				// delete index keys
	relDataNdx->Delete();			// delete data key

	DbSearchRecord(relCurNdx, key.GetKeyStr());	// search for deleted key to get 'next'
	return 0;
}
//============================================================================
int	RTable::DbRefreshRecord() {

	relDataNdx->Fetch(relRecNo, relFldLst);
	relRecStatus = VIRGIN;
	relNdxRoot->PresetIndexes(relRecNo);
	return 0;
}
//============================================================================
int	RTable::DbUpdateRecord() {
	RData	data;
	RIndex* ndx;
	char	datastr[DATAMAX];

	// delete old data record, write new one
	// TBD - ReWrite that would only delete/write if size > slot
	if (relRecStatus != VIRGIN) {				// no data changes, STET
		relDataNdx->Delete();
		ItemBuild(datastr, relnFlds, relFldLst);
		data.SetData(datastr);
		relDataNdx->WriteData(data, relRecNo);

		for (ndx = relNdxRoot; ndx; ndx = ndx->GetLink())
			ndx->UpdateIndex();
		relRecStatus = VIRGIN;					// added 170122
		relNdxRoot->PresetIndexes(relRecNo);
	}
	return 0;
}
//============================================================================
//int RTable::ClearField(int fldno) {
int RTable::ClearField(const char* fldname, int offset) {
	RField*	fld;

//	if (relRecStatus == VIRGIN) // 170121 dont see any use/benefit
//		relNdxRoot->PresetIndexes(relRecNo);		// prep for alteration
	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	fld->ClearField();
	fld->SetFieldStatus();
	relRecStatus = CHANGED;
	return 0;
	}
//===========================================================================
/*
DbClearRecord - Clear a data record

Puts nulls / zeros in all fields of the current record in the record buffer.
This function is intended as a way cleaning up the record buffer before
adding new records.  The record is left unpositioned.

Parameters
relid - the handle to an open relation

Returns
-1 if relid is invalid
*/
//============================================================================
// TBD get rid of? called in RDbf
int RTable::CloseRelation() {
	// might update noRecs if it existed
	// any cleanup? fields? indexes?
	return 0;
	}
//===========================================================================
/*
DbDeleteIndex - delete a named index of a relation

int	DbDeleteIndex(relID relid, const char* ndxname)

Delete one index of a relation. The data records are unaffected as are
other indexes.

Parameters
relid - the handle to an open relation
ndxname - an ascii character string of the name of the index being deleted

Returns
-1 if relid is invalid
*/
/*int Sapphire::DbDeleteIndex(relID rel, const char* ndxname) {

	if (rel == 0)
		return -1;
	int rc = ((RTable*)rel)->DropIndex(ndxname);
	return rc;
}*/
//============================================================================
// delete named index

int RTable::DbDeleteIndex(const char* ndxname) {
	int		rc;
	int		recno;
	int		ndxrecno;
	RKey	keyw;			// work key;
	RIndex*	ndx;

	RIndex	ndxndx((relID)this, "sysndx", SYSNDX, relNdx);
	RIndex	ndxndxx((relID)this, "sysndx", SYSNDXX, relNdx);
	ndx = NULL;

	keyw.MakeSearchKey("s", relName);

	// locate index record for 'relname' and 'ndxname'
	rc = ndxndxx.Find(keyw);
	if (rc != 0)
			return 0;
	while (rc >= 0) {					// search thru ndxndxx/ndxndx to find 'ndxname'
		recno = ndxndxx.GetRecno();
		rc = ndxndx.Fetch(recno, fldndxlst);
		if (rc < 0)
			return -1;					// 'relname'/'ndxname' non-exisient, bail out
		rc = _stricmp(fldndx2.GetFieldName(), ndxname);
		if (rc == 0)
			break;						// got it
		rc = ndxndxx.Next();
		}
	if (rc < 0)							// 'relname'/'ndxname' non-exisient, bail out
		return -1;

	// get ndxndxno then delete all btree items with relndxno as daddr
	// AKA - delete all index entries with this ndxrecno

	ndxrecno = fldndx3.GetFieldDataInt();
	ndx = new RIndex((relID)this, "", ndxrecno, relNdx);
	rc = ndx->First();
	while (rc >= 0) {
		ndx->Delete();
		rc = ndx->First();		// first == next
		}
	delete ndx;

	// delete sysndxx & and sysndx records for 'relname','ndxname'

	rc = ndxndxx.Find(keyw);

	while (rc >- 0) {
		recno = ndxndxx.GetRecno();
		rc = ndxndx.Fetch(recno, fldndxlst);
		if (rc < 0)
			return -1;
		rc = _stricmp(fldndx2.GetFieldName(), ndxname);
		if (rc == 0) {
			ndxndxx.Delete();
			ndxndx.Delete();
			break;
			}
		rc = ndxndxx.Next();
		}

	return 0;
	}
//============================================================================
// delete all of this table's:
// indexes
// records
// sysatr's
// sysrel
int RTable::DropRelation(RBtree* btree, const char* relname) {
	int		rc;
	int		recno;
	int		ndxrecno;
	RKey	keyw;			// work key;
	RKey	keyk;			// index key
	RIndex*	ndx;

	RIndex	ndxrel((relID)this, "sysrel", SYSREL, btree);
	RIndex	ndxrelx((relID)this, "sysrel", SYSRELX, btree);
	RIndex	ndxatr((relID)this, "sysatr", SYSFLD, btree);
	RIndex	ndxatrx((relID)this, "sysatr", SYSFLDX, btree);
	RIndex	ndxndx((relID)this, "sysndx", SYSNDX, btree);
	RIndex	ndxndxx((relID)this, "sysndx", SYSNDXX, btree);

	keyw.MakeSearchKey("s", relname);

	// delete all index records for 'relname'
	rc = ndxndxx.Find(keyw);
	for(;rc >= 0;) {
		// get ndxndxno then delete all btree items with relndxno as daddr
		recno = ndxndxx.GetRecno();
		rc = ndxndx.Fetch(recno, fldndxlst);
		if (rc < 0)
			break;
		rc = _stricmp(fldndx1.GetFieldData(), relname);
		if (rc == 0) {
			ndxrecno = fldndx3.GetFieldDataInt();
			ndx = new RIndex((relID)this, "", ndxrecno, btree);
			rc = ndx->First();
			while (rc >= 0) {
				ndx->Delete();
				rc = ndx->First();		// first == next
				}
			delete ndx;
			}
		rc = ndxndxx.Next();
		}

	// delete all data records for 'relname'
	rc = ndxrelx.Find(keyw);
	if (rc == -1)
		return 0;
	recno = ndxrelx.GetRecno();
	rc = ndxrel.Fetch(recno, fldrellst);
	if (rc >= 0) {
		rc = _stricmp(fldrel1.GetFieldData(), relname);
		if (rc == 0) {
			ndxrecno = fldrel3.GetFieldDataInt();
			ndx = new RIndex((relID)this, "", ndxrecno, btree);
			rc = ndx->First();
			while (rc >= 0) {
				ndx->Delete();
				rc = ndx->First();		// first == next
				}
			delete ndx;
			}
		}


	// delete sysndx & sysndxx records for 'relname'
	rc = ndxndxx.Find(keyw);
	while (rc >= 0) {
		ndxndxx.GetCurrentKey(&keyk);
		rc = _stricmp(keyk.GetKeyStr()+1, relname);
		if (rc == 0) {
			recno = ndxndxx.GetRecno();
			ndxndx.Fetch(recno, fldndxlst); // field distribution not actually used
			ndxndx.Delete();
			ndxndxx.Delete();
			rc = ndxndxx.First();
			}
		else
			rc = ndxndxx.Next();
		}

	// delete sysatrx records for 'relname'
	rc = ndxatrx.Find(keyw);
	while (rc >= 0) {
		ndxatrx.GetCurrentKey(&keyk);
		rc = _stricmp(keyk.GetKeyStr()+1, relname);
		if (rc == 0) {
			recno = ndxatrx.GetRecno();
			ndxatr.Fetch(recno, fldndxlst);
			ndxatr.Delete();
			ndxatrx.Delete();
			rc = ndxatrx.First();
			}
		else
			rc = ndxatrx.Next();
		}

	// delete sysrelx records for 'relname'
	rc = ndxrelx.Find(keyw);
	while (rc >= 0) {
		ndxrelx.GetCurrentKey(&keyk);
		rc = _stricmp(keyk.GetKeyStr()+1, relname);
		if (rc == 0) {
			recno = ndxrelx.GetRecno();
			ndxrel.Fetch(recno, fldndxlst);
			ndxrel.Delete();
			ndxrelx.Delete();
			break;			// Give me a ping, Vasili. One ping only, please.
			}
		rc = ndxrelx.Next();
		}

	return 0;
	}
//===========================================================================
/*
	DbDeleteRecord	- Delete a database record

	Removes the current record from the specified data relation.

	The function updates all indexes.

	Parameters
	relid - the handle to an open relation

	Returns
	-1 if relid is invalid
	*/
//============================================================================
// look thru field objects to match name and supply RField object
//RField*	RTable::GetField(const char* fldname) {
RField*	RTable::DbGetFieldObject(const char* fldname, int offset) {
	int		i;

	for (i = 0; i < relnFlds; i++) {
		if (strcmp(relFldLst[i]->GetFieldName(), fldname) == 0) {
			//			return (relFldLst[i]);
			if ((i + offset) >= relnFlds)
				return NULL;
			return (relFldLst[i + offset]);
		}
	}
	return NULL;
}
//===========================================================================
/*
DbGet	- Get data from a data relation by field

const char*	DbGetChar(relID relid, const char* fldname, int offset)
int		DbGetInt(relID relid, const char* fldname, int offset)
float	DbGetFloat(relID relid, const char* fldname, int offset)
double	DbGetDoublerelID relid, const char* fldname, int offset)

DbGet<type> describes a family of functions that retrieve field data from the
record buffer of the specified data relation. A fieldname is used to specify
the field. The offset is a subscript-like reference mode. e.g. "field0" with
offset value of 2 would return contents of "field0"+2, presumably "field2"

Negative return value indicate an error condition.

Field type must match Fetch's data type or return -2;

Parameters
relid - the handle to an open relation
fldname - ascii string with fieldname
offset - susbscript-like field name reference extension

Returns
-1 if relid is invalid
*///===========================================================================
/*
DbGetField	- Get data from a data relation by field

int	DbGetField(relID relid, const char* fldname, char* data, int offset)
int	DbGetField(relID relid, const char* fldname, int* data, int offset)
int	DbGetField(relID relid, const char* fldname, float* data, int offset)
int	DbGetField(relID relid, const char* fldname, double* data, int offset)

DbGetField describes a family of functions that retrieve field data from the
record buffer of the specified data relation.  The retrieved data is placed at
the address specified by "data". A fieldname is used to specify
the field. The offset is a subscript-like reference mode. e.g. "field0" with
offset value of 2 would return contents of "field0"+2, presumably "field2"

Negative return value indicate an error condition.

Field type must match Fetch's data type or return -2;

Parameters
relid - the handle to an open relation
fldname - ascii string with fieldname
data - a pointer to the memory area to receive the database field's data
offset - susbscript-like field name reference extension

Returns
-1 if relid is invalid
*/
const char*	RTable::DbGetChar(const char* fldname, int offset) {
	const char* p;
	RField *fld;

	fld = GetField(fldname, offset);
	p = fld->GetFieldData();
	return p;
	}
int	RTable::DbGetInt(const char* fldname, int offset) {
	int		data;
	RField*	fld;
	int		type;

	if (strlen(fldname) == 0) {				// undocumented special code for dba
		memcpy(&data, &relRecNo, sizeof(int));
		return 0;
		}
	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != INT)				// 180214 type agreement check
		return -2;
	memcpy(&data, fld->GetFieldData(), sizeof(int));
	return data;
	}

float RTable::DbGetFloat(const char* fldname, int offset) {
	float	data;
	RField*	fld;
	int		type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != FP)				// 180214 type agreement check
		return -2;
	memcpy(&data, fld->GetFieldData(), sizeof(float));
	return data;
	}

double RTable::DbGetDouble(const char* fldname, int offset) {
	double	data;
	RField*	fld;
	int		type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != FP)				// 180214 type agreement check
		return -2;
	memcpy(&data, fld->GetFieldData(), sizeof(float));
	return data;
	}
//============================================================================
// FetchData Char

int RTable::DbGetField(const char* fldname, char* data, int offset) {
	RField*	fld;
	int		len;
	int		type;

	//	fld = GetField(fldname);
	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (!(type & STRING))				// 180214 type agreement check
		return -2;
	len = fld->GetFieldLen();
	strcpy_s(data, len, fld->GetFieldData());
	return 0;
}
//============================================================================
// FetchData Int
int RTable::DbGetField(const char* fldname, int* data, int offset) {
	RField*	fld;
	int		type;

	if (strlen(fldname) == 0) {				// special code for dba
		memcpy(data, &relRecNo, sizeof(int));
		return 0;
	}
	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != INT)				// 180214 type agreement check
		return -2;
	memcpy(data, fld->GetFieldData(), sizeof(int));
	return 0;
}
//============================================================================
// FetchData Float
int RTable::DbGetField(const char* fldname, float* data, int offset) {
	RField*	fld;
	int		type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != FP)				// 180214 type agreement check
		return -2;
	memcpy(data, fld->GetFieldData(), sizeof(float));
	return 0;
}
///===========================================================================
// FetchData Double
int RTable::DbGetField(const char* fldname, double* data, int offset) {
	RField*	fld;
	int		type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != DP)				// 180214 type agreement check
		return -2;
	memcpy(data, fld->GetFieldData(), sizeof(double));
	return 0;
}
/*
//============================================================================
// FetchData Char

//int RTable::FetchData(const char* fldname, char* data) {
int RTable::FetchData(const char* fldname, char* data, int offset) {
	RField*	fld;
	int		len;
	int		type;

//	fld = GetField(fldname);
	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (!(type & STRING))				// 180214 type agreement check
		return -2;
	len = fld->GetFieldLen();
	strcpy_s(data, len, fld->GetFieldData());
	return 0;
	}
//============================================================================
// FetchData Int
int RTable::FetchData(const char* fldname, int* data, int offset) {
	RField*	fld;
	int		type;

	if (strlen(fldname) == 0) {				// special code for dba
		memcpy(data, &relRecNo, sizeof(int));
		return 0;
		}
	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != INT)				// 180214 type agreement check
		return -2;
	memcpy(data, fld->GetFieldData(), sizeof(int));
	return 0;
	}
//============================================================================
// FetchData Float
int RTable::FetchData(const char* fldname, float* data, int offset) {
	RField*	fld;
	int		type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != FP)				// 180214 type agreement check
		return -2;
	memcpy(data, fld->GetFieldData(), sizeof(float));
	return 0;
	}
///===========================================================================
// FetchData Double
int RTable::FetchData(const char* fldname, double* data, int offset) {
	RField*	fld;
	int		type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != DP)				// 180214 type agreement check
		return -2;
	memcpy(data, fld->GetFieldData(), sizeof(double));
	return 0;
	}
	*/
//============================================================================
int	RTable::DbFirstRecord(RIndex* ndx) {
	int		rc;

	if (ndx == NULL)
		ndx = relDataNdx;		// use default
	else {
		if ((RTable*)(ndx->GetParent()) != this)
			return -1;
		}
	rc = ndx->First();
	if (rc < 0)
		return -1;

	relRecNo = ndx->GetRecno();
	rc = relDataNdx->Fetch(relRecNo, relFldLst);
	if (rc < 0)
		return -1;
	relRecStatus = VIRGIN;
	relNdxRoot->PresetIndexes(relRecNo);
	return 0;
	}
//===========================================================================
/*
DbGetIndex - Return the handle of a named index for a specified relation

ndxID	DbGetIndex(relID relid, const char* ndxname)

Parameters
relid - the handle to an open reltion
ndxname - an ascii character string of the name of the index being deleted

Returns
-1 - error
*/
//============================================================================
// search for index by ndxname thru linked list of 'this''s indexes
// returns NULL if no find
RIndex* RTable::DbGetIndexObject(const char* ndxname) {
	RIndex*	ndx;

	if (strlen(ndxname) == 0)
		return (relDataNdx);
	ndx = relNdxRoot->GetIndexHandle(ndxname);
	return ndx;
	}
//============================================================================
int	RTable::DbLastRecord(RIndex* ndx) {
	int		rc;

	if (ndx == NULL)
		ndx = relDataNdx;		// use default
	else {
		if ((RTable*)(ndx->GetParent()) != this)
			return -1;
		}
	rc = ndx->Last();
	if (rc < 0)
		return -1;

	relRecNo = ndx->GetRecno();
	rc = relDataNdx->Fetch(relRecNo, relFldLst);
	if (rc < 0)
		return -1;

	relRecStatus = VIRGIN;
	relNdxRoot->PresetIndexes(relRecNo);
	return 0;
	}
//============================================================================
// Make a new index for 'this' relation
//		template string:
//				a or d (scending)
//		followed by a list of field names
// build SYSNDX records and RIndex class object

//===========================================================================
/*
DbMakeIndex	- Make an index

ndxID DbMakeIndex(relID relid, const char* ndxname, const char *tmplte, RField* fldlst[]);

Parameters
relid - the handle to an open relation
ndxname - the zero terminated ascii string containing the name for a new
index. This name must not aleady be associated with this relation.
tmplte - an ascii string, each of which characters corresponds to a RField
object. Allowable descriptor letters are: 'a' or 'd' - ascending or descending
fldlst - a NULL terminated array of RField object pointers

Returns
The function returns the ndxID of the newly created index.  This process can
take some time if there are many records in the data relation.
*/
RIndex* RTable::DbMakeIndex(const char* ndxname, const char *tmplte, RField* fldlst[]) {
	RField*	fld;
	RIndex*	ndx;
	RKey	wkey;
	RData	data;
	char	str[DATAMAX];

	int		relrecno;
	int		descmask = 0;
	int		i = 0;
	int		j = 0;
	int		ndxno;
	int		rc;
	int		recno;
	int		type;
	char*	fldname;

	RIndex	ndxrel((relID)this, "sysrel", SYSREL, relNdx);
	RIndex	ndxndx((relID)this, "sysndx", SYSNDX, relNdx);
	RIndex	ndxndxx((relID)this, "sysndx", SYSNDXX, relNdx);

	wkey.MakeSearchKey("s", relName);
	rc = ndxndxx.Find(wkey);
	while (rc >= 0) {					// search thru ndxndxx/ndxndx to find 'ndxname'
		recno = ndxndxx.GetRecno();
		ndxndx.Fetch(recno, fldndxlst);
		rc = _stricmp(fldndx2.GetFieldName(), ndxname);
		if (rc == 0)
			return NULL;					// duplicate found
		rc = ndxndxx.Next();
		}

	ndxno = GetHighestIndex(ndxrel, ndxndx) + 1;// new ndxno = old high++
	
	fldndx5.SetFieldData("");
	fldndx6.SetFieldData("");
	fldndx7.SetFieldData("");
	fldndx8.SetFieldData("");
	fldndx9.SetFieldData("");					// preset fields to empty

 	ndx = new RIndex((relID)this, ndxname, ndxno, relNdx);
	// process 'a'/'d' template
	fldndx4.SetFieldData(tmplte);
	while (*tmplte) {
		if (j > 5 || fldlst[i] == NULL) {		// too many 'a'/'d' or too few fld's
			delete ndx;
			return NULL;
			}
		type = *tmplte++;
		descmask = ASCENDING;
		if (type == 'd')
			descmask = MSKDESC;
		fld = fldlst[i++];						// look up in field list
		fld = GetField(fld->GetFieldName());	// TBD looks wierd, 'fld' being used two ways
		ndx->AddField(*fld, descmask);
		fldname = fld->GetFieldName();
		fldndxlst[j+4]->SetFieldData(fldname);	// put field name in ndx data
		j++;
		}

	// build sysndx data record & index
	recno = ndxndx.GetLastRecNo() + 1;
	fldndx1.SetFieldData(relName);
	fldndx2.SetFieldData(ndxname);
	fldndx3.SetFieldData(ndxno);
	ItemBuild(str, NFLDSNDX, fldndxlst);
	data.SetData(str);
	ndxndx.WriteData(data, recno);				// write data entry in sysndx
	ndxndxx.AddField(fldndx1, ASCENDING);		// relname
	ndxndxx.Add(recno);							// write key entry in sysndxx

	// go through relation and make new index entry in this new index
	rc = relDataNdx->First();
	if (rc >= 0) {
		for (;;) {
			relrecno = relDataNdx->GetRecno();
			relDataNdx->Fetch(relFldLst);
			ndx->Add(relrecno);
			rc = relDataNdx->Next();
			if (rc < 0)
				break;
			}
		}

	AddIndex(ndx);		// add new index to linked list of indexes
	return ndx;
	}

//============================================================================
int	RTable::MakeRelation(RBtree* btree, const char *relname, RField *fldlst[]) {
	RKey	wkey;
	RData	data;
	char	str[DATAMAX];
	int		recno;
	int		i;
	int		ndxno = USERNDX;
	int		rc;

	RIndex	ndxrel((relID)this, "sysrel", SYSREL, btree);
	RIndex	ndxrelx((relID)this, "sysrel", SYSRELX, btree);
	RIndex	ndxatr((relID)this, "sysatr", SYSFLD, btree);
	RIndex	ndxatrx((relID)this, "sysatr", SYSFLDX, btree);
	RIndex	ndxndx((relID)this, "sysndx", SYSNDX, btree);
	RIndex	ndxndxx((relID)this, "sysndx", SYSNDXX, btree);

	// see if relation already exists
	wkey.MakeSearchKey("s", relname);
	rc = ndxrelx.Find(wkey);
	if (rc == 0)							// 1609111
		return -1;							// duplicate name

	// get new ndx # for new table
	ndxno = GetHighestIndex(ndxrel, ndxndx) + 1;

	// make sysrel data record
	recno = ndxrel.GetLastRecNo() + 1;		// get existing highest record number
	fldrel1.SetFieldData(relname);
	fldrel2.SetFieldData(3);
	fldrel3.SetFieldData(ndxno);
	ItemBuild(str, NFLDSREL, fldrellst);
	data.SetData(str);
	ndxrel.WriteData(data, recno);

	// sysrel's index
	ndxrelx.AddField(fldrel1, ASCENDING);		// relname
	ndxrelx.Add(recno);

	// make sysatr data/index records
	recno = ndxatr.GetLastRecNo() + 1;
	for (i = 0; fldlst[i]; i++) {
		fldfld1.SetFieldData(relname);
		fldfld2.SetFieldData(fldlst[i]->GetFieldName());
		fldfld3.SetFieldData(fldlst[i]->GetFieldType());
		fldfld4.SetFieldData(fldlst[i]->GetFieldLen());
		ItemBuild(str, NFLDSFLD, fldfldlst);
		data.SetData(str);
		ndxatr.WriteData(data, recno+i);
		ndxatrx.Reset();
		ndxatrx.AddField(fldfld1, ASCENDING);	// relname
		ndxatrx.Add(recno+i);
		}
	return 0;
	}
//============================================================================
int	RTable::DbNextRecord(RIndex* ndx) {
	int		rc;

	if (ndx == NULL)
		ndx = relDataNdx;
	rc = ndx->Next();
	if (rc < 0)
		return -1;

	relRecNo = ndx->GetRecno();
	rc = relDataNdx->Fetch(relRecNo, relFldLst);
	if (rc < 0)
		return -1;
	else {
		if ((RTable*)(ndx->GetParent()) != this)
				return -1;
		}
	relRecStatus = VIRGIN;
	relNdxRoot->PresetIndexes(relRecNo);
	// relCurNdx = ndx?? 161122
	return 0;
	}
// ===========================================================================
// Make a relation object and open relation
// 1. Find relation record in SYSREL
// 2. Make a RField object for each field in SYSFLD
// 3. Allocate a RField* array and fillin the Field addresses
// 4. Make a RIndex for indexes in SYSNDX, string together in linked list
//
//	return -1 if relation can't be found
int	RTable::OpenRelation(dbfID dbf, RBtree* btree, const char* relname) {
	int		i, j;
	int		count;
	int		rc;
	int		recno;
	int		type;
	char	ascdesc[MAXFLD+1];
	RKey	keyw;			// work key;
	RData	data;
	RField*	fld;
	RIndex*	ndx;

	relParent = dbf;
	relNdx = btree;
	i = strlen(relname);
	relName = new char[i+1];
	strcpy(relName, relname);

	// cheater open for sysrel
	if (strcmp(relname, "sysrel") == 0) {
		relNdxNo = SYSREL;					// data index no
		relnFlds = NFLDSREL;
		relFldLst = fldrellst;
		relRecNo = 0;
		relDataNdx = new RIndex((relID)this, "sysrel", SYSREL, btree);
		relCurNdx = new RIndex((relID)this, "sysrelx", SYSRELX, btree);
		relCurNdx->AddField(*relFldLst[0], ASCENDING);  //160911
		relDataNdx->SetLink(relCurNdx);
		relNdxRoot = relDataNdx;
		return 0;
		}
	// cheater open for sysatr
	if (strcmp(relname, "sysatr") == 0) {
		relNdxNo = SYSFLD;					// data index no
		relnFlds = NFLDSFLD;
		relFldLst = fldfldlst;
		relRecNo = 0;
		relDataNdx = new RIndex((relID)this, "sysatr", SYSFLD, btree);
		relCurNdx = new RIndex((relID)this, "sysatrx", SYSFLDX, btree);

		relCurNdx->AddField(*relFldLst[0], ASCENDING);	//160911
		relCurNdx->AddField(*relFldLst[1], ASCENDING);
		relDataNdx->SetLink(relCurNdx);
		relNdxRoot = relDataNdx;
		return 0;
		}
	// cheater open for sysndx
	if (strcmp(relname, "sysndx") == 0) {
		relNdxNo = SYSNDX;					// data index no
		relnFlds = NFLDSNDX;
		relFldLst = fldndxlst;
		relRecNo = 0;
		relDataNdx = new RIndex((relID)this, "sysndx", SYSNDX, btree);
		relCurNdx = new RIndex((relID)this, "sysndxx", SYSNDXX, btree);
		relCurNdx->AddField(*relFldLst[0], ASCENDING); //160911
		relCurNdx->AddField(*relFldLst[1], ASCENDING);
		relDataNdx->SetLink(relCurNdx);
		relNdxRoot = relDataNdx;
		return 0;
		}

	RIndex	ndxrel((relID)this, "sysrel", SYSREL, btree);
	RIndex	ndxrelx((relID)this, "sysrel", SYSRELX, btree);
	RIndex	ndxatr((relID)this, "sysatr", SYSFLD, btree);
	RIndex	ndxatrx((relID)this, "sysatr", SYSFLDX, btree);
	RIndex	ndxndx((relID)this, "sysndx", SYSNDX, btree);
	RIndex	ndxndxx((relID)this, "sysndx", SYSNDXX, btree);

	// get this relation's record from SYSREL
	keyw.MakeSearchKey("s", relName);
	rc = ndxrelx.Find(keyw);
	if (rc != 0)
		return -1;
	recno = ndxrelx.GetRecno();
	ndxrel.Fetch(recno, fldrellst);
	relNdxNo = fldrel3.GetFieldDataInt();		// data index no

	// now build field list
	// SYSFLD key = SYSFLD, relname, fldname
	// pass 1: count fields
	keyw.MakeSearchKey("s", relName);
	rc = ndxatrx.Find(keyw);
	if (rc == -1) {								// error in Find			
		delete [] relName;
		return(-1);
		}	

	// TBD - why not use relnFlds
	recno = ndxrelx.GetRecno();
	rc = ndxatr.Fetch(recno, fldfldlst);
	for (count = 1; count < MAXNDX; count++) {				// count fields
		rc = ndxatrx.Next();
		if (rc < 0)
			break;
		recno = ndxatrx.GetRecno();
		rc = ndxatr.Fetch(recno, fldfldlst);
		if (rc < 0)
			break;
		rc = strcmp(fldfld1.GetFieldData(), relname);	// still the same relation?
		if (rc)
			break;								// guess not
		}
	relnFlds = count;
	relFldLst = new RField*[count+1];			// make RField list array
	relFldLst[count] = NULL;					// array terminator
	relOwner = TRUE;							// deletable

	// pass 2: fill RField's
	keyw.MakeSearchKey("s", relName);
	rc = ndxatrx.Find(keyw);					// find error, is database corrupted?
	if (rc == -1)
		return -1;
	for (i = 0; i < count; i++) {
		recno = ndxatrx.GetRecno();
		rc = ndxatr.Fetch(recno, fldfldlst);
		fld = new RField(fldfld2.GetFieldData(), fldfld3.GetFieldDataInt(), fldfld4.GetFieldDataInt());
		relFldLst[i] = fld;
		rc = ndxatrx.Next();
		}

	relDataNdx = new RIndex((relID)this, "data", relNdxNo, btree);
	relRecNo = relDataNdx->GetLastRecNo();		// get highest data record address

	// now build ndx list
	// count # of indexes
	//	SYSNDX key = SYSNDX, relname
	relNdxRoot = relDataNdx;
	keyw.MakeSearchKey("s", relname);
	rc = ndxndxx.Find(keyw);
	if (rc == -1)
		return 0;
	for(;;) {
		recno = ndxndxx.GetRecno();
		rc = ndxndx.Fetch(recno, fldndxlst);
		rc = _stricmp(fldndx1.GetFieldData(), relname);
		if (rc != 0)
			break;
		strcpy(ascdesc, fldndx4.GetFieldData());
		ndx = new RIndex((relID)this, fldndx2.GetFieldData(), fldndx3.GetFieldDataInt(), btree);
		// scan atrlst[] to get RField ptrs for ndxFld array
		for (j = 0; j < MAXKEY; j++) {
			if (strlen(fldndxlst[j+3]->GetFieldData()) == 0)
				break;
			for (i = 0; i < relnFlds; i++ ) {
				if (strcmp(relFldLst[i]->GetFieldName(), fldndxlst[j+4]->GetFieldData()) == 0) {
					type = (ascdesc[i] == 'd') ? MSKDESC : ASCENDING;
					ndx->AddField(*relFldLst[i], type);
					break;
					}
				}
			}
		AddIndex(ndx);
		rc = ndxndxx.Next();
		if (rc < 0)
			break;
		}
	return (0);
	}
//============================================================================
int	RTable::DbPrevRecord(RIndex* ndx) {
	int		rc;

	if (ndx == NULL)
		ndx = relDataNdx;		// use default
	else {
		if ((RTable*)(ndx->GetParent()) != this)
			return -1;
		}
	rc = ndx->Prev();
	if (rc < 0)
		return -1;

	relRecNo = ndx->GetRecno();
	rc = relDataNdx->Fetch(relRecNo, relFldLst);
	if (rc < 0)
		return -1;

	relRecStatus = VIRGIN;
	relNdxRoot->PresetIndexes(relRecNo);
	return 0;
	}
//===========================================================================
/*
DbRefreshRecord	- Refresh the current record

Rereads the disk record for the current record, discarding any changes made
to the current record by DbSetField... or DbClearField...

Use this function to get a fresh copy of a record after aborting an edit
operation.

Parameters
relid - the handle to an open relation

Returns
-1 if relid is invalid
*/
/*int Sapphire::DbRefreshRecord(relID relid) {
if (relid == 0)
return -1;
((RTable*)relid)->RefreshRecord();
return 0;
}*/

//============================================================================
//int RTable::DbSearchRecord(RIndex* ndx, RKey& key) {
int RTable::DbSearchRecord(RIndex* ndx, const char* key) {
	int		rc, rc2;
	RKey	skey;

	if (ndx == NULL)				// default
		ndx = relDataNdx;
	if ((RTable*)(ndx->GetParent()) != this)
		return -1;
	skey.SetKey(key);
	rc = ndx->Find(skey);			// 0 if exact (possibly limited) match
	if (rc < 0)
		return -1;

	relRecNo = ndx->GetRecno();
	rc2 = relDataNdx->Fetch(relRecNo, relFldLst);
	if (rc2 < 0)
		return -1;

	relRecStatus = VIRGIN;
	relNdxRoot->PresetIndexes(relRecNo);
	relCurNdx = ndx;
	return rc;
	}
//============================================================================
int RTable::SetLink(RTable* rel) {
	relLink = rel;
	return 0;
	}
/*
//============================================================================
// StoreData Char
int RTable::StoreData(const char* fldname, const char* data, int offset) {
	RField*		fld;
	int			type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (!(type & STRING))				// 180302 type agreement check
		return -2;
	fld->SetFieldData(data);
	fld->SetFieldStatus();
	relRecStatus = CHANGED;
	return 0;
}
//============================================================================
// StoreData Int
int RTable::StoreData(const char* fldname, int data, int offset) {
	RField*		fld;
	int			type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != INT)				// 180302 type agreement check
		return -2;
	fld->SetFieldData(data);
	fld->SetFieldStatus();
	relRecStatus = CHANGED;
	return 0;
}*/
//===========================================================================
/*
DbSetField	- Change Field Content

int	DbSetField(relID relid, const char* fldname, const char* data, int offset)
int	DbSetField(relID relid, const char* fldname, int data, int offset)
int	DbSetField(relID relid, const char* fldname, float data, int offset)
int	DbSetField(relID relid, const char* fldname, double data, int offset)

Use this function to move data into the record buffer
for the data relation. Use DbUpdate to write the data to the data relation.
A fieldname is used to specify the field. The offset is a subscript-like mode. e.g. "field0" with offset value of 2
would set contents of "field0"+2, presumably "field2"

Offset
Parameters
relid - the handle to an open relation
fldname - ascii string with fieldname
data - a pointer or direct reference to the memory area to supply the database field's data
offset - susbscript-like field name reference extension

Returns
-1 if relid is invalid
*///============================================================================
// StoreData Float
int RTable::DbSetField(const char* fldname, float data, int offset) {
	RField*		fld;
	int			type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != FP)				// 180302 type agreement check
		return -2;
	fld->SetFieldData(data);
	fld->SetFieldStatus();
	relRecStatus = CHANGED;
	return 0;
}
//============================================================================
// StoreData Double
int RTable::DbSetField(const char* fldname, double data, int offset) {
	RField*		fld;
	int			type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != DP)				// 180302 type agreement check
		return -2;
	fld->SetFieldData(data);
	fld->SetFieldStatus();
	relRecStatus = CHANGED;
	return 0;
}
//============================================================================
// StoreData Char
int RTable::DbSetField(const char* fldname, const char* data, int offset) {
	RField*		fld;
	int			type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (!(type & STRING))				// 180302 type agreement check
		return -2;
	fld->SetFieldData(data);
	fld->SetFieldStatus();
	relRecStatus = CHANGED;
	return 0;
	}
//============================================================================
// StoreData Int
int RTable::DbSetField(const char* fldname, int data, int offset) {
	RField*		fld;
	int			type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != INT)				// 180302 type agreement check
		return -2;
	fld->SetFieldData(data);
	fld->SetFieldStatus();
	relRecStatus = CHANGED;
	return 0;
	}
/*//============================================================================
// StoreData Float
int RTable::StoreData(const char* fldname, float data, int offset) {
	RField*		fld;
	int			type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != FP)				// 180302 type agreement check
		return -2;
	fld->SetFieldData(data);
	fld->SetFieldStatus();
	relRecStatus = CHANGED;
	return 0;
	}
//============================================================================
// StoreData Double
int RTable::StoreData(const char* fldname, double data, int offset) {
	RField*		fld;
	int			type;

	fld = GetField(fldname, offset);
	if (fld == NULL)
		return -1;
	type = fld->GetFieldType();
	if (type != DP)				// 180302 type agreement check
		return -2;
	fld->SetFieldData(data);
	fld->SetFieldStatus();
	relRecStatus = CHANGED;
	return 0;
	}*/
//===========================================================================
/*
DbUpdateRecord	- Update a database record

Writes the content of the record buffer back to the data relation on storage.
This is only necessary if the record content has changed.

Parameters
relid - the handle to an open relation

Returns
-1 if relid is invalid
*/

//============================================================================
// Private methods
//============================================================================
// add a new index object to RHE of linked list of index objects for 'this'
int	RTable::AddIndex(RIndex* ndx) {
	RIndex*	andx;
	RIndex*	bndx;

	bndx = relNdxRoot;
	for (andx = relNdxRoot; andx; andx = andx->GetLink()) // find end of list
		bndx = andx;
	bndx->SetLink(ndx);	
	return 0;
	}
//============================================================================
int RTable::DeleteFields() {
	RField*	fld;
	int		i;

	for (i = 0; i < relnFlds; i++) {
		fld = relFldLst[i];
		if (fld == NULL)
			break;
		delete fld;
		}
	relnFlds = 0;
	return 0;
	}
//============================================================================
int RTable::DeleteIndexes() {
	RIndex*	ndx;
	RIndex* nxtndx;

	for (ndx = relNdxRoot; ndx; ndx = nxtndx) {
		nxtndx = ndx->GetLink();
		delete ndx;
		}
	relNdxRoot = NULL;
	return 0;
	}
//============================================================================
// look thru field objects to match name and supply RField object
//RField*	RTable::GetField(const char* fldname) {
RField*	RTable::GetField(const char* fldname, int offset) {
	int		i;

	for (i = 0; i < relnFlds; i++) {
		if (strcmp(relFldLst[i]->GetFieldName(), fldname) == 0) {
//			return (relFldLst[i]);
			if ((i + offset) >= relnFlds)
				return NULL;
			return (relFldLst[i + offset]);
			}
		}
	return NULL;
	}
//============================================================================
// sweep sysrel & sysndx to find the highest assigned ndx number
int RTable::GetHighestIndex(RIndex& ndxrel, RIndex& ndxndx) {
	int		highno = 0;
	int		nhighno = 0;
	int		rhighno = 0;
	int		rc;

	rc = ndxndx.First();					// start at first sysndx
	if (rc >= 0) {
		ndxndx.Fetch(fldndxlst);
		if (rc > 0) {
			nhighno = fldndx3.GetFieldDataInt();// look at 'relndxno'
			for (;;) {							// cycle thru sysndx
				rc = ndxndx.Next();
				if (rc < 0)
					break;
				ndxndx.Fetch(fldndxlst);
				if (fldndx3.GetFieldDataInt() > highno)
					nhighno = fldndx3.GetFieldDataInt();
				}
			}
		}

	rc = ndxrel.First();					// start at first sysrel
	if (rc > 0) {
		ndxrel.Fetch(fldndxlst);
		rhighno = fldrel3.GetFieldDataInt();// look at 'relndxno'
		for (;;) {							// cycle thru sysndx
			rc = ndxrel.Next();
			if (rc < 0)
				break;
			ndxrel.Fetch(fldndxlst);
			if (fldrel3.GetFieldDataInt() > highno)
				rhighno = fldrel3.GetFieldDataInt();
			}
		}

	highno =(rhighno > nhighno) ? rhighno : nhighno;
	if (highno < USERNDX)
		highno = USERNDX;

	return (highno);
	}
