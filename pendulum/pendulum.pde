
#include "timer.h"

#if 1
	#define SWITCH_ON  HIGH
	#define SWITCH_OFF LOW
#else
	#define SWITCH_ON  LOW
	#define SWITCH_OFF HIGH
#endif

#define HANGOUT_AFTER	600000
#define HANGOUT_TIME	60000
#define ACCEL_RAMP      1250
#define DECCEL_RAMP     1250

enum
{
	kRandomSeed = 0,
	kSwitch		= 3,
	kMagnet		= 8,
	kPower		= 10,
	kLED		= 13,

	kSpinMinimum   = 200,
	kSpinMaximum   = 900,

	kDropMinimum   = 10000,
	kDropMaximum   = 45000,
};

static bool  sIsShutdown = true;
static Timer sMagnetTimer, sLongTimer;

void setup()
{
	pinMode(kSwitch, INPUT);

	pinMode(kLED, OUTPUT);
	pinMode(kRandomSeed, INPUT);

	pinMode(kSwitch, INPUT);
	pinMode(kPower, OUTPUT);
	pinMode(kMagnet, OUTPUT);

	digitalWrite(kPower, LOW);
	digitalWrite(kMagnet, LOW);
}

void pendulum( bool  on )
{
	if ( on )
	{
		if ( sLongTimer.elapsed() )
		{
			sLongTimer.start(HANGOUT_AFTER);
			sMagnetTimer.start(HANGOUT_TIME);
		}
		else if ( sMagnetTimer.elapsed() )
		{
			long spinLength = random(kSpinMinimum, kSpinMaximum);
			long runLength = random(kDropMinimum, kDropMaximum);
	
			sMagnetTimer.setInterval(runLength);
	
			digitalWrite(kMagnet, HIGH);
			delay(spinLength);
		}
		digitalWrite(kPower, HIGH);
		digitalWrite(kMagnet, LOW);
	}
	else
		delay(1000);
}

void startup()
{
	digitalWrite(kPower, HIGH);
	delay(ACCEL_RAMP);

	sMagnetTimer.setInterval(0);
	sLongTimer.start(HANGOUT_AFTER);

	digitalWrite(kLED, HIGH);

	randomSeed( analogRead(kRandomSeed) );
}

void shutdown()
{
	digitalWrite(kMagnet, LOW);
	digitalWrite(kPower, LOW);
    delay(DECCEL_RAMP);
	digitalWrite(kLED, LOW);

	sMagnetTimer.setInterval(0);
}

void loop()
{
    if ( digitalRead(kSwitch) == SWITCH_OFF )
    {
        if ( !sIsShutdown )
        {
            sIsShutdown = true;
            shutdown();
        }
    }
    else if ( sIsShutdown )
    {
        sIsShutdown = false;
        startup();
    }
    pendulum(!sIsShutdown);
}
