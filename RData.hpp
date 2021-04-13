#if !defined(_DATA_H_)

#define _DATA_H_

class RData {

//#include "RField.h"
	private:
		char*	datStr;
		int		datLen;

	public:
		RData();
		RData(char* item);
		RData(const char* item);
		RData(char *item, int dlen);
		~RData();

		int		GetDataLen() {return datLen;}
		char*	GetDataStr() {return datStr;}
		int		SetData(char* str);
		void	PrintData();

	};
#endif