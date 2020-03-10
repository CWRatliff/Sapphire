
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include "RField.h"
#include "RKey.hpp"
#include "ndbdefs.h"
#include "utility.h"
#include "dbdef.h"
//===================================================================
// Assemble a key item or data item from individual fields
// use index as source for data types (esp descending)
int ItemBuild(char *dest, int nflds, RField *fldlst[], int ndxtype[]) {
	char	*p;
	int		ilen;
	int		itype;
	const char*	iaddr;

	p = dest;
	for (int i = 0; i < nflds; i++) {
		itype = ndxtype[i];
		iaddr = fldlst[i]->GetChar();
		*p++ = itype;
		if (itype & STRING) {
			strcpy(p, iaddr);
			p += strlen(iaddr) + 1;
			}
		else if ((itype & MSKASC) == FP) {
			ilen = sizeof(float);
			memcpy(p, iaddr, ilen);
			p += ilen;
			}
		else if ((itype & MSKASC) == DP) {
			ilen = sizeof(double);
			memcpy(p, iaddr, ilen);
			p += ilen;
			}
		else if ((itype & MSKASC) == INT) {
			ilen = sizeof(int);
			memcpy(p, iaddr, ilen);
			p += ilen;
			}
		else
			assert(itype);
		}
	*p++ = '\0';			// terminator byte
	return (p - dest);
	}
//===================================================================
// Assemble a key item or data item from individual fields
int ItemBuild(char *dest, int nflds, RField *fldlst[]) {
	char	*p;
	int		ilen;
	int		itype;
	const char*	iaddr;

	p = dest;
	for (int i = 0; i < nflds; i++) {
		itype = fldlst[i]->GetType();
		iaddr = fldlst[i]->GetChar();
		*p++ = itype;
		if (itype & STRING) {
			strcpy(p, iaddr);
			p += strlen(iaddr) + 1;
			}
		else if (itype == FP) {
			ilen = sizeof(float);
			memcpy(p, iaddr, ilen);
			p += ilen;
			}
		else if (itype == DP) {
			ilen = sizeof(double);
			memcpy(p, iaddr, ilen);
			p += ilen;
			}
		else if (itype == INT) {
			ilen = sizeof(int);
			memcpy(p, iaddr, ilen);
			p += ilen;
			}
		else
			assert(itype);
		}
	*p++ = '\0';			// terminator byte
	return (p - dest);
	}

//===================================================================
// Break down an item into individual fields
int ItemDistribute(char* src, RField *fldlst[]) {
	char	*p;
	int		idb;
	int		ilen;
	int		i;
	char*	iaddr;

	p = src;
	for (i = 0; i < MAXFLD+1; i++) {
		if (fldlst[i] == NULL)
			break;
		idb = *p++;
		if (idb == 0)				// 180309
			break;
		if (idb & STRING) {
			fldlst[i]->SetData(p);
			p += strlen(p) + 1;
			}
		else if (idb == FP) {
			ilen = sizeof(float);
			iaddr =	fldlst[i]->GetDataAddr();
			memcpy(iaddr, p, ilen);
//			fldlst[i]->SetData((float)p);
			p += ilen;
			}
		else if (idb == DP) {
			ilen = sizeof(double);
			iaddr = fldlst[i]->GetDataAddr();
			memcpy(iaddr, p, ilen);
//			fldlst[i]->SetData((double*)p);
			p += ilen;
			}
		else if (idb == INT) {
			ilen = sizeof(int);
			iaddr = fldlst[i]->GetDataAddr();
			memcpy(iaddr, p, ilen);
//			fldlst[i]->SetData((int*)p);
			p += ilen;
			}
		else
			assert(idb);								//180306
		}
	return (i);
	}
//===================================================================
void ItemPrint(char* src) {
	char	*p;
	char	idb;
	int		ilen;
	int		iwork;
	float	fwork;
	double	dwork;

	p = src;
	idb = *p++;
	while (idb) {
		if (idb & STRING) {
			printf("str:%s ", p);
			p += strlen(p) + 1;
			}
		else if ((idb & MSKASC) == FP) {
			iwork = 0;
			ilen = sizeof(float);
			memmove(&fwork, p, ilen);
			printf("fp:%f ", fwork);
			p += ilen;
			}
		else if ((idb & MSKASC) == DP) {
			iwork = 0;
			ilen = sizeof(double);
			memmove(&dwork, p, ilen);
			printf("dp:%f ", dwork);
			p += ilen;
			}
		else if ((idb & MSKASC) == INT) {
			iwork = 0;
			ilen = sizeof(int);
			memmove(&iwork, p, ilen);
			printf("int:%d ", iwork);
			p += ilen;
			}
		else
			break;
		idb = *p++;
		}
	}
//==========================================================
// tolower doesnt exist anymore apparently
int stricpy(char* dest, char* src) {
	int	cnt = 0;
	int sch;
	for (;; dest++, src++) {
		sch = *src;
		if (sch >= 'A' && sch <= 'Z') sch += 'a' - 'A';
		*dest = sch;
		cnt++;
		if (sch = '\0')
			break;
		}
	return cnt;
	}
/*
int ItemLength(const char *item) {

   
	char	idb;
	int		len;
	int		litm;
	
	for (len = 0;;) {
		idb = *item;				// grab type byte
		if (idb == 0)
			return (len+1);
			
		// is this subfield one of the string types?
		if (idb & STRING)
			litm = strlen(item+1) + 1;
		else if (idb & INT)
			litm = idb & INTLEN;
		else
			return (0);
		litm++;					// plus size of type byte
		item += litm;
		len += litm;
		}
	return (0);
	}
	*/
