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
//		?up to 5 subfields                             
//		max key length 256 bytes
//		each subfield is prceeded by a descriptor byte (kcb)
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
// return the first key component
int RKey::GetKeyHead() {
	int		component;

	memcpy(&component, keyStr+1, sizeof(int));
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
int RKey::KeyCompare(RKey &otherKey) {
	return (Compare(keyStr, otherKey.keyStr));
	}
//==================================================================
int RKey::KeyCompare(char *keyString) {
	return (Compare(keyStr, keyString));
	}	

//==================================================================
int RKey::Compare(const char *ikey, const char *tkey) {

// compare a key from the index (ikey) with a test key (tkey)
// return	-1 if ikey < tkey
//			0  if keys are equal
//			+1 if ikey > tkey   
	
// if ikey == tkey but ikey has additional field(s) ikey > tkey, rc = 2
// if ikey == tkey but tkey has additional field(s) ikey < tkey, rc = -2

// TBD numeric data should be compared after converting to FP

	char	idb;									// descriptor byte
	char	tdb;									// test desc byte
	int		ilen;									// index length
	int		tlen;									// test length
	int		len;
	int		rc;										// ret code
	int		iint, tint;
	float	ifp, tfp;
	double	idp, tdp;

	for (rc = 0; rc == 0;) {
		idb = *ikey++;								// get next descriptor
		tdb = *tkey++;
		if ((idb == 0) && (tdb == 0))
			return (0);
		if (tdb == 0)
			return (2);
		if (idb == 0)
			return (-2);
			
		// is this subfield one of the string types?
		if (idb & STRING) {
			ilen = strlen(ikey);
			tlen = strlen(tkey);
//			len = _min(ilen, tlen); // MSVS
			len = ilen;
			if (tlen < ilen)
				len = tlen;
			if (idb & MSKNOCASE || tdb & MSKNOCASE)
				rc = strncasecmp(ikey, tkey, len);
			else
				rc = memcmp(ikey, tkey, len);
			if (rc == 0) {
				if (ilen < tlen)				// if strings equal but length unequal
					rc = -1;					// if ikey smaller tkey, ikey is smaller
				if (ilen > tlen)
					rc = 1;						// tkey smaller
				}
			ilen++;
			tlen++;
			}
		else if ((idb & MSKASC) == FP) {
			ifp = tfp = 0;
			ilen = sizeof(float);
			tlen = sizeof(float);
			ifp = *((float*)ikey);
			tfp = *((float*)tkey);
			rc = 0;
			if (ifp < tfp)
				rc = -1;
			else if (ifp > tfp)
				rc = 1;
			}
		else if ((idb & MSKASC)  == DP) {
			idp = tdp = 0;
			ilen = sizeof(double);
			tlen = sizeof(double);
			idp = *((double*)ikey);
			tdp = *((double*)tkey);
			rc = 0;
			if (idp < tdp)
				rc = -1;
			else if (idp > tdp)
				rc = 1;
			}
		else if ((idb & MSKASC)  == INT) {
			iint = tint = 0;
			ilen = sizeof(int);
			tlen = sizeof(int);
			iint = *((int*)ikey);
			tint = *((int*)tkey);
			rc = 0;
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
	int		len;
	int		type;
	int		idata;
	char	*data;
	char	temp[KEYMAX];
	char	*key = &temp[0];
 
	keyLen = 0;
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
//				idata = (int)va_arg(arg, float); //unix no likey
				idata = (int)va_arg(arg, double);
				data = (char *)&idata;
				len = sizeof(float);
				*key = FP;
				}
			else if (type == 'd') {				// double
				idata = (int)va_arg(arg, double);
				data = (char *)&idata;
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
RKey& RKey::operator=(char* item) { 
	keyLen = KeyLength(item);
//	keyStr = new char[KEYMAX];
	memcpy(keyStr, item, keyLen);
	return *this;
	}
	
//==================================================================

void RKey::PrintKey() {
	
	ItemPrint(keyStr);
	}
