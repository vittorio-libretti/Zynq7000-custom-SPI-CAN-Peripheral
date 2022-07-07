#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

// Linux headers
#include <fcntl.h> //Contains file controls like 0_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // Write(), read(), close()

int SerialConnect(const char *);
int SerialDisconnect(const int);
short int Identify(const int);

//Commands

uint32_t HowManyBytes(const char * cmd, const int serial_port);
uint32_t HowManyMessages(const char * cmd, const int serial_port);
uint32_t GetMeBytes(const char * cmd, const int serial_port);
uint32_t GetMeMessages(char * cmd, const int serial_port, const uint32_t size);
