/*
	timer.cpp
*/

#include <Arduino.h>

#include "timer.h"
//#include "trace.h"

Timer::Timer( unsigned int  ival ) :
	mStarted( ::millis() ),
	mInterval(ival)
{
}

bool
Timer::elapsed()
{
    unsigned long now = ::millis();

	//TRACE("%ld - %ld > %ld == %d\n", now, mStarted, mInterval, now - mStarted > mInterval)

    if ( now - mStarted > mInterval )
    {
        mStarted = now;
        return ( true );
    }
    return ( false );
}

void
Timer::setInterval( unsigned long ival )
{
    mInterval = ival;
}

void
Timer::start( unsigned long ival )
{
    mStarted = ::millis();
    mInterval = ival;
}
