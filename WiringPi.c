#include <stdio.h>
#include <time.h>
#include <wiringPi.h>	

long long GetTimeMS();
int Pressed(int pins[], int count);
void DelayMS(int pins[], int count, long long ms);

#define NUM_BUTTONS 3
#define NUM_LEDS 3
#define LEFT_TO_RIGHT 1
#define RIGHT_TO_LEFT -1
#define delayTime 500
#define HIGH 1
#define LOW 0
#define MS_PER_SEC 1000LL
#define NS_PER_MS 1000000LL

int main(void) {
	wiringPiSetupGpio();	// Initialize wiringPi with GPIO pin numbering

	int L2Rbutton, R2Lbutton, Quitbutton, LED1, LED2, LED3;

	L2Rbutton = 17;
	R2Lbutton = 27;
	Quitbutton = 22;

	LED1 = 5;
	LED2 = 6;
	LED3 = 26;

	pinMode(L2Rbutton, INPUT);	// Set GPIO 17 as INPUT
	pinMode(R2Lbutton, INPUT);	// Set GPIO 27 as INPUT
	pinMode(Quitbutton, INPUT);	// Set GPIO 22 as INPUT
	pinMode(LED1, OUTPUT);	// Set GPIO 5 as OUTPUT	
	pinMode(LED2, OUTPUT);	// Set GPIO 6 as OUTPUT
	pinMode(LED3, OUTPUT);	// Set GPIO 26 as OUTPUT

	int Inputs[NUM_BUTTONS] = { 17, 27, 22 };
	int Leds[NUM_LEDS] = { 5, 6, 26 };
	int current = 0;	// Index of the currently lit LED
	int step = LEFT_TO_RIGHT;		// +1 = left-to-right, -1 = right-to-left
	int btn = -1;
	int i;

	while (btn != Quitbutton)
	{
		btn = Pressed(Inputs, NUM_BUTTONS);		// Check which button is pressed

		if (btn == Quitbutton) {		// Exit the loop if Quit button is pressed
			break;
		}

		if (btn == L2Rbutton) {
			step = LEFT_TO_RIGHT;	// Left to Right
		}
		else if (btn == R2Lbutton) {
			step = RIGHT_TO_LEFT;	// Right to Left
		}

		// Light only the current LED
		for (i = 0; i < NUM_LEDS; i++) {
			digitalWrite(Leds[i], (i == current) ? HIGH : LOW);		// Set current LED HIGH, others LOW
		}

		// Advance to next LED, wrapping around
		current = (current + step + NUM_LEDS) % NUM_LEDS;	// Wrap around using modulo

		DelayMS(Inputs, NUM_BUTTONS, delayTime);	// Delay for specified time, but exit early if any button is pressed
	}
	for (i = 0; i < NUM_LEDS; i++) {		// Turn off all LEDs before exiting
		digitalWrite(Leds[i], LOW);
	}
	return 0;
}

// Returns pin number of pressed button, or -1 if none
int Pressed(int pins[], int count) {
	int i;
	for (i = 0; i < count; i++) {		// Loop through each button pin
		if (digitalRead(pins[i]) == HIGH) {	// Check if the button is pressed (active HIGH)
			return pins[i]; // Return the pin number of the pressed button
		}
	}
	return -1; // Return -1 if no button is pressed
}

//	Function to get the current time in milliseconds
long long GetTimeMS(void) {
	struct timespec ts;	
	clock_gettime(CLOCK_MONOTONIC, &ts);	// Get the current time in seconds and nanoseconds
	return (long long)ts.tv_sec * MS_PER_SEC + ts.tv_nsec / NS_PER_MS;	// Convert to milliseconds
}

// Function to delay execution for a specified number of milliseconds
void DelayMS(int pins[], int count, long long ms) {
	long long start = GetTimeMS();		// Get the start time in milliseconds

	while (GetTimeMS() - start < ms) {	// Loop until the specified time has elapsed
		if (Pressed(pins, count) != -1)	// Exit early if any button is pressed
			break;
	}
}

