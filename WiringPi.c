#include <stdio.h>
#include <time.h>
#include <wiringPi.h>	

long long GetTimeMS();
int Pressed(int pins[], int count);
void DelayMS(int pins[], int count, long long ms);

#define NUM_BUTTONS 3
#define NUM_LEDS 3



int main(void) {
	wiringPiSetupGpio();

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

	int Inputs[3] = { 17, 27, 22 };
	int Leds[3] = { 5, 6, 26 };
	int current = 0;	// Index of the currently lit LED
	int step = 1;		// +1 = left-to-right, -1 = right-to-left
	int btn = -1;

	while (btn != Quitbutton)
	{
		btn = Pressed(Inputs, NUM_BUTTONS);

		if (btn == Quitbutton) break;

		if (btn == L2Rbutton) {
			step = 1;	// Left to Right
		}
		else if (btn == R2Lbutton) {
			step = -1;	// Right to Left
		}

		// Light only the current LED
		int i;
		for (i = 0; i < NUM_LEDS; i++) {
			digitalWrite(Leds[i], (i == current) ? 1 : 0);
		}

		// Advance to next LED, wrapping around
		current = (current + step + NUM_LEDS) % NUM_LEDS;

		DelayMS(Inputs, NUM_BUTTONS, 500);
	}
	return 0;
}

int Pressed(int pins[], int count) {
	int i;
	for (i = 0; i < count; i++) {
		if (digitalRead(pins[i]) == 1) {
			return pins[i]; // Return the pin number of the pressed button
		}
	}
	return -1; // Return -1 if no button is pressed
}

long long GetTimeMS(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

// Function to delay execution for a specified number of milliseconds
void DelayMS(int pins[], int count, long long ms) {
	long long start = GetTimeMS();

	while (GetTimeMS() - start < ms) {
		if (Pressed(pins, count) != -1)
			break;
	}
}
