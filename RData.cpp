#include <memory.h>
#include <string.h>
#include <stdio.h>
#include "RField.h"
#include "RKey.hpp"
#include "RData.hpp"
#include "Ndbdefs.h"
#include "utility.h"
#include "dbdef.h"
//    datStr
//    |
//    v
//   +----+------+----+------+-----------+---+
//   | t1 | d1   | t2 | d2   | ...       | 0 | 
//   +----+------+----+------+-----------+---+
//   <--------------- datLen ---------------->

// Data types
//		x100 0000	- alphanumeric string (zero terminated)
//		x110 0000	- alpha string uncased
//		x010 0000	- numeric string
//		x001 0000	- integer
//		x000 1000	- floating point
//		x000 0100	- double precision fp

//		0000 0000	- terminator
//==================================================================
RData::RData() {
	datStr = new char[DATAMAX];
	datLen = 0;
	}
// memcpy is used for int - no guarantee of word alignment
//==================================================================
/*
	create an RData object using "item"
	item is a mixed string of datatypes and data
*/
RData::RData(char* item) {
	datStr = new char[DATAMAX];
	SetData(item);
	}

// memcpy is used for int, long, et. al. - no guarantee of word alignment
//==================================================================
RData::RData(char *item, int dlen) {
/*
	create an RData object using a given data stream "item"
	whose total length is known
*/
	datStr = new char[DATAMAX];
	memcpy(datStr, item, dlen);
	datLen = dlen;
	}
//==================================================================
RData::~RData() {
	if (datStr)
		delete [] datStr;
	}
//==================================================================
int RData::SetData(char* str) {
	char	*p;
	int		idb;
	int		ilen;
	int		itemlen = 0;

	p = str;
	while (*p) {
		itemlen++;
		idb = *p++;
		if (idb & STRING) {
			ilen = strlen(p) + 1;
			itemlen += ilen;
			p += ilen;
			}
		else if (idb == FP) {
			ilen = sizeof(float);
			itemlen += ilen;
			p += ilen;
			}
		else if (idb == DP) {
			ilen = sizeof(double);
			itemlen += ilen;
			p += ilen;
			}
		else if (idb == INT) {
			ilen = sizeof(int);
			itemlen += ilen;
			p += ilen;
			}
		}
	datLen = itemlen + 1;
	memmove(datStr, str, datLen);
	p = datStr + itemlen;
	*p = '\0';				// terminator

	return (datLen);
	}
//===================================================================
void RData::PrintData() {

	ItemPrint(datStr);
	}
