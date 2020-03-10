#if !defined(_NDBDEFS_H_)

#define _NDBDEFS_H_

#define	TRUE			1
#define	FALSE			0

// System relations, fields, indexes index number
#define	SYSREL			1
#define	SYSRELX			2
#define	SYSFLD			3
#define	SYSFLDX			4
#define	SYSNDX			5
#define SYSNDXX			6
#define USERNDX			100

// relation record status (was any field altered?)
#define	VIRGIN			0
#define	CHANGED			1

// key position status
#define	IEOF			2
#define	ONKEY			1
#define	BETWEEN			0
#define	UNPOSITIONED	-1

#define	NODE			int
#define	LNODE			sizeof(NODE) 
#define	DATAADR			int

#define	KEYMAX			256
#define	FLDMAX			256
#define DATAMAX			1000
#define MAXFLD			100			// max of 100 fields per relation
#define MAXNDX			100			// max of 100 indexes per relation
#define MAXKEY			5			// 5 items in key
#define MAXNAME			32			// sise of table, field, or index name

// ndx_ID packet, to allow concurrent indexes
typedef	struct	ndxID_str {
	int		ndxKeyNo;				// key in page
	NODE	ndxNode;				// node number
	int		ndxStatus;				// status bits
	RKey	ndxKey;					// NB. ndxno present
	} NDX_ID;

#endif
