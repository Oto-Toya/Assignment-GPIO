#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

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

#define INPUT  0
#define OUTPUT 1
#define LOW    0
#define HIGH   1

#define PINS_PER_REG  10
#define BITS_PER_PIN  3
#define PIN_MASK    0x7
#define OUTPUT_CODE   0x1
#define LEFT_TO_RIGHT 1
#define RIGHT_TO_LEFT -1
#define MS_PER_SEC 1000LL
#define NS_PER_MS 1000000LL

void pinSelect(int pin, int mode);		// Mode: 0 = INPUT, 1 = OUTPUT
void setPin(int pin, int state);		// State: 0 = LOW, 1 = HIGH
int readPin(int pin);					// Returns 0 for LOW, 1 for HIGH
long long GetTimeMS();					// Returns current time in milliseconds
int Pressed(int pins[], int count);		// Returns pin number of pressed button, or -1 if none
void DelayMS(int pins[], int count, long long ms);		// Delays for specified ms, but exits early if any button is pressed

#define NUM_BUTTONS 3		// L2Rbutton, R2Lbutton, Quitbutton
#define NUM_LEDS 3			// LED1, LED2, LED3
#define delayTime 500		// Delay time in milliseconds
#define setReg 7			// GPIO+7 is the SET register
#define clrReg 10			// GPIO+10 is the CLR register
#define readReg 13			// GPIO+13 is the LEVEL register

volatile unsigned int* GPIO;	// Pointer to GPIO memory-mapped I/O

int main(void) {
	// USE VIRTUAL MEMORY SPACE FOR PI 3 //
	// unsigned int BASE = 0x3F200000;
	// USE VIRTUAL MEMORY SPACE FOR PI 4 //
	// unsigned int BASE = 0xFE200000;
	unsigned int BASE = 0xFE200000;		// Update this if using a different Raspberry Pi model
	int MEM, i;	
	int L2Rbutton, R2Lbutton, Quitbutton, LED1, LED2, LED3;
	// TEST FOR ROOT ACCESS //
	if (getuid() != 0) {		// Check if the user ID is not root (0)
		printf("ROOT PRIVILEGES REQUIRED\n");
		return 1;
	}
	// OPEN MEMORY INTERFACE //
	if ((MEM = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {	// Open /dev/mem with read/write and synchronous access
		printf("CANNOT OPEN MEMORY INTERFACE\n");
		return 2;
	}
	// SET POINTER GPIO TO //
	// CONTROL MEMORY //
	// BASE ADDRESS (BASE) //
	GPIO = (unsigned int*)mmap(0, getpagesize(), PROT_READ | PROT_WRITE,
		MAP_SHARED, MEM, BASE);
	if (GPIO == MAP_FAILED) {		// Check if memory mapping failed
		printf("MEMORY MAPPING FAILED\n");
		return 3;
	}
	L2Rbutton = 17;		// GPIO pin for Left-to-Right button
	R2Lbutton = 27;		// GPIO pin for Right-to-Left button
	Quitbutton = 22;	// GPIO pin for Quit button
	
	LED1 = 5;
	LED2 = 6;
	LED3 = 26;

	pinSelect(L2Rbutton, INPUT);	// Set GPIO 17 as INPUT
	pinSelect(R2Lbutton, INPUT);	// Set GPIO 27 as INPUT
	pinSelect(Quitbutton, INPUT);	// Set GPIO 22 as INPUT
	pinSelect(LED1, OUTPUT);	// Set GPIO 5 as OUTPUT	
	pinSelect(LED2, OUTPUT);	// Set GPIO 6 as OUTPUT
	pinSelect(LED3, OUTPUT);	// Set GPIO 26 as OUTPUT

	int Inputs[NUM_BUTTONS] = { 17, 27, 22 };
	int Leds[NUM_LEDS] = { 5, 6, 26 };
	int current = 0;	// Index of the currently lit LED
	int step = LEFT_TO_RIGHT;		// +1 = left-to-right, -1 = right-to-left
	int btn = -1;

	while (btn != Quitbutton)
	{
		btn = Pressed(Inputs, NUM_BUTTONS);		// Check which button is pressed

		if (btn == Quitbutton) break;			// Exit the loop if Quit button is pressed

		if (btn == L2Rbutton) {
			step = LEFT_TO_RIGHT;	// Left to Right
		}
		else if (btn == R2Lbutton) {
			step = RIGHT_TO_LEFT;	// Right to Left
		}

		// Light only the current LED
		for (i = 0; i < NUM_LEDS; i++) {
			setPin(Leds[i], (i == current) ? HIGH : LOW);	// Set current LED HIGH, others LOW
		}

		// Advance to next LED, wrapping around
		current = (current + step + NUM_LEDS) % NUM_LEDS;	// Wrap around using modulo

		DelayMS(Inputs, NUM_BUTTONS, delayTime);	// Delay for specified time, but exit early if any button is pressed
	}

	// Turn off all LEDs before exiting
	for (i = 0; i < NUM_LEDS; i++) {
		setPin(Leds[i], LOW);
	}
	munmap((void*)GPIO, getpagesize());			// Unmap the GPIO memory
	close(MEM);			// Close the memory interface
	return 0;
}

// Mode: 0 = INPUT, 1 = OUTPUT
void pinSelect(int pin, int mode) {
	int bitPos = (pin % PINS_PER_REG) * BITS_PER_PIN;	// Calculate bit position for the pin
	int regIndex = pin / PINS_PER_REG;		// Determine which FSEL register to use (0, 1, or 2)

	if (mode == INPUT) {
		// Set pin as INPUT: Clear the 3 bits
		unsigned int mask = ~(PIN_MASK << bitPos); // 0x7 = 0000 0000 0000 0000 0000 0000 0000 0111
		*(GPIO + regIndex) &= mask;		// Clear the bits to set as INPUT
	} else if (mode == OUTPUT) {
		// Set pin as OUTPUT: Clear then set the appropriate bit
		unsigned int clearMask = ~(PIN_MASK << bitPos);	// Clear the 3 bits first
		unsigned int setMask = (OUTPUT_CODE << bitPos); // Set only the first bit for OUTPUT
		*(GPIO + regIndex) &= clearMask; // Clear the bits first
		*(GPIO + regIndex) |= setMask;   // Then set the OUTPUT bit
	} else {
		printf("Invalid mode. Use 0 for INPUT and 1 for OUTPUT.\n");
	}
}

// State: 0 = LOW, 1 = HIGH
void setPin(int pin, int state) {
	int mask = (1 << (pin)); // Create a mask for the specific pin
	if (state == HIGH) {
		*(GPIO + setReg) = mask;		// Set the pin HIGH by writing to the SET register (GPIO+7)
	}
	else if (state == LOW) {
		*(GPIO + clrReg) = mask;	// Set the pin LOW by writing to the CLR register (GPIO+10)
	}
	else {
		printf("Invalid state. Use 0 for LOW and 1 for HIGH.\n");
	}
}

// Returns 0 for LOW, 1 for HIGH
int readPin(int pin) {
	int mask = (1 << (pin)); // Create a mask for the specific pin
	int value = *(GPIO + readReg) & mask; // Read the level register and apply the mask
	return value ? 1 : 0; // Return 1 if HIGH, 0 if LOW
}

// Returns pin number of pressed button, or -1 if none
int Pressed(int pins[], int count) {
	int i;
	for (i = 0; i < count; i++) {
		if (readPin(pins[i]) == HIGH) {
			return pins[i]; // Return the pin number of the pressed button
		}
	}
	return -1; // Return -1 if no button is pressed
}

// Returns current time in milliseconds
long long GetTimeMS(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);	// Get the current time in seconds and nanoseconds
	return (long long)ts.tv_sec * MS_PER_SEC + ts.tv_nsec / NS_PER_MS;	// Convert to milliseconds
}

// Function to delay execution for a specified number of milliseconds
void DelayMS(int pins[], int count, long long ms) {
	long long start = GetTimeMS();		// Get the start time in milliseconds
	
	while (GetTimeMS() - start < ms) {	// Loop until the specified time has elapsed
		if (Pressed(pins, count) != -1)		// Check if any button is pressed during the delay, if so, exit the delay early
			break;
	}
}
