CC=g++
CFLAGS= -c -g -pthread -lrt -D_REENTRANT -D`uname -s` -Wall
LDFLAGS= -g -pthread -lrt
target=moth

WORK_HOME=/home/misery/MyProject/MyCpp
SRCS=$(wildcard *.cpp ${WORK_HOME}/lru_cache/*.cpp \
		${WORK_HOME}/ftp_client/*.cpp ${WORK_HOME}/socket/*.cpp ${WORK_HOME}/common/*.cpp \
		${WORK_HOME}/session/*.cpp ${WORK_HOME}/message/*.cpp ${WORK_HOME}/sys/*.cpp ${WORK_HOME}/sql/*.cpp \
		${WORK_HOME}/log/*.cpp ${WORK_HOME}/exception/*.cpp)
		
OBJS=$(patsubst %cpp,%o,$(SRCS))

INCLUDE=-I${WORK_HOME}/lru_cache/ -I${WORK_HOME}/ftp_client/ -I${WORK_HOME}/socket/ \
		-I${WORK_HOME}/common/ -I${WORK_HOME}/session/ -I${WORK_HOME}/message/ -I{WORK_HOME}/sys/ -I${WORK_HOME}/sql/ \
		-I${WORK_HOME}/log/ -I${WORK_HOME}/exception/

all : $(target)

moth : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo  $@ Build OK.
	
.SUFFIXES:.cpp .o
%.o	: %.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $<	

clean:
	@rm -rf $(target) *.o ${WORK_HOME}/lru_cache/*.o ${WORK_HOME}/ftp_client/*.o \
	${WORK_HOME}/socket/*.o ${WORK_HOME}/common/*o ${WORK_HOME}/session/*.o ${WORK_HOME}/message/*.o \
	${WORK_HOME}/sys/*.o ${WORK_HOME}/sql/*.o ${WORK_HOME}/log/*.o ${WORK_HOME}/exception/*.o
