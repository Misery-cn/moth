cmake_minimum_required(VERSION 2.8.11)

include_directories(../../include/sys)
include_directories(../../include/common)
include_directories(../../include/exception)
aux_source_directory(. SRCS)
add_library(moth_sys STATIC ${SRCS})
