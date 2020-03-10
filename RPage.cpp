#include <iostream>
#include <string.h>
#include "RPage.hpp"

using	std::cout;
using	std::endl;
using	std::hex;
using	std::dec;

#define	MAX			32767
#define	OSETLEN		0
#define	OSETCNT		1
#define	OSETFRE		2
#define	OSETLWM		3
#define	OSETP		8
#define	memoffset(x)	(short *)gift+x 

//	Heap object management
//		1.  For small heaps (< 64K)
//		2.  Page is built from RHE down,
//		3.  Slots are built from LHE up.
//		4.  Page is addressed as an array of short int's (2-byte) AKA 'word'
//		5.	Content of a slot is the address of the length portion of an item
//				i.e. &page[slotx] => @(slotx's length)
//						slotx = page[slot#]
//						@(slotx data) = &page[slotx - len]
//		6.  Page space is allocated in words.
//		7.  Item length designates the number of bytes requested + 2 length bytes
//				(the byte allocation may be incremented to get to an even allocation).
//		8.  Free'd blocks are denoted by a negative or zero length (always even)
//		9.  The length block is on the RHE of the allocation.
//		10. Memory is provided by user ("gift")
//		11. Initially, all memory is free, there are no used slots and the 
//				free length block encompasses the whole Page.
//		12. pLen, pCount, pFree & pLWM all begin at page space (OSETP)
//		13. len (and variants) are in short words
//				size (and varients) are len + 1 (includes len word itself)
//				ptr's and variants are array subscripts into Page
//				xsize is size + 1 for the associated slot[]
//		x.	This class is built as a compromise on memory usage (for the
//				data portion) and speed with speed usually dominant
//				a) Data movement minimized
//				b) Garbage collection at allocation (not at delete time)
//				c) Uses 1st ready space preferably 
/*
			+------+------+------+-----+
			|pLen  |pCount|pFree |pLWM |
page[0]---->+------+------+------+-----+------+---------------------------------+
			|slot0 |slot1 |slot2 | ... |slotn |									|
			+------+------+------+-----+------+									|
			|<-------------pCount------------>|									|
			|																	|
			|				+- pLWM												|
			|				|													|
			|				v													|
			|				+------------+----+									|
			|				|  data		 |len |. . .							|
			|				+------------+----+									|
			|							 ^										|
			|							/		 |<-------- len  --------->|	|
			|			  page[slotx] -/		 +-------------------------+----+
			|									 | data					   |len |
			+------------------------------------+-------------------------+----+

*/
//==================================================================
RPage::RPage() {
//	*pLWM = *pFree = *pLen = *pCount = 0;
	}
//==================================================================
RPage::RPage(char *gift, const int bytelen) {

	Reset(gift, bytelen);
	*pLWM = *pFree = *pLen = (bytelen - OSETP)>>1;// usable Page in words
	*pCount = 0;
	memset(page, 0, *pLen);  			// TBD non-essential
	}
//==================================================================
//	Allocate space in the page at a specific slot position
//	might want to make new slot at end as an option

int RPage::Allocate(const int slot, const int bytelen) {
	short	size; 								// alloc size + len word
	short	iptr;
	
	if (slot > *pCount)							// slot # illegal, overlaps page memory
		return (-2);
	size = ((bytelen+1)>>1)+1;  				// round to even, CVW
	if ((size+1) > *pFree)	
		return (-1);							// error if slot + size greater than avail
		
	iptr = Locator(size);						// find a place
	Setup(iptr, size);							// prepare space
	
	// insert new slot by moving slots to the right up a peg
	if (slot < *pCount) 						// scootchie up
		memmove(&page[slot+1], &page[slot], (*pCount-slot) * sizeof(short));

	page[slot] = iptr;
	(*pFree)--;
	(*pCount)++;
	return (slot);
	}
//==================================================================
// return TRUE if there is enough free space to for a new item of 'bytelen' bytes long

int RPage::BigEnough(const int bytelen) {

	int xsize = ((bytelen+1)>>1)+2;				// round up to even, add len word and slot
	if (xsize < *pFree)
		return 1;
	return 0;
	}
//==================================================================
// delete an item and its slot, scootchie down slots on right side (if any)

int RPage::Delete(const int slot) { 
	return (Delete(slot, slot));
	}
//==================================================================
// delete a series of item and slots, scootchie down slots on right side
int RPage::Delete(const int slot1, const int slot2) { 
	short	len;
	short	iptr;
	int		slotmove;
	
	if (slot2 > *pCount || slot1 > slot2)
		return (-1);
	slotmove = *pCount - slot2 - 1;				// zero if slot2 is at RHE
	for (int i = slot1; i <= slot2; i++) {
		iptr = page[i];  
		len = page[iptr];
		page[iptr] = -len;						// mark as free
		*pFree += (len + 2);					// reclaim free'd size + slot
		(*pCount)--;  
		}
	if (slotmove) 				// scootchie down unless slot2 is RHE
		memmove(&page[slot1], &page[slot2+1], slotmove * sizeof(short));
	return (0);
	}
//==================================================================
// Return the address of the data specified by a slot number
//	return zero if error
// ??? maybe we should copy item to user's memory and not give our address away

char* RPage::GetItem(const int slot) {

	short	iptr;
	int		len;
	
	if (slot >= *pCount || slot < 0)			// if slot out of bounds
		return 0;
	iptr = page[slot];
	len = page[iptr];							// size of item
	if (len <= 0)								// if slot points to free'ed block
		return 0;
	return ((char *)&page[iptr - len]);
	} 
//==================================================================
// Return the length of a data item (in bytes)

int RPage::GetLen(const int slot) {
	int		iptr;

	if (slot >= *pCount || slot < 0)			// if slot out of bounds
		return 0;
	iptr = page[slot];
	if (page[iptr] <= 0)						// oops, this is free'd space
		return 0;
	return (page[iptr]<<1);
	} 
//================================================================== 
void RPage::Initialize(char	*gift, const int bytelen) {

	Reset(gift, bytelen);
	*pLWM = *pFree = *pLen = (bytelen - OSETP)>>1;	// usable Page in words
	*pCount = 0;
	memset(page, 0, *pLen);  					// TBD non-essential
	}
//==================================================================
//	Insert new data

// insert into an existing item at the specified slot

int RPage::Insert(const char *item, const int bytelen, const int slot, const int offset) {
	short	iptr;
	int		len;
	char	*p;
	
	if (slot >= *pCount)						// if slot out of bounds
		return 0;
	iptr = page[slot];
	len = page[iptr];
	if (bytelen+offset > (len<<1))
		return 0;
	p = (char *)&page[iptr - len];
	memcpy(p+offset, item, bytelen);
	return 1;
	}

//==================================================================
//	Return an address (page[n]) that can accomodate a new data item
//		address is the prospective length word of the new item, 'len' bytes below are avail
//		pLWM is altered if garbage collection occurs

//	Strategy:
//		1. Use virgin memory (below pLWM) if its big enough
//		2. Try to use 1st avail free frag
//		3. Compact entire Page then use the virgin (newly ordained) area

int RPage::Locator(const int size) {
	short	iptr1, iptr2;
	short	dest;
	short	len1, len2;				// item length
	short	size1, size2;			// item length + length word
	short	xsize;					// item length + length word + slot
	short	innerspace;				// space between slot area and LWM

	innerspace = *pLWM - *pCount;
	xsize = size + 1;							// xsize includes a new slot
	// if the zone between slots and data is big enough, use it now
	if (xsize <= innerspace) {
		dest = *pLWM-1;
		return (dest);
		}
	
	// scan Page for free memory
	
	// this loop looks for the first free frag
	for (iptr1 = *pLen-1; iptr1 > *pLWM; iptr1 -= (len1+1)) {
		len1 = page[iptr1];
		if (len1 <= 0)
			break;								// break out if ptr1 -> free space
		}
	// iptr not necessarily pointing to free item
	// try to use first free frag, starting at page RHE
	len1 = -len1;
	size1 = len1 + 1;							// negate freeness
	// if size big enough, were outta here
	if ((xsize <= size1 || (size <= size1 && innerspace > 0)) && *pCount < *pLWM) {
		return (iptr1);
		}
			  
	// fragment is too small, do full garbage compaction
	for (iptr2 = iptr1 - (len1+1); iptr2 >= (*pLWM-1);) {

		if (iptr2 == (*pLWM-1)) {				// if we found the LWM
			*pLWM = iptr1+1;					// incorporate it
			break;
			}

		len2 = page[iptr2];
/*		
 +----------+-----+----+--------+----+---------------
 |			|xxxxx|len2|yyyyyyyy|len1|  
 +----------+-----+----+--------+----+---------------
				  ^				^
				  ptr2			ptr1
*/
		if (len2 <= 0) {  						// if this frag is free, combine
			size2 = -len2 + 1;					// strip avail bit
			size1 += size2;		  				// combine length
			page[iptr1] = -size1 - 1;  			// make first frag bigger
			iptr2 -= size2;
			if (iptr2+1 == *pLWM)
				*pLWM = iptr1+1;
/*		
 +------+---+-------------------+-----+--------------
 |		|	|xxxxx......yyyyyyyy|len1'|  
 +------+---+-------------------+-----+--------------
		^ 						^
		ptr2 					ptr1
*/
			}
				
//		this item is not free, swap with free frag
		else {
			// scan for this address in slots and adjust
			for (int i = 0; i < *pCount; i++) {
				if (page[i] == iptr2) {
					page[i] = iptr1;
					break;
					}
				}
			page[iptr1] = len2;
			size2 = len2 + 1;
			memmove(&page[iptr1-len2], &page[iptr2-len2], len2<<1);
			if (iptr2-size2 == *pLWM) {
				*pLWM = iptr1 - size2; 			// redo LWM if moved up
				break;
				}
			iptr1 -= size2;				// point to moved free frag
			iptr2 -= size2;
/*		
 +------+---+--------+----+-----+----+---------------
 |		|	|........|len1|xxxxx|len |  
 +------+---+--------+----+-----+----+---------------
		^			 ^
		ptr2		 ptr1
*/				
			}
		}
					
	innerspace = *pLWM - *pCount;
	if (xsize <= innerspace) {					// if combined frag is big enough?
		dest = *pLWM-1;
		return (dest);
		}
	return (-1);// error return
	}
//==================================================================
//	All new data structure for this object

// TBD - length not used
void RPage::Reset(char *gift, const int len) {

	pLen	= memoffset(OSETLEN);
	pCount	= memoffset(OSETCNT);
	pFree	= memoffset(OSETFRE);
	pLWM	= memoffset(OSETLWM); 
	page	= (short *)(gift + OSETP);
	}
//==================================================================
// Setup a space
// a free fragment is modified if space is left over
 
void RPage::Setup(const int iptr, const int size) {
	int		osize;// old frag length
	short	lptr;
 
	if (iptr > *pLWM)  							// are we reusing a frag?
		osize = page[iptr];						// save old frag len if not adding to LWM

	lptr = iptr - size;
	page[iptr] = size - 1;						// set len to # of shorts
	*pFree -= size;
	
	if (iptr > *pLWM) {							// take care of fragment
		osize += size;							// reduce osize (N.B. osize is negative)
		if (osize <= 0)
			page[lptr] = osize;
		}
	else
		*pLWM = lptr+1; 						// LWM reached a new low?
	} 
//==================================================================
void RPage::Dump() {

	unsigned int	ch;
	int		len;
	short	iptr;
	short	dlen;
	char	*ptr;
	
	cout << "Page free space = " << *pFree * 2;
	cout << " slots = " << *pCount << " LWM = " << (*pLWM * 2) << endl;
	for (int i = 0; i < *pCount; i++) {
		iptr = page[i];
		if (iptr < *pCount || iptr == MAX)
			continue;
		cout << " slot # " << i << " ";
		len = page[iptr];
		dlen = (len + 1) >> 1;
		cout << " len = " << len << " | ";
		ptr = (char *)&page[iptr-dlen]; 
		for (int j = 0; j < len; j++) {
			ch = *ptr++;
			cout << hex << ch << ' ';
			} 
		cout << dec << endl;
		}
	}