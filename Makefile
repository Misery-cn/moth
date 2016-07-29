CC=g++
CFLAGS= -c -g -pthread -lrt -D_REENTRANT -D`uname -s` -Wall
LDFLAGS= -g -pthread -lrt
target=moth

WORK_HOME=/home/misery/moth
FRAME_DIR=${WORK_HOME}/frame
FRAME_SRC=${FRAME_DIR}/src
FRAME_INC=${FRAME_DIR}/include
SRCS=$(wildcard *.cpp ${WORK_HOME}/lru_cache/*.cpp \
		${WORK_HOME}/ftp_client/*.cpp ${FRAME_SRC}/common/*.cpp \
		${FRAME_SRC}/session/*.cpp ${FRAME_SRC}/message/*.cpp ${FRAME_SRC}/sys/*.cpp ${FRAME_SRC}/db/*.cpp \
		${FRAME_SRC}/log/*.cpp ${FRAME_SRC}/exception/*.cpp ${FRAME_SRC}/config/*.cpp)
		
OBJS=$(patsubst %cpp,%o,$(SRCS))

INCLUDE=-I${WORK_HOME}/lru_cache/ -I${WORK_HOME}/ftp_client/ -I${FRAME_INC}/sys/ \
		-I${FRAME_INC}/common/ -I${FRAME_INC}/session/ -I${FRAME_INC}/message/ -I${FRAME_INC}/db/ \
		-I${FRAME_INC}/log/ -I${FRAME_INC}/exception/ -I${FRAME_INC}/config/

all : $(target)

moth : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo  $@ Build OK.
	
.SUFFIXES:.cpp .o
%.o	: %.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $<	

clean:
	@rm -rf $(target) *.o ${WORK_HOME}/lru_cache/*.o ${WORK_HOME}/ftp_client/*.o \
	${FRAME_SRC}/sys/*.o ${FRAME_SRC}/common/*o ${FRAME_SRC}/session/*.o ${FRAME_SRC}/message/*.o \
	${FRAME_SRC}/db/*.o ${FRAME_SRC}/log/*.o ${FRAME_SRC}/exception/*.o ${FRAME_SRC}/config/*.o
	
