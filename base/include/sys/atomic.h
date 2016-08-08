#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#if __GNUC__ < 4
#include "atomic_asm.h"
#else
#include "atomic_gcc.h"
#if __WORDSIZE==64
#include "atomic_gcc8.h"
#endif
#endif
#include "compiler.h"

#endif
