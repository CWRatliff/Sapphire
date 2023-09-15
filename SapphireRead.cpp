// SapphireRead.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Sapphire.h"

#define MSKNOCASE	0x40
#define MSKDESC		0x80
#define MSKASC		0x7f
#define ASCENDING	0
#define INT			1
#define FP			2
#define	DP			3
#define STRNUMERIC	4
#define	STRING		0x20
#define STRNOCASE	0x60

RTable* sysrel;
RTable* sysatr;
RTable* sysndx;

int main() {
	Sapphire	dbase;		// "The" database object
	RDbf* dbf;
	RTable* table;
	RIndex* ndx;
	RField* field;
	RField* fldt;
	RField* fldf;

	RField* fldFldName;
	RField* fldTabName;
	RField* fldFldType;
	RField* fldFldLen;

	char const* fldname;
	char const* tabname;
	char const* relname;

	int	fldtype;
	int fldlen;
	int i;
	int rc;
	int trc;
	const char* ctype;

	const char* datatype(int);
	void indexfields();

    std::cout << "Sapphire Database Summary\n";
	dbf = dbase.DbLogin("robodatabase");
	if (dbf == NULL) {
		std::cout << "Cant open database\n";
		exit(0);
		}

	std::cout << "Opened: " << dbf << "\n";
	//dbf->PrintTree();
	//table = dbf->DbOpenTable("table1");
	//std::cout << "table object:   " << table << "\n";

	sysrel = dbf->DbOpenTable("sysrel");
	sysatr = dbf->DbOpenTable("sysatr");
	sysndx = dbf->DbOpenTable("sysndx");
	trc = sysrel->DbFirstRecord(0);
	for (; trc >= 0;) {
		field = sysrel->DbGetFieldObject("relname");
		relname = field->GetCharPtr();
		std::cout << "Table: " << relname << "\n";
		// now get the field names for this table
		rc = sysatr->DbFirstRecord(0);
		for (; rc >= 0;) {
			fldt = sysatr->DbGetFieldObject("relname");
			fldf = sysatr->DbGetFieldObject("fldname");
			fldFldType = sysatr->DbGetFieldObject("fldtype");
			fldFldLen = sysatr->DbGetFieldObject("fldlen");
			tabname = fldt->GetCharPtr();
			if (strcmp(relname, tabname) == 0) { // only fields matching relname
				fldname = fldf->GetCharPtr();
				fldtype = fldFldType->GetInt();
				ctype = datatype(fldtype);
				fldlen = fldFldLen->GetInt();
				std::cout << "  Field: " << fldname << " type: " << ctype << " len: " << fldlen << "\n";
				}
			rc = sysatr->DbNextRecord(0);
			}
		// now get the index names for this table
		rc = sysndx->DbFirstRecord(0);
		for (; rc >= 0;) {
			fldt = sysndx->DbGetFieldObject("relname");
			fldf = sysndx->DbGetFieldObject("ndxname");
			tabname = fldt->GetCharPtr();
			if (strcmp(relname, tabname) == 0) { // only fields matching relname
				fldname = fldf->GetCharPtr();
				std::cout << "  Index: " << fldname << "\n     ";
				indexfields();
				std::cout << "\n";
			}
			rc = sysndx->DbNextRecord(0);
		}
		// count the number of data records in this table
		table = dbf->DbOpenTable(relname);
		if (table != NULL) {
			int count = 0;

			rc = table->DbFirstRecord(NULL);
			for (; rc >= 0;) {
				count++;
				rc = table->DbNextRecord(0);
				}
			std::cout << "  Record count = " << count << "\n\n";
			}
		dbf->DbCloseTable(table);
		trc = sysrel->DbNextRecord(0);
		}
	}
//================================================================================
void indexfields() {
	char	adfldname[50 + 1 + 4];
	const char* tmplte;
	const char* fldname;
	const char* tabname;
	RField* fldt;
	RField* fldad;
	RField* fldf1;
	RField* fldf2;
	RField* fldf3;
	RField* fldf4;
	RField* fldf5;

	fldad = sysndx->DbGetFieldObject("fldasdec");
	fldf1 = sysndx->DbGetFieldObject("fldname1");
	fldf2 = sysndx->DbGetFieldObject("fldname2");
	fldf3 = sysndx->DbGetFieldObject("fldname3");
	fldf4 = sysndx->DbGetFieldObject("fldname4");
	fldf5 = sysndx->DbGetFieldObject("fldname5");

	tmplte = fldad->GetCharPtr();
	fldname = fldf1->GetCharPtr();
	strcpy_s(adfldname, "(x) ");
	adfldname[1] = tmplte[0];
	strcat_s(&adfldname[4], 50, fldname);
	std::cout << " " << adfldname;

	fldname = fldf2->GetCharPtr();
	if (strlen(fldname) == 0)
		return;
	strcpy_s(adfldname, "(x) ");
	adfldname[1] = tmplte[1];
	strcat_s(&adfldname[4], 50, fldname);
	std::cout << " " << adfldname;

	fldname = fldf3->GetCharPtr();
	if (strlen(fldname) == 0)
		return;
	strcpy_s(adfldname, "(x) ");
	adfldname[1] = tmplte[2];
	strcat_s(&adfldname[4], 50, fldname);
	std::cout << " " << adfldname;

	fldname = fldf4->GetCharPtr();
	if (strlen(fldname) == 0)
		return;
	strcpy_s(adfldname, "(x) ");
	adfldname[1] = tmplte[3];
	strcat_s(&adfldname[4], 50, adfldname);
	std::cout << " " << adfldname;

	fldname = fldf5->GetCharPtr();
	if (strlen(fldname) == 0)
		return;
	strcpy_s(adfldname, "(x) ");
	adfldname[1] = tmplte[4];
	strcat_s(&adfldname[4], 50, fldname);
	std::cout << " " << adfldname;

	}

const char* datatype(int type) {
	if (type & STRING)
		return ("str");
	else if ((type & MSKASC) == FP)
		return ("fp");
	else if ((type & MSKASC) == DP)
		return("dp");
	else if ((type & MSKASC) == INT)
		return("int");
	else
		return ("");
	}
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
