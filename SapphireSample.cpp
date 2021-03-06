// SapphireSample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


//int main()
//{
//    return 0;
//}

// dbexample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// this group used by _open
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>
#include <string.h>

#include "Sapphire.h"

int _tmain(int argc, _TCHAR* argv[]) {


	// arrays of data to be put into example database
	const char	*array[] = { "alphabet",
		"betamax",
		"aaaaa",
		"gamma-ray",
		"physical",
		"deltawing",
		"budda",
		"alephnull",
		"boathook",
		"chicago",
		"dogged",
		"mainsail",
		"beatnick",
		"cloud",
		"coronet",
		"cement",
		"" };
	const char *array2[] = {
		"cement",
		"normal",
		"barium",
		"algebra",
		"dramatic",
		"fractal",
		"spinnaker",
		"order",
		"pate",
		"adjective",
		"" };
	const char	*array3[] = { "Kermit",
		"Bond",
		"Bolero",
		"George",
		"Miss Piggy",
		"Ludwig",
		"Amadeus",
		"Voyager",
		"Liberty",
		"Camarillo",
		"Rainbow",
		"Dell",
		"Beatle",
		"McCord",
		"Bugle",
		"Rockport",
		"" };

	int		i, j;
	int		ifld;
	int		rc = 0;
	char	item[30];
	char	item2[30];
	char	key[20];		// key assembly area
	const char*p;

							// These RField objects are used to make a new table
	RField f1("field1", STRING, 20);
	RField f2("field2", INT, 4);
	RField f3("field3", STRING, 20);
	RField* flst[] = { &f1, &f2, &f3, NULL };

	// several RField arrays for making indexes
	RField* xlst[] = { &f1, NULL };
	RField* xlst2[] = { &f2, &f3, NULL };
	RField* xlst3[2];

	xlst3[0] = &f1;			// alternate way to populate RField lists
	xlst3[1] = NULL;

	Sapphire	dbase;		// "The" database object
	RDbf*		dbfile;
	RTable*		table;
	RIndex*		ndx0;
	RIndex*		ndx1;
	RIndex*		ndx2;
	RIndex*		ndx3;

	_unlink("tstdbf.dbf");	// delete old copy if any
	dbfile = dbase.DbLogin("tstdbf");
	if (dbfile == NULL)
	dbfile = dbase.DbCreateFile("tstdbf");

	// make a new relation, open it and add indexes and data
	rc = dbfile->DbMakeTable("table1", flst);
	table = dbfile->DbOpenTable("table1");
	dbfile->PrintTree();
	ndx0 = table->DbGetIndexObject("");
	ndx1 = table->DbGetIndexObject("namendx");
	if (ndx1 == NULL)
		ndx1 = table->DbMakeIndex("namendx", "a", xlst);
	dbfile->PrintTree();
	ndx2 = table->DbMakeIndex("namendx", "aa", xlst);
	for (i = 0;; i++, rc++) {
		if (strlen(array[i]) == 0)
			break;
		table->DbSetField("field1", array[i]);
		table->DbSetField("field2", i);
		table->DbSetField("field3", array3[i]);
		table->DbAddRecord();
	}
	dbfile->PrintTree();

	// create a new index for existing table
	ndx2 = table->DbGetIndexObject("postindex");
	if (ndx2 == NULL)
		ndx2 = table->DbMakeIndex("postindex", "aa", xlst2);
	dbfile->PrintTree();

	// MakeSearchKey and Search
	MakeSearchKey(key, "s", "physical");
	rc = table->DbSearchRecord(ndx1, key);
	if (rc != 0)
		printf("Search failure\n");

	// example of offset usage

	p = table->DbGetChar("field3");
	strcpy_s(item, p);
	p = table->DbGetChar("field1", 2);			// alternative using 'subscripted' field
	strcpy_s(item2, p);
	printf("Both fetched items should be the same %s & %s\n", item, item2);

	// delete a record, all indexes should reflect deleted record
	table->DbDeleteRecord();
	dbfile->PrintTree();
	printf("Record 5 physical, Miss Piggy should be deleted\n");

	// update example
	MakeSearchKey(key, "s", "chicago");
	table->DbSearchRecord(ndx1, key);
	p = item;
	p = table->DbGetChar("field3");
	printf("Fetched data %s\n", item);
	table->DbSetField("field3", "denver");
	table->DbUpdateRecord();
	dbfile->PrintTree();
	printf("Camarillo should be replaced with denver\n");

	// examples of DbFirstRecord, Next, Last, Prev
	table->DbFirstRecord(ndx1);
	p = table->DbGetChar("field3");
	strcpy_s(item, p);
	printf("DbFirstRecord - (Bolero) Fetched data %s\n", item);

	table->DbNextRecord(ndx1);
	p = table->DbGetChar("field3");
	strcpy_s(item, p);
	printf("Next - (Voyager) Fetched data %s\n", item);

	table->DbLastRecord(ndx1);
	p = table->DbGetChar("field3");
	strcpy_s(item, p);
	printf("Last - (Dell) Fetched data %s\n", item);

	table->DbPrevRecord(ndx1);
	p = table->DbGetChar("field3");
	strcpy_s(item, p);
	printf("Prev - (George) Fetched data %s\n", item);

	table->DbNextRecord(ndx1);
	p = table->DbGetChar("field3");
	strcpy_s(item, p);
	printf("Next - (Dell) Fetched data %s\n", item);

	i = table->DbNextRecord(ndx1);
	if (i >= 0) {
		p = table->DbGetChar("field3");
		strcpy_s(item, p);
		printf("Next at eof SHOULD NOT PRINT %s\n", item);
	}

	i = table->DbNextRecord(ndx1);
	if (i >= 0) {
		p = table->DbGetChar("field");
		strcpy_s(item, p);
		printf("Next at eof SHOULD NOT PRINT %s\n", item);
	}

	MakeSearchKey(key, "s", "boathook");
	i = table->DbSearchRecord(ndx1, key);
	MakeSearchKey(key, "s", "boat");
	j = table->DbSearchRecord(ndx1, key);
	printf("Search - exact hit %d, less than %d\n", i, j);

	// establish position with ndx 1, update, then Next using ndx2
	table->DbFirstRecord(ndx1);
	p = table->DbGetChar("field3");
	// DbFirstRecord - (Bolero)

	table->DbNextRecord(ndx1);
	p = table->DbGetChar("field3");
	// Next - (Voyager)

	table->DbSetField("field1", "zephyr");
	table->DbSetField("field3", "Voyager II");
	table->DbUpdateRecord();

	table->DbNextRecord(ndx2);
	p = table->DbGetChar("field3");
	strcpy_s(item, p);
	ifld = table->DbGetInt("field2");
	printf("Next index 2 - (Liberty) Fetched data %s %d\n", p, ifld);

	dbfile->PrintTree();

	// read a table using data ndx
	table->DbFirstRecord(ndx0);
	p = table->DbGetChar("field3");
	strcpy_s(item, p);
	printf("DbFirstRecord - (Kermit) base index %s\n", item);

	table->DbNextRecord(ndx0);
	p = table->DbGetChar("field3");
	strcpy_s(item, p);
	printf("Next index 0 - (Bond) Fetched data %s\n", item);

	ndx3 = table->DbGetIndexObject("decindex");
	if (ndx3 == NULL)
		ndx3 = table->DbMakeIndex("decindex", "d", xlst3);
	dbfile->PrintTree();
	dbase.DbLogout(dbfile);
	system("pause");
	return 0;
}


