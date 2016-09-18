#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#if __GNUC__ < 4
	#include "atomic_asm.h"
#else
	#if __WORDSIZE==32
		typedef volatile int atomic_t;
	#elif __WORDSIZE==64
		typedef volatile long atomic_t;
	#endif
	#include "atomic_gcc.h"
#endif


#include "compiler.h"

#endif
