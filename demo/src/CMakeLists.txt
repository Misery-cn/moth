cmake_minimum_required(VERSION 2.8.11)

include_directories(../include)
include_directories(../../trunk/include/arch)
include_directories(../../trunk/include/sys)
include_directories(../../trunk/include/common)
include_directories(../../trunk/include/exception)
include_directories(../../trunk/include/log)
include_directories(../../trunk/include/net)

if(HAVE_GOOD_YASM_ELF64)
    message("compile crc32c_intel_fast_asm and crc32c_intel_fast_zero_asm")
    set(yasm_srcs
        ../../trunk/src/common/crc32c_intel_fast_asm.S
        ../../trunk/src/common/crc32c_intel_fast_zero_asm.S)
endif(HAVE_GOOD_YASM_ELF64)


SET(SRC_LIST moth.cpp
	master.cpp
	mastermap.cpp
    ../../trunk/src/arch/arm.c
    ../../trunk/src/arch/intel.c
    ../../trunk/src/arch/probe.cpp
    ../../trunk/src/sys/socket.cpp
    ../../trunk/src/sys/thread.cpp
    ../../trunk/src/sys/thread_pool.cpp
    ../../trunk/src/sys/cond.cpp
    ../../trunk/src/sys/mutex.cpp
    ../../trunk/src/sys/mmap.cpp
    ../../trunk/src/sys/rw_lock.cpp
    ../../trunk/src/sys/share_memory.cpp
    ../../trunk/src/sys/spinlock.cpp
    ../../trunk/src/exception/exception.cpp
    ../../trunk/src/common/armor.cpp
    ../../trunk/src/common/buffer.cpp
    ../../trunk/src/common/config_utils.cpp
    ../../trunk/src/common/crc32.cpp
    ../../trunk/src/common/crc32_aarch64.c
    ../../trunk/src/common/crc32_intel_baseline.c
    ../../trunk/src/common/crc32_intel_fast.c
    ../../trunk/src/common/crc32_sctp.c
	${yasm_srcs}
    ../../trunk/src/common/env.cpp
    ../../trunk/src/common/page.cpp
    ../../trunk/src/common/safe_io.cpp
    ../../trunk/src/common/singleton.cpp
    ../../trunk/src/common/strtol.cpp
    ../../trunk/src/common/throttle.cpp
    ../../trunk/src/common/timer.cpp
    ../../trunk/src/common/time_utils.cpp
    ../../trunk/src/common/utils.cpp
    ../../trunk/src/common/string_utils.cpp
    ../../trunk/src/common/dir_utils.cpp
    ../../trunk/src/common/file_utils.cpp
    ../../trunk/src/log/logger.cpp
    ../../trunk/src/log/log_appender.cpp
    ../../trunk/src/net/msg_types.cpp
    ../../trunk/src/net/accepter.cpp
    ../../trunk/src/net/dispatch_queue.cpp
    ../../trunk/src/net/messenger.cpp
    ../../trunk/src/net/message.cpp
    ../../trunk/src/net/simple_messenger.cpp
    ../../trunk/src/net/dispatcher.cpp
    ../../trunk/src/net/socketconnection.cpp
)


ADD_EXECUTABLE(moth ${SRC_LIST})

target_link_libraries(moth) 
