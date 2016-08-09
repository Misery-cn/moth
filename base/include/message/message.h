
#ifndef _MESSAGE_H_
#define _MESSAGE_H_

struct Message
{
public:
	Message();
	virtual ~Message();
	
	// 由具体的消息自己实现编解码
	virtual int encode() = 0;
	virtual int decode() = 0;
	
private:
	int len_;
};

#endif