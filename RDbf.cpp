// 180302 windows/dos errno put into create
// 180304 closer delete of dbfname to errno()

#define MSDOS
//#define LINUX

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include "dbdef.h"
#include "RField.h"
#include "RData.hpp"
#include "RKey.hpp"
#include "Ndbdefs.h"
#include "RPage.hpp"
#include "RNode.hpp"
#include "RBtree.hpp"
#include "RIndex.h"
#include "RTable.h"
#include "RDbf.h"

#ifdef MSDOS
#include <io.h>
extern	errno_t	err;
#endif
#ifdef LINUX
#include <sys/types.h>
#include <unistd.h>
extern	int	err;
#endif

//=============================================================================
RDbf::RDbf(const char* dbfname) {
	int len = strlen(dbfname);
	dbfName = new char[len+1];
	strcpy(dbfName, dbfname);
	dbfLink = NULL;
	dbfRelRoot = NULL;
	dbfBtree = NULL;
	}
//=============================================================================
RDbf::~RDbf() {
	RTable*	rel;
	RTable*	nxtrel;

	for (rel = dbfRelRoot; rel; rel = nxtrel) {
		nxtrel = rel->GetLink();
		delete rel;
		}
#ifdef MSDOS
	if (dbfFd > 0)
		_close(dbfFd);
#endif
#ifdef LINUX
	if (dbfFd > 0)
		close(dbfFd);
#endif
	delete [] dbfName;
	if (dbfBtree)
		delete dbfBtree;
	}
//===========================================================================
// File oriented ops
//===========================================================================
// return dos file number if AOK, else NULL (errno is set)
int	RDbf::Create(const char *dbfname) {
	char	zeros[NODESIZE];
	int		len;
	char*	dosname;

	len = strlen(dbfname);
	dosname = new char[len+5];
	strcpy(dosname, dbfname);
	if (strstr(dosname, ".dbf") == 0)
		strcat(dosname, ".dbf");
	memset(zeros, 0, NODESIZE);
#ifdef MSDOS
	_set_errno(0);
	dbfFd = _open(dosname, _O_BINARY | _O_RDWR | _O_CREAT, _S_IREAD | _S_IWRITE);
	_get_errno(&err);
#endif
#ifdef LINUX
	errno = 0;
	dbfFd = open(dosname, S_IRUSR | S_IWUSR);
	err = errno;
#endif

	delete [] dosname;
	if (err > 0)
		return 0;
	if (dbfFd) {
#ifdef MSDOS
		_write(dbfFd, zeros, NODESIZE);
#endif
#ifdef LINUX
		write(dbfFd, zeros, NODESIZE);
#endif
		dbfAvail = 0;
		dbfBtree = new RBtree(dbfFd, dbfAvail);
		return dbfFd;
		}
	return 0; // couldn't open
	}
//=============================================================================
// return dos file number if AOK, else NULL (errno is set)
int RDbf::Login(const char* dbfname) {
	int		len;
	char*	dosname;

	len = strlen(dbfname);
	dosname = new char[len + 5];
	strcpy(dosname, dbfname);
	if (strstr(dosname, ".dbf") == 0)
		strcat(dosname, ".dbf");
#ifdef MSDOS
	_set_errno(0);
	dbfFd = _open(dosname, _O_BINARY | _O_RDWR | _O_CREAT, _S_IREAD | _S_IWRITE);
	_get_errno(&err);
#endif
#ifdef LINUX
	errno = 0;
	dbfFd = open(dosname, S_IRUSR | S_IWUSR);
	err = errno;
#endif
	delete[] dosname;
	if (err > 0)
		return 0;
	if (dbfFd) {
		_lseek(dbfFd, 0L, SEEK_SET);
		_read(dbfFd, &dbfAvail, sizeof(NODE));
		dbfBtree = new RBtree(dbfFd, dbfAvail);
		}
	return dbfFd;
	}
//===========================================================================
// Relation oriented ops
//===========================================================================
/*
DbMakeTable	- Make a new data table (relation)

int	DbMakeTable(const char* relname, RField* fldlst[]);


Creates a new data relation in the specified database.

To create a new relation, supply the handle of the database (dbfID) that will contain
the relation, the name of the relation (which must not already exist), and
descriptions for each field.  The function creates the data relation

Fields are described by the RField class.

DbMakeRelation does not open the relation just created.

Parameters
relname - an ascii character string of the name of the relation being created
fldlst - a NULL terminated array of RField object pointers

Returns
-1 if DbMakeRelation fails

*/
int	RDbf::DbMakeTable(const char* relname, RField* fldlst[]) {
	RTable	rel;			// dummy
	int		rc;

	rc = rel.MakeRelation(dbfBtree, relname, fldlst);
	return rc;
}
/*
DbOpenTable	- Open a data relation

RTable*	DbOpenTable(const char* relname)

Opens the specified data Table (Tablename) in the specified database
(dbfID), prepares the data relation for further access and modification,
returns a relation_id (relID) and creates a record buffer for the relation.
The record buffer is initially clear. All secondary indexes are initialized
and set to UNPOSITIONED status.

To open a relation, the user must be logged into the database file (*.dbf) containing
the relation (DbLogin).

Parameters
	relname - an ascii character string of the name of the relation being created

Returns
	On sucess, a pointer to the table object is returned
	On failure, a NULL will be returned.
*/
//===========================================================================
RTable* RDbf::DbOpenTable(const char* relname) {
	RTable*	rel;
	int		rc;

	rel = new RTable;
	rc = rel->OpenRelation((dbfID)this, dbfBtree, relname);
	if (rc < 0) {
		delete rel;
		return 0;
		}
	rel->SetLink(dbfRelRoot);
	dbfRelRoot = rel;
	return rel;
	}
//===========================================================================
/*
DbCloseTable	- Close a Table

int	DbCloseTable(RTable* rel)

Closes the specified data relation. The relid handle is no longer valid.

Parameters

Returns
	-1 if relid is invalid
*/
int RDbf::DbCloseTable(RTable* rel) {
	RTable*	xrel;

	if (rel == 0)
		return -1;
	rel->CloseRelation();
	if (dbfRelRoot == rel)
		dbfRelRoot = rel->GetLink();
	for (xrel = dbfRelRoot; xrel; xrel = xrel->GetLink()) {
		if (xrel->GetLink() == rel) {
			xrel->SetLink(rel->GetLink());
			}
		}
	delete rel;
	return 0;
	}
//===========================================================================
/*
DbDeleteTable

int	DbDeleteTable(const char* relname)

Delete all database system records from a specified database, delete all
indexes and data records.

Parameters
relname - an ascii character string of the name of the relation being created

Returns
-1 if relid is invalid

*/
int RDbf::DbDeleteTable(const char* relname) {
	RTable*	relp;
	int		rc;

	// sweep thru linked list of rel's to make sure relation isn't open
	for (relp = dbfRelRoot; relp; relp = relp->GetLink()) {
		if (_stricmp(relname, relp->GetRelname()) == 0)
			return (-1);
		}
	rc = dbfRelRoot->DropRelation(dbfBtree, relname);
	return rc;
	}
//===========================================================================
int	RDbf::SetLink(RDbf* dbf) {
	dbfLink = dbf;
	return (0);
	}

void RDbf::PrintTree() {
	dbfBtree->PrintTree();
	}
