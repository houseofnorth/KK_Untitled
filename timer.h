/*
	timer.h
*/

#pragma once

#ifndef _timer_h__
#define _timer_h__

class Timer
{
protected:
	unsigned long mStarted,
				  mInterval;
public:

	Timer( unsigned int ival = 0 );

	bool elapsed();
	void setInterval( unsigned long ival );
	void start( unsigned long ival );
};

#endif /* _timer_h__ */
