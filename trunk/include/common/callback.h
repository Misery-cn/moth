#ifndef _CALLBACK_H_
#define _CALLBACK_H_

class Callback
{
public:
	Callback() {}
	virtual ~Callback() {}

	virtual void complete(int r)
	{
		finish(r);
		delete this;
	}

protected:
	virtual void finish(int r) = 0;
	

private:
	// ½ûÓÃ
	Callback(const Callback& other);
	const Callback& operator=(const Callback& other);
};

#endif