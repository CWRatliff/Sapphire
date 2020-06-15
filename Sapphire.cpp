// 180202 - added ret code to DbFirstRecord, DbLastRecord
// might be applicable to Add, Clear, Delete, Refresh, and Update
// 180214 - added error returns if rel invalid

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

//	errno_t	err;			

Sapphire::Sapphire() {
	dbDbfRoot = NULL;
	}

Sapphire::~Sapphire() {
	RDbf*	dbf;
	RDbf*	nxtdbf;

	dbf = (RDbf*)dbDbfRoot;
	for (; dbf; dbf = nxtdbf) {
		nxtdbf = dbf->GetLink();
		delete dbf;
		}
	}
//=============================================================================
/*
DbLogin - 	Log onto an existing Sapphire database.

	RDbf* DbLogin(const char* dbfname)

	This function opens the specified Sapphire database (using an existing Sapphire
	object), and returns a dbfID handle to identify the opened database. The handle must 
	be saved, as it is used in other functions (i.e. DbOpenRelation, DbDropRelation,
	DbMakeRelation, DbLogout) to subsequently identify the database. Furthermore
	the dbfID must never be altered while the Login is in effect, otherwise 
	unpredictable effects will follow, few of which are likely to be wanted.

Parameters
	dbfname - a zero terminated ascii character string database file name. The 
	name can include a device (drive), directory, paths, and a filename in the
	final path directory. The file name may have a  ".dbf" suffix, but it will 
	be added automatically if absent.

Return value
	On failure, a NULL will be returned.
	If the Login operation is sucessful, a non-NULL dbfID handle is returned. 
	Otherwise a NULL will be returned. DbGetErrno may be call to get the dos 
	level error number.

See also
	DbCreate, DbLogout

*/
RDbf* Sapphire::DbLogin(const char* dbfname) {
	RDbf*	dbf;
	int		rc;

	dbf = new RDbf(dbfname);
	rc = dbf->Login(dbfname);
	if (rc == 0) {
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

	This function closes the previously Login'ed database with the dbfID handle.
	Closing the database releases all resources.  

Parameters
	dbf - the database object must have been initialized by a sucessful DbLogin 
	operation.

Return
	A zero return indicates a sucessful DbLogout, a -1 indicates failure.

*/
int Sapphire::DbLogout(RDbf* dbf) {
	RDbf*	predbf;							// previous dbf object in linked list
	RDbf*	sdbf;

	sdbf = dbDbfRoot;
//	if ((RDbf*)dbfid == dbf) {
	if (dbf == dbDbfRoot) {					// if root dbf is target
		dbDbfRoot = dbf->GetLink();			// set root to sucessor (if any)
		delete dbf;
		return 0;
		}

	for (predbf = dbDbfRoot;; predbf = sdbf) {	// second or later dbf in linked list
		sdbf = predbf->GetLink();
		if (sdbf == NULL)
			return 0;
		if (sdbf == dbf) {
			predbf->SetLink(sdbf->GetLink()); // link over target link
			delete dbf;
			return 0;
			}
		}
	return (-1);							// no find	
	}
//>>>>>>>>>>>>>>>>>>>>> see DbCloseRelation for better implementation

//===========================================================================
/*
DbCreate - Create a new Sapphire database file

	RDbf*	DbCreateFile(const char* dbfname);

	Creates a new database file. Returns a RDbf* if sucessful, or a value 
	of 0 (NULL) if not sucessful. If a system error caused the failure, the 
	errno q.v. 	will be stored and can be accessed with the DbGetErrno function.
	After a sucessful call, the dbfID can be further used to create tables,
	indexes et.al.
	
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
	int		rc;

	dbf = new RDbf(dbfname);
	rc = dbf->Create(dbfname);
//	if (rc == NULL) {
	if (rc == 0) {
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
	return errno;
	}
