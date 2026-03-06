#include "Arduino.h"
#include "timer.h"     // Custom timer class used for non-blocking timing

// Configure switch polarity.
// If the hardware switch outputs HIGH when ON, keep this as-is.
// If the switch is active-low, change the #if 1 to #if 0.
#if 1
	#define SWITCH_ON  HIGH
	#define SWITCH_OFF LOW
#else
	#define SWITCH_ON  LOW
	#define SWITCH_OFF HIGH
#endif

// Timing constants (milliseconds)
#define HANGOUT_AFTER	600000   // After 10 minutes of running, enter a "hangout" phase
#define HANGOUT_TIME	60000    // Hangout phase lasts 1 minute
#define ACCEL_RAMP      1250    // Startup ramp delay (allow system to accelerate)
#define DECCEL_RAMP     1250    // Shutdown ramp delay (allow system to decelerate)

// Pin assignments and timing ranges
enum
{
	kRandomSeed = 0,    // Analog pin used to generate random seed
	kSwitch		= 3,    // Input switch pin
	kMagnet		= 8,    // Output pin controlling electromagnet
	kPower		= 10,   // Output pin controlling main power
	kLED		= 13,   // LED indicator pin

	// Random duration for magnet activation (ms)
	kSpinMinimum   = 200,
	kSpinMaximum   = 900,

	// Random delay between magnet activations (ms)
	kDropMinimum   = 10000,
	kDropMaximum   = 45000,
};

// Global state variables
static bool  sIsShutdown = true;   // Tracks whether system is currently shut down
static Timer sMagnetTimer, sLongTimer; // Timers controlling magnet pulses and long pauses

void setup()
{
	// Configure switch input
	pinMode(kSwitch, INPUT);

	// Configure LED output
	pinMode(kLED, OUTPUT);

	// Configure analog pin used for random seed
	pinMode(kRandomSeed, INPUT);

	// Configure power and magnet control pins
	pinMode(kSwitch, INPUT);
	pinMode(kPower, OUTPUT);
	pinMode(kMagnet, OUTPUT);

	// Ensure outputs start OFF
	digitalWrite(kPower, LOW);
	digitalWrite(kMagnet, LOW);
}

// Main pendulum control logic
void pendulum( bool  on )
{
	if ( on )
	{
		// If the long timer has expired, enter a "hangout" period
		// This pauses magnet activity for a while
		if ( sLongTimer.elapsed() )
		{
			sLongTimer.start(HANGOUT_AFTER);   // Restart long cycle timer
			sMagnetTimer.start(HANGOUT_TIME);  // Pause magnet for hangout time
		}

		// If the magnet timer has expired, trigger a magnet pulse
		else if ( sMagnetTimer.elapsed() )
		{
			// Choose random durations
			long spinLength = random(kSpinMinimum, kSpinMaximum); // Magnet ON time
			long runLength = random(kDropMinimum, kDropMaximum);  // Delay until next pulse
	
			// Set timer for next activation
			sMagnetTimer.setInterval(runLength);
	
			// Activate electromagnet
			digitalWrite(kMagnet, HIGH);

			// Keep magnet on for the selected duration
			delay(spinLength);
		}

		// Ensure power remains ON while system is running
		digitalWrite(kPower, HIGH);

		// Turn magnet OFF after pulse
		digitalWrite(kMagnet, LOW);
	}
	else
	{
		// If system is OFF, idle with a delay
		delay(1000);
	}
}

// Startup sequence when system turns ON
void startup()
{
	// Enable power
	digitalWrite(kPower, HIGH);

	// Allow system to ramp up
	delay(ACCEL_RAMP);

	// Reset magnet timer
	sMagnetTimer.setInterval(0);

	// Start long cycle timer
	sLongTimer.start(HANGOUT_AFTER);

	// Turn on status LED
	digitalWrite(kLED, HIGH);

	// Seed random number generator using analog noise
	randomSeed( analogRead(kRandomSeed) );
}

// Shutdown sequence when system turns OFF
void shutdown()
{
	// Ensure magnet is off
	digitalWrite(kMagnet, LOW);

	// Turn off power
	digitalWrite(kPower, LOW);

	// Allow system to slow down
    delay(DECCEL_RAMP);

	// Turn off LED indicator
	digitalWrite(kLED, LOW);

	// Reset magnet timer
	sMagnetTimer.setInterval(0);
}

void loop()
{
	// If switch is OFF
    if ( digitalRead(kSwitch) == SWITCH_OFF )
    {
        // If system was previously running, perform shutdown
        if ( !sIsShutdown )
        {
            sIsShutdown = true;
            shutdown();
        }
    }

	// If switch is ON and system was previously shut down
    else if ( sIsShutdown )
    {
        sIsShutdown = false;
        startup();
    }

	// Run pendulum logic depending on current state
    pendulum(!sIsShutdown);
}
