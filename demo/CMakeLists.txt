cmake_minimum_required(VERSION 2.8.11)

enable_language(C ASM)

project(moth)

include(CMake.common)

set(CMAKE_ASM_COMPILER  ${PROJECT_SOURCE_DIR}/../trunk/yasm-wrapper)
set(CMAKE_ASM_FLAGS "-f elf64")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wtype-limits -Wignored-qualifiers -Winit-self")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith -Werror=format-security -fno-strict-aliasing -fsigned-char")

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
    message(STATUS "optional:-std=c++11")   
endif(CMAKE_COMPILER_IS_GNUCXX)

execute_process(
        COMMAND yasm -f elf64 ${CMAKE_SOURCE_DIR}/../trunk/src/common/crc32c_intel_fast_asm.S -o /dev/null
        RESULT_VARIABLE no_yasm
        OUTPUT_QUIET)
#没有yasm组件
if(no_yasm)
	message("we do not have a modern/working yasm")
else(no_yasm)
	message("we have a modern and working yasm")
	if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64|x86_64")
		message(STATUS " we are x84_64")
		set(save_quiet ${CMAKE_REQUIRED_QUIET})
		set(CMAKE_REQUIRED_QUIET true)
		include(CheckCXXSourceCompiles)
		check_cxx_source_compiles("
		#if defined(__x86_64__) && defined(__ILP32__)
		#error x32
		#endif
		int main() {}" not_arch_x32)
		set(CMAKE_REQUIRED_QUIET ${save_quiet})
		if(not_arch_x32)
			message("we are not x32")
			set(HAVE_GOOD_YASM_ELF64 1)
		else(not_arch_x32)
			message("we are x32; no yasm for you")
		endif(not_arch_x32)
  	else(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64|x86_64")
		message("we are not x86_64 && !x32")
	endif(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64|x86_64")
endif(no_yasm)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|AARCH64")
	set(HAVE_ARM 1)
	CHECK_C_COMPILER_FLAG(-march=armv8-a+crc HAVE_ARMV8_CRC)
	if(HAVE_ARMV8_CRC)
		#设置编译选项
		set(ARM_CRC_FLAGS "-march=armv8-a+crc")
	endif()
endif()

CONFIGURE_FILE(../trunk/config.h.in ../trunk/include/common/acconfig.h)

add_subdirectory(src)
