cmake_minimum_required(VERSION 2.8.11)

include_directories(../include)
include_directories(../../trunk/tools/ftp_client)
include_directories(../../trunk/include/sys)
include_directories(../../trunk/include/common)
include_directories(../../trunk/include/exception)
include_directories(../../trunk/include/log)

SET(SRC_LIST moth.cpp
    ../../trunk/tools/ftp_client/ftp.cpp
    ../../trunk/src/sys/socket.cpp
    ../../trunk/src/sys/thread.cpp
    ../../trunk/src/sys/thread_pool.cpp
    ../../trunk/src/sys/cond.cpp
    ../../trunk/src/sys/mutex.cpp
    ../../trunk/src/exception/exception.cpp
    ../../trunk/src/common/timer.cpp
    ../../trunk/src/common/datetime_utils.cpp
    ../../trunk/src/common/utils.cpp
    ../../trunk/src/common/string_utils.cpp
    ../../trunk/src/common/dir_utils.cpp
    ../../trunk/src/common/file_utils.cpp
    ../../trunk/src/log/logger.cpp
    ../../trunk/src/log/log.cpp
    ../../trunk/src/log/log_appender.cpp
)

ADD_EXECUTABLE(moth ${SRC_LIST})

target_link_libraries(moth) 
