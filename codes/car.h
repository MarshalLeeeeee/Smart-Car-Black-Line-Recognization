//小车类封装
#include"PORT.h"

class car
{
	ComPort com;
public:
	car() :com((LPCWSTR)L"COM14")
	{
	}
	void turnl()
	{
		com.Send('L');
	}
	void turnr()
	{
		com.Send('R');
	}
	void run()
	{
		com.Send('A');
	}
	void stop()
	{
		com.Send('P');
	}
	void back()
	{
		com.Send('B');
	}
};