#if !defined( _RPAGE_H_ )

#define _RPAGE_H_

class RPage { 
	
	protected:
		short	*pLen;              // length of page (2 byte words)
		short	*pCount;            // # of slots
		short	*pFree;             // # of free words
		short	*pLWM;              // low water mark of used page
		short	*page;              // address of page start
		
	public:
		RPage();
		RPage(char *gift, const int len);

		int		Allocate(const int slot, const int len);
		int		BigEnough(const int len);
		int		Delete(const int slot);
		int		Delete(const int slot1, const int slot2);
		char*	GetItem(const int slot);
		int		GetLen(const int slot);
		int		GetSlots() {return *pCount;}
		int		GetUsed() {return(*pLen - *pFree);}
		void	Initialize(char *gift, const int len);
//		Insert(const char *item, const len, const slot, const size);
		int		Insert(const char *item, const int len, const int slot, const int offset = 0);
		void	Reset(char *gift, const int len);
		
		void	Dump();

	private:
		int		Locator(const int tlen);
		void	Setup(const int ptr, const int tlen);	
	};
	
#endif
