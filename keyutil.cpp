#include <stdarg.h> 
#include <string.h>
#include "dbdef.h"
//==================================================================
// N.B. mostly duplicate code as class RKey - duplicated to avoid need for RDbase user
//	to need to include class RKey
// template string:
//		s => string, 63 char max (arbitrary)
//		u => caseless string
//		n => numeric string
//		f =-> float
//		d => double float
//		i => integer

int MakeSearchKey(char* key, const char *tmplte, ...) { 
                   
	va_list arg;
	int		descmask = 0;
	int		keylen = 0;
	int		len;
	int		type;
	int		idata;
	float	fdata;
	double	ddata;
	char	*data;
 
 	va_start(arg, tmplte);
	while (*tmplte) {
		type = *tmplte++;
		if (type == 's' || type == 'u' || type == 'n') {
			data = (char *)va_arg(arg, char *);
			len = strlen(data) + 1;
			if (len > 63) {
				return (-1);				// error, too long
				}
			if (type == 's')
				*key = STRING;
			else if (type == 'u')
				*key = STRNOCASE;
			else
				*key = STRNUMERIC;
			}
		else if (type == 'f') {				// floating point
			fdata = (float)va_arg(arg, double);
			data = (char *)&fdata;
			len = sizeof(float);
			*key = FP;
			}
		else if (type == 'd') {				// double precision
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
		else
			return 0;

		*key = *key | descmask;				// descending?
		descmask = 0;
		key++;
		memcpy(key, data, len);
		key += len;
			keylen += len + 1;
		}
	va_end(arg);
	
	*key = 0;								// key terminator
	keylen++;
	return keylen;
	}
//==========================================================================
int KeyLength(const char *key) {

   
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
