/*
+-------------------+
| Sapphire Database |
+-------------------+
	|
	v
	+----------------------+        +-----------+
	| Database File (.dbf) |------->| ... .dbfs |-----> NULL
	+----------------------+        +-----------+
		|           |    
		v			v    
		+-------+   +-------+        +------------+
		| Btree |<--| Table |------->| ... tables |-----> NULL
		+-------+   +-------+        +------------+
                       |   
					   v   
				       +-------+        +-------------+
					   | Index |------->| ... indexes |-----> NULL
					   +-------+        +-------------+
					   |       |
					   | field |----->fld1
					   | list  |--------->fld2
					   | .     |------------->fld3
					   | .     |-----------------> ...
					   | .     |----->fldn
					   +-------+

Data object - controls a group of data items of varying types
Key object - controls a key with 1 to 5 data items
Page object - manages memory block that contains Data and Key objects
Node object - controls btree nodes, owns page, does all R/W I/O
Field object - controls a single database data field
Index object - controls one btree with multiple tables and index items
Table object - foundation of a database, each table points to index
	linked list, points to associated btree
Dbf object - owns OS file, linked list to tables, owns btree
Sapphire - master object, does file based db ops, linked list to dbf's
*/

// 180202 - added ret code to DbFirstRecord, DbLastRecord
// might be applicable to Add, Clear, Delete, Refresh, and Update
// 180214 - added error returns if rel invalid

#include "OS.h"
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
#include "RSapphire.h"
#ifdef LINUX
#include "utility.h"
#endif

#ifdef MSDOS
errno_t	err;
#endif

Sapphire::Sapphire() {
	dbDbfRoot = NULL;
	}

Sapphire::~Sapphire() {
	RDbf*	dbf;
	RDbf*	nxtdbf;

	dbf = dbDbfRoot;
//	for (; dbf; dbf = nxtdbf) {
//		nxtdbf = dbf->GetLink();
	if (dbf != NULL)
		delete dbf;
//		}
	}
//=============================================================================
/*
DbLogin - 	Log onto an existing Sapphire database.

	RDbf* DbLogin(const char* dbfname)

	This function opens the specified Sapphire database (using an existing Sapphire
	object), and returns a RDbf* for use in other functions (i.e. DbOpenRelation, DbDropRelation,
	DbMakeRelation, DbLogout) to subsequently identify the database.

Parameters
	dbfname - a zero terminated ascii character string database file name. The 
	name can include a device (drive), directory, paths, and a filename in the
	final path directory. The file name may not have a  ".dbf" suffix, it will 
	be added.

Return value
	On failure, a NULL will be returned.
	If the Login operation is sucessful, a non-NULL RDbf* is returned. 
	Otherwise a NULL will be returned. DbGetErrno may be call to get the dos 
	level error number.

See also
	DbCreate, DbLogout

*/
RDbf* Sapphire::DbLogin(const char* dbfname) {
	RDbf*	dbf;
	RDbf*	ldbf;
	int		rc;

	// scan for dbf file already open
	for (ldbf = dbDbfRoot; ldbf; ldbf = ldbf->GetLink()) {
		if (stricmp(ldbf->GetName(), dbfname) == 0)
			return(0);
		}

	dbf = new RDbf(dbfname);
	rc = dbf->Login(dbfname);
	if (rc <= 0) {
		delete dbf;
		return 0;
		}
	dbf->SetLink((RDbf*)dbDbfRoot);			// add to linked list of dbf's
	dbDbfRoot = dbf;
	return (dbf);
	}

//=============================================================================
/*
DbLogout - Log out of a Sapphire database.

	int		DbLogout(RDbf* dbf)

	This function closes the specified database.
	Closing the database releases all resources.  

Parameters
	dbf - the database object must have been initialized by a sucessful DbLogin 
	operation.

Return
	A zero return indicates a sucessful DbLogout, a -1 indicates failure.

*/
int Sapphire::DbLogout(RDbf* dbf) {
	RDbf*	xdbf;

	if (dbf == dbDbfRoot)					// if root dbf is target
		dbDbfRoot = dbf->GetLink();			// set root to sucessor (if any)
	else {
		for (xdbf = dbDbfRoot; xdbf; xdbf = dbf->GetLink()) {
			if (xdbf->GetLink() == dbf) {
				xdbf->SetLink(dbf->GetLink());
				break;
				}
			}
			return (-1);
		}
	delete dbf;
	return 0;
	}
//===========================================================================
/*
DbCreate - Create a new Sapphire database file

	RDbf*	DbCreateFile(const char* dbfname);

	Creates a new database file. Returns a RDbf* if sucessful, or a value 
	of 0 (NULL) if not sucessful. If a system error caused the failure, the 
	errno q.v. 	will be stored and can be accessed with the DbGetErrno function.
	
Parameters
	dbfname - a zero terminated ascii character string database file name. The 
	name can include a device (drive), directory, paths, and a filename in the
	final path directory. The file name may have a  ".dbf" suffix, but it will 
	be added automatically if absent.

Return
	On failure, a NULL will be returned.
	A sucessful create will return a non-null pointer to a new RDbf object;

See also
	DbLogin, DbLogout
*/

RDbf*	Sapphire::DbCreateFile(const char *dbfname) {
	RDbf*	dbf;
	RDbf*	ldbf;
	int		rc;

	// scan for dbf file already open
	for (ldbf = dbDbfRoot; ldbf; ldbf = ldbf->GetLink()) {
		if (stricmp(ldbf->GetName(), dbfname) == 0)
			return(0);
		}

	dbf = new RDbf(dbfname);
	rc = dbf->Create(dbfname);
	if (rc <= 0) {
		delete dbf;
		return (NULL);
		}
	dbf->SetLink((RDbf*)dbDbfRoot);			// add to linked list of dbf's
	dbDbfRoot = dbf;
	return (dbf);
	}

//==============================================================================
// return system error number "errno"
int Sapphire::DbGetErrno() {

#ifdef MSDOS
	return err;
#endif
#ifdef LINUX
	return errno;
#endif
	}