CC=g++
CFLAGS= -c -Wall
LDFLAGS= -g -pthread -lrt -lz
target=moth

WORK_HOME=/home/misery/moth
BASE_DIR=${WORK_HOME}/base
BASE_SRC=${BASE_DIR}/src
BASE_INC=${BASE_DIR}/include
SRCS=$(wildcard *.cpp ${WORK_HOME}/lru_cache/*.cpp \
		${WORK_HOME}/ftp_client/*.cpp ${BASE_SRC}/common/*.cpp \
		${BASE_SRC}/session/*.cpp ${BASE_SRC}/message/*.cpp ${BASE_SRC}/sys/*.cpp ${BASE_SRC}/db/*.cpp \
		${BASE_SRC}/log/*.cpp ${BASE_SRC}/exception/*.cpp ${BASE_SRC}/config/*.cpp)
		
OBJS=$(patsubst %cpp,%o,$(SRCS))

INCLUDE=-I${WORK_HOME}/lru_cache/ -I${WORK_HOME}/ftp_client/ -I${BASE_INC}/sys/ \
		-I${BASE_INC}/common/ -I${BASE_INC}/session/ -I${BASE_INC}/message/ -I${BASE_INC}/db/ \
		-I${BASE_INC}/log/ -I${BASE_INC}/exception/ -I${BASE_INC}/config/

all : $(target)

moth : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo  $@ Build OK.
	
.SUFFIXES:.cpp .o
%.o	: %.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $<	

clean:
	@rm -rf $(target) *.o ${WORK_HOME}/lru_cache/*.o ${WORK_HOME}/ftp_client/*.o \
	${BASE_SRC}/sys/*.o ${BASE_SRC}/common/*o ${BASE_SRC}/session/*.o ${BASE_SRC}/message/*.o \
	${BASE_SRC}/db/*.o ${BASE_SRC}/log/*.o ${BASE_SRC}/exception/*.o ${BASE_SRC}/config/*.o
	
