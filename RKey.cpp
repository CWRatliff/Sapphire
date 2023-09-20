// 210707 - Compare greatly revised, KeyCompare(s) order of calling Compare reversed
//			they were calling search key, then index key
// 210710 - Added function GetKeyBody that strips recno

#include "OS.h"
#include <memory.h>
#include <stdarg.h> 
#include <stdlib.h>
#include <stdio.h>
#include "RField.h"
#include <string.h>
#include "RKey.hpp" 
#include "Ndbdefs.h"
#include "utility.h"
#include "dbdef.h"

//    keyStr
//    |
//    v
//   +----+------+----+------+-----------+---+
//   | t1 | k1   | t2 | k2   | ...       | 0 | 
//   +----+------+----+------+-----------+---+
//   <--------------- keyLen ---------------->

// key composition:
//		up to 5 subfields                             
//		max key length 256 bytes
//		each subfield is preceeded by a descriptor byte (kcb)
//		1xxx xxxx	- descending order
//		x010 0000	- alphanumeric string (zero terminated)
//		x110 0000	- alpha string uncased
//		x000 0100	- numeric string
//		x000 0001	- integer
//		x000 0010	- floating point
//		x000 0011	- double precision

//		0000 0000	- terminator
//==================================================================*/
//	construct a search key

RKey::RKey() {
	keyStr = new char[KEYMAX];
	keyLen = 0;
	}

//====================================================================
RKey::~RKey() {
	if (keyStr)
		delete [] keyStr;
	}
//==================================================================
// return the key without recno
void RKey::GetKeyBody(const RKey &orig) {

	memcpy(keyStr, orig.keyStr, orig.keyLen - 5);
	keyLen = orig.keyLen - 5;
	keyStr[keyLen-1] = '\0';
	}
//==================================================================
// return the first key component
int RKey::GetKeyHead() {
	int		component;

	memcpy(&component, keyStr + 1, sizeof(int));
	return (component);
	}
//==================================================================
// return the last key component
int RKey::GetKeyTail() {
	int		component;

	memcpy(&component, keyStr + keyLen - 5, sizeof(int));
	return (component);
	}
//==================================================================
// append a key to the existing key object
int	RKey::KeyAppend(RKey &otherKey) {
	memcpy(keyStr + keyLen - 1, otherKey.keyStr, otherKey.keyLen);
	keyLen += otherKey.keyLen - 1;
	return 0;
	}
//==================================================================
// RKey :: supplied key (otherkey)
int RKey::KeyCompare(RKey &otherKey) {
	return (Compare(keyStr, otherKey.keyStr));
}
//==================================================================
// keystring :: RKey
int RKey::KeyCompare(const char *keyString) {
	return (Compare(keyString, keyStr));
}

//==================================================================
int RKey::Compare(const char *ikey, const char *tkey) {

// compare a key from the index (ikey) with a test key (tkey)
// return	-1 if ikey < tkey
//			0  if keys are equal
//			+1 if ikey > tkey   

//N.B. index secondary key will always have one more subkey,
//		the recno subfield, only test key will terminate cleanly
// return -3 if two keys are mismatched for ndxno
// TBD numeric data should be compared after converting to FP

	char	idb;									// descriptor byte
	char	tdb;									// test desc byte
	int		ilen;									// index length
	int		tlen;									// test length
	int		len;
	int		rc;										// ret code
	int		rcs;
	int		iint, tint;
	float	ifp, tfp;
	double	idp, tdp;

	// ndxno key subfields must be equal
	idb = *ikey++;
	tdb = *tkey++;
	if (idb != 1 || tdb != 1)
		return -3;
	iint = *((int *)ikey);
	tint = *((int *)tkey);
	if (iint < tint)
		return (-1);
	if (iint > tint)
		return (1);
	ikey += sizeof(int);
	tkey += sizeof(int);

	for (rc = 0; rc == 0;) {					// while subkeys are equal
		idb = *ikey++;							// get next descriptor
		tdb = *tkey++;

		if (tdb == 0)							// end of search key
			break;

		// is this subfield one of the string types?
		if (idb & STRING) {
			ilen = strlen(ikey);
			tlen = strlen(tkey);

			len = ilen;							// min length
			if (ilen > tlen)
				len = tlen;

			if (idb & MSKNOCASE || tdb & MSKNOCASE)
				rcs = strnicmp(ikey, tkey, len);
			else
				rcs = memcmp(ikey, tkey, len);

			if (rcs == 0) {
				if (ilen < tlen)				// if strings equal but length unequal
					rc = -1;					// if ikey smaller tkey, ikey is smaller
				if (ilen > tlen)
					rc = 1;						// tkey smaller
				}
			else
				rc = rcs;

			ilen++;								// step over '\0'
			tlen++;
			}
		else if ((idb & MSKASC) == FP) {
			ilen = tlen = sizeof(float);
			ifp = *((float*)ikey);
			tfp = *((float*)tkey);
			if (ifp < tfp)
				rc = -1;
			else if (ifp > tfp)
				rc = 1;

			}
		else if ((idb & MSKASC)  == DP) {
			ilen = tlen = sizeof(double);
			idp = *((double*)ikey);
			tdp = *((double*)tkey);
			if (idp < tdp)
				rc = -1;
			else if (idp > tdp)
				rc = 1;
			}
		else if ((idb & MSKASC)  == INT) {
			ilen = tlen = sizeof(int);
			iint = *((int*)ikey);
			tint = *((int*)tkey);
			if (iint < tint)
				rc = -1;
			else if (iint > tint)
				rc = 1;
			}
		ikey += ilen;						// advance
		tkey += tlen;
		if (idb & MSKDESC || tdb & MSKDESC)
			rc = rc * -1;					// flip return code if descending key
		} 

	return (rc);
	}
//==================================================================

int RKey::KeyLength(const char *key) {

   
	char	idb;
	int		len;
	int		lkey;
	
	for (len = 0;;) {
		idb = *key;				// grab type byte
		if (idb == 0)
			return (len+1);
			
		// is this subfield one of the string types?
		if (idb & STRING)
			lkey = strlen(key+1) + 1;
		else if ((idb & MSKASC) == FP)
			lkey = sizeof(float);
		else if ((idb & MSKASC) == DP)
			lkey = sizeof(double);
		else if ((idb & MSKASC) == INT)
			lkey = sizeof(int);
		else
			return (0);
		lkey++;					// plus size of type byte
		key += lkey;
		len += lkey;
		}
	return (0);
	}
//==================================================================
// set key object to given key 

int RKey::SetKey(const char *str) {
	if (str != NULL) {
		keyLen = KeyLength(str);
		memcpy(keyStr, str, keyLen);
		}
	return 0;
	}
//=================================================================
// recompute keylen
// int RKey::SetKeyLen() {
//	keyLen = KeyLength(keyStr);
//	return (keylen);
//	}
//==================================================================
// template string:
//		s => string, 63 char max (arbitrary)
//		i => integer
//		f =-> float
//		d => double float

int RKey::MakeSearchKey(const char *tmplte, ...) { 
                   
	va_list arg;
	int		descmask = 0;
	int		len = 0;
	int		type;
	int		idata;
	float	fdata;
	double	ddata;
	char*	data;
	char	temp[KEYMAX];
	char	*key = &temp[0];
 
	keyLen = 0;
	data = (char *)"";
	len = 0;
 	va_start(arg, tmplte);
	while (*tmplte) {
		type = *tmplte++;
		if (type == '-')
			descmask = MSKDESC;
		else {
			if (type == 's' || type == 'u' || type == 'n') {
				data = (char *)va_arg(arg, char *);
				len = strlen(data) + 1;
				if (len > 63) {
					return (-1);				// error, too long
					}
				if (type == 's')
					*key = STRING;
				else if (type == 'u')
					*key = STRING | MSKNOCASE;
				else
					*key = STRNUMERIC;
				}
			else if (type == 'f') {				// float
				fdata = (float)va_arg(arg, double);
				data = (char *)&fdata;
				len = sizeof(float);
				*key = FP;
				}
			else if (type == 'd') {				// double
				ddata = va_arg(arg, double);
				data = (char *)&ddata;
				len = sizeof(double);
				*key = DP;
				}
			else if (type == 'i') {				// integer
				idata = va_arg(arg, int);
				data = (char *)&idata;
				len = sizeof(int);
				*key = INT;
				}

			*key = *key | descmask;				// descending?
			descmask = 0;
			key++;
			if (data != NULL && len > 0)
				memcpy(key, data, len);
			key += len;
			keyLen += len + 1;
			}
		}
	va_end(arg);
	
	*key = 0;									// key terminator
	keyLen++;
//	keyStr = new char[KEYMAX]; // 161217 memory leak?
	memcpy(keyStr, temp, keyLen);
	return 0;
	}
//==================================================================

void RKey::operator=(const RKey &other) {
	keyLen = other.keyLen;
//	keyStr = new char[KEYMAX];
	memcpy(keyStr, other.keyStr, keyLen);
	}
	
//==================================================================
RKey& RKey::operator=(const char* item) { 
	keyLen = KeyLength(item);
//	keyStr = new char[KEYMAX];
	memcpy(keyStr, item, keyLen);
	return *this;
	}
	
//==================================================================

void RKey::PrintKey() {
	
	ItemPrint(keyStr);
	}
