cmake_minimum_required(VERSION 2.8.11)

include_directories(../../include/arch)
include_directories(../../include/common)
include_directories(../../include/exception)
include_directories(../../include/sys)

aux_source_directory(. SRCS)

if(HAVE_GOOD_YASM_ELF64)
	set(yasm_srcs
	crc32c_intel_fast_asm.S
    crc32c_intel_fast_zero_asm.S)
endif(HAVE_GOOD_YASM_ELF64)


if(HAVE_ARMV8_CRC)
	#add_library(common_crc_aarch64 STATIC crc32_aarch64.cpp)
	#set_target_properties(common_crc_aarch64 PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS} ${ARM_CRC_FLAGS}")
	#target_link_libraries(moth_common common_crc_aarch64)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARM_CRC_FLAGS}")
endif(HAVE_ARMV8_CRC)

add_library(moth_common STATIC ${SRCS} ${yasm_srcs})