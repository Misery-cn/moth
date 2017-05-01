#ifndef _PAGE_H_
#define _PAGE_H_

#include "int_types.h"
#include "utils.h"

extern uint32_t _page_size;
extern unsigned long _page_mask;
extern uint32_t _page_shift;


#define PAGE_SIZE _page_size
#define PAGE_MASK _page_mask
#define PAGE_SHIFT _page_shift

#endif
