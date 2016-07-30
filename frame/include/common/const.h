#ifndef _CONST_H_
#define _CONST_H_

const int ALLOC_MEM_ERROR = -1;
const int NO_ERROR = 0;
const int SUCCESS = 0;

const int MIN_IP_LEN = 7;
const int MAX_IP_LEN = 15;
// 吞吐量不大时暂初始为5
const int MAX_CONN = 5;
// 超时时间
const int WAIT_TIME_OUT = 30;

const int K = 1024;
const int M = 1024 * 1024;
const int G = 1024 * 1024 * 1024;
// 1K
const int BUFF_SIZE = 1 * K;
// 1M
const int MAX_READ_SIZE = 1 * M;

const int MAX_FILENAME = 1024;

#endif
