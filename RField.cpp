//210126 - user supplied char data longer than fldLen causes heap overrun

#include <string.h>
#include "RField.h"
#include "RKey.hpp"
#include "Ndbdefs.h"
#include "dbdef.h"

//=============================================================================
RField::RField() {
	fldName = NULL;
	fldData = NULL;
	fldOwner = FALSE;		// data area owned elsewhere
	}
//=============================================================================
// const flddata???
RField::RField(const char* fldname, char* flddata, int fldtype, int fldlen) {
	int	len = strlen(fldname) + 1;
	fldName = new char[len];
	strcpy(fldName, fldname);
	fldData = flddata;
	fldType = fldtype;
	fldLen = fldlen;
	fldChg = FALSE;
	fldOwner = FALSE;		// data area owned elsewhere
	}
//=============================================================================

RField::RField(const char* fldname, int fldtype, int fldlen) {
	int	len = strlen(fldname) + 1;
	fldName = new char[len];
	strcpy(fldName, fldname);
	fldType = fldtype;
	if (fldtype == INT)
		fldLen = sizeof(int);
	else if (fldtype == FP)
		fldLen = sizeof(float);
	else if (fldtype == DP)
		fldLen = sizeof(double);
	else
		fldLen = fldlen + 1;
	fldData = new char[fldLen+1];	//210127
	fldChg = 0;
	fldOwner = TRUE;		// data area owned by 'this'
	}
//=============================================================================

RField::RField(const char* fldname, int fldtype) {
	int	len = strlen(fldname) + 1;
	fldName = new char[len];
	strcpy(fldName, fldname);
	fldType = fldtype;
	if (fldtype == INT)
		fldLen = sizeof(int);
	else if (fldtype == FP)
		fldLen = sizeof(float);
	else if (fldtype == DP)
		fldLen = sizeof(double);
	else
		fldLen = FLDMAX;
	fldData = new char[fldLen+1];	//210127
	fldChg = 0;
	fldOwner = TRUE;		// data area owned by 'this'
	}
//=============================================================================
RField::~RField() {
	if (fldName)
		delete [] fldName;
	if (fldOwner && fldData)
		delete [] fldData;
	}
//=============================================================================
int RField::ClearField() {
	double	zero = 0;
	
	if (fldType & STRING)
		strcpy(fldData, "");				// empty string
	else if (fldType == FP)
		memcpy(fldData, &zero, sizeof(float));	// zero
	else if (fldType == DP)
		memcpy(fldData, &zero, sizeof(double));	// zero
	else
		memcpy(fldData, &zero, sizeof(int));	// zero
	fldChg = TRUE;
	return (0);
	}
//=============================================================================
// fetch a char pointer to character type field data
const char* RField::GetChar() {
	const char* p;

	p = fldData;
	return (p);
}
//=============================================================================
// fetch an int field
int RField::GetInt() {
	int		i = 0;

	memcpy(&i, fldData, sizeof(int));
	return (i);
}
//=============================================================================
// fetch a float field
float RField::GetFloat() {
	float		f = 0;

	memcpy(&f, fldData, sizeof(float));
	return (f);
}
//=============================================================================
// fetch a double field
double RField::GetDouble() {
	double		d = 0;

	memcpy(&d, fldData, sizeof(double));
	return (d);
}
//=============================================================================
int RField::SetData(const char *data) {
	int	len;

//	strcpy(fldData, data);           //210126
	strncpy(fldData, data, fldLen);  //210126
	fldData[fldLen] = 'f';
	fldData[fldLen] = '\0';          //210126
	len = strlen(data) + 1;
	fldChg = TRUE;
	return (len);
	}
//=============================================================================
int RField::SetData(int data) {

	memcpy(fldData, &data, sizeof(int));
	fldChg = TRUE;
	return (sizeof(int));
	}
//=============================================================================
int RField::SetData(float data) {

	memcpy(fldData, &data, sizeof(float));
	fldChg = TRUE;
	return (sizeof(float));
	}
//=============================================================================
int RField::SetData(double data) {

	memcpy(fldData, &data, sizeof(double));
	fldChg = TRUE;
	return (sizeof(double));
	}

