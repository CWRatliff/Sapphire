
int ItemBuild(char *dest, int nflds, RField *fldlst[]);
int ItemBuild(char *dest, int nflds, RField *fldlst[], int fldtype[]);
int ItemDistribute(const char* src, RField *fldlst[]);
void ItemPrint(char* src);
#ifdef LINUX
int stricpy(char* d, const char* s);
int stricmp(const char *s1, const char *s2);
int strnicmp(const char *s1, const char *s2, int len);
#endif