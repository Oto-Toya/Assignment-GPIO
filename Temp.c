#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <unistd.h>

// ---------Notes---------
// GPIO FUNCTION SELECT REGISTERS:
// FSEL0 (GPIO+0): GPIO 0-9    (3 bits per pin)
// FSEL1 (GPIO+1): GPIO 10-19  (3 bits per pin)
// FSEL2 (GPIO+2): GPIO 20-29  (3 bits per pin)
//
// Each pin gets 3 bits: 000 = INPUT, 001 = OUTPUT
// Pin's bit position = (pin % 10) * 3

// pin 17: FSEL1, bit = 17 - 10 = 7, so bit position = 7 * 3 = 21
// pin 17: Input: 000, Output: 001,
// Input mask: 0xFF1FFFFF (clear bits 21-23), Output mask: 0x00200000 (set bit 21)

void pinSelect(int pin, int mode);
void setPin(int pin, int state);
int readPin(int pin);
int Pressed(int pins[], int count);
long long GetTimeMS(void);
void DelayMS(int pins[], int count, long long ms);

#define NUM_BUTTONS 3
#define NUM_LEDS 3

volatile unsigned int* GPIO;

main(void) {
	// USE VIRTUAL MEMORY SPACE FOR PI 3 //
	// unsigned int BASE = 0x3F200000;
	// USE VIRTUAL MEMORY SPACE FOR PI 4 //
	// unsigned int BASE = 0xFE200000;
	unsigned int BASE = 0xFE200000;
	int MEM, i;
	int L2Rbutton, R2Lbutton, Quitbutton, LED1, LED2, LED3;
	// TEST FOR ROOT ACCESS //
	if (getuid() != 0) {
		printf("ROOT PRIVILEGES REQUIRED\n");
		return 1;
	}
	// OPEN MEMORY INTERFACE //
	if ((MEM = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		printf("CANNOT OPEN MEMORY INTERFACE\n");
		return 2;
	}
	// SET POINTER GPIO TO //
	// CONTROL MEMORY //
	// BASE ADDRESS (BASE) //
	GPIO = (unsigned int*)mmap(0, getpagesize(), PROT_READ | PROT_WRITE,
		MAP_SHARED, MEM, BASE);
	if ((unsigned int)GPIO < 0) {
		printf("MEMORY MAPPING FAILED\n");
		return 3;
	}
	L2Rbutton = 17;
	R2Lbutton = 27;
	Quitbutton = 22;
	
	LED1 = 5;
	LED2 = 6;
	LED3 = 26;

	pinSelect(L2Rbutton, 0);	// Set GPIO 17 as INPUT
	pinSelect(R2Lbutton, 0);	// Set GPIO 27 as INPUT
	pinSelect(Quitbutton, 0);	// Set GPIO 22 as INPUT
	pinSelect(LED1, 1);	// Set GPIO 5 as OUTPUT	
	pinSelect(LED2, 1);	// Set GPIO 6 as OUTPUT
	pinSelect(LED3, 1);	// Set GPIO 26 as OUTPUT

	int Inputs[3] = {17, 27, 22};
	int Leds[3] = { 5, 6, 26 };
	int current = 0;	// Index of the currently lit LED
	int step = 1;		// +1 = left-to-right, -1 = right-to-left
	int btn;

	while (1)
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
		for (i = 0; i < NUM_LEDS; i++) {
			setPin(Leds[i], (i == current) ? 1 : 0);
		}

		// Advance to next LED, wrapping around
		current = (current + step + NUM_LEDS) % NUM_LEDS;

		DelayMS(Inputs, NUM_BUTTONS, 500);
	}

	// Turn off all LEDs before exiting
	for (i = 0; i < NUM_LEDS; i++) {
		setPin(Leds[i], 0);
	}

	munmap((void*)GPIO, getpagesize());
	close(MEM);
}

// Mode: 0 = INPUT, 1 = OUTPUT
void pinSelect(int pin, int mode) {
	int bitPos = (pin % 10) * 3;	// Calculate bit position for the pin
	int regIndex = pin / 10;		// Determine which FSEL register to use (0, 1, or 2)

	if (mode == 0) {
		// Set pin as INPUT: Clear the 3 bits
		unsigned int mask = ~(0x7 << bitPos); // 0x7 = 0000 0000 0000 0000 0000 0000 0000 0111
		*(GPIO + regIndex) &= mask;		// Clear the bits to set as INPUT
	} else if (mode == 1) {
		// Set pin as OUTPUT: Clear then set the appropriate bit
		unsigned int clearMask = ~(0x7 << bitPos);	// Clear the 3 bits first
		unsigned int setMask = (0x1 << bitPos); // Set only the first bit for OUTPUT
		*(GPIO + regIndex) &= clearMask; // Clear the bits first
		*(GPIO + regIndex) |= setMask;   // Then set the OUTPUT bit
	}
	else 
	{
		printf("Invalid mode. Use 0 for INPUT and 1 for OUTPUT.\n");
	}
}

void setPin(int pin, int state) {
	int mask = (1 << (pin)); // Create a mask for the specific pin
	if (state == 1){
		*(GPIO + 7) = mask;
	}
	else if (!state) {
		*(GPIO + 10) = mask;
	}
	else {
		printf("Invalid state. Use 0 for LOW and 1 for HIGH.\n");
	}
}

int readPin(int pin) {
	int mask = (1 << (pin)); // Create a mask for the specific pin
	int value = *(GPIO + 13) & mask; // Read the level register and apply the mask
	return value ? 1 : 0; // Return 1 if HIGH, 0 if LOW

}

int Pressed(int pins[], int count) {
	int i;
	for (i = 0; i < count; i++) {
		if (readPin(pins[i]) == 1) {
			return pins[i]; // Return the pin number of the pressed button
		}
	}
	return -1; // Return -1 if no button is pressed
}

long long GetTimeMS(void) {
	struct timeb time;      // Structure to hold time information
	ftime(&time);           // Get the current time and store it in the time structure
	return (long long)time.time * 1000 + time.millitm;  // Calculate and return the current time in milliseconds by combining seconds and milliseconds
}

// Function to delay execution for a specified number of milliseconds
void DelayMS(int pins[], int count, long long ms) {
	long long start = GetTimeMS();

	while (GetTimeMS() - start < ms) {
		if (Pressed(pins, count) != -1)
			break;
	}
}