#include "page.h"

int _get_bits_of(int v)
{
	int n = 0;
	
	while (v)
	{
		n++;
		v = v >> 1;
	}
	
	return n;
}

// uint32_t _page_size = sysconf(_SC_PAGESIZE);
uint32_t _page_size = Utils::get_page_size();
unsigned long _page_mask = ~(unsigned long)(_page_size - 1);
uint32_t _page_shift = _get_bits_of(_page_size - 1);

