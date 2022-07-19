/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2005 - 2021 Teunis van Beelen
*
* Email: teuniz@protonmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************
*/


/* Last revision: February 9, 2021 */
/* For more info and how to use this library, visit: http://www.teuniz.net/RS-232/ */


#include "rs232.h"


#if defined(__linux__) || defined(__FreeBSD__)   /* Linux & FreeBSD */

#define RS232_PORTNR  38


int Cport[RS232_PORTNR],
    error;

struct termios new_port_settings,
       old_port_settings[RS232_PORTNR];

const char *comports[RS232_PORTNR]={"/dev/ttyS0","/dev/ttyS1","/dev/ttyS2","/dev/ttyS3","/dev/ttyS4","/dev/ttyS5",
                                    "/dev/ttyS6","/dev/ttyS7","/dev/ttyS8","/dev/ttyS9","/dev/ttyS10","/dev/ttyS11",
                                    "/dev/ttyS12","/dev/ttyS13","/dev/ttyS14","/dev/ttyS15","/dev/ttyUSB0",
                                    "/dev/ttyUSB1","/dev/ttyUSB2","/dev/ttyUSB3","/dev/ttyUSB4","/dev/ttyUSB5",
                                    "/dev/ttyAMA0","/dev/ttyAMA1","/dev/ttyACM0","/dev/ttyACM1",
                                    "/dev/rfcomm0","/dev/rfcomm1","/dev/ircomm0","/dev/ircomm1",
                                    "/dev/cuau0","/dev/cuau1","/dev/cuau2","/dev/cuau3",
                                    "/dev/cuaU0","/dev/cuaU1","/dev/cuaU2","/dev/cuaU3"};

int RS232_OpenComport(int comport_number, int baudrate, const char *mode, int flowctrl)
{
  int baudr,
      status;

  if((comport_number>=RS232_PORTNR)||(comport_number<0))
  {
    printf("illegal comport number\n");
    return(1);
  }

  switch(baudrate)
  {
    case      50 : baudr = B50;
                   break;
    case      75 : baudr = B75;
                   break;
    case     110 : baudr = B110;
                   break;
    case     134 : baudr = B134;
                   break;
    case     150 : baudr = B150;
                   break;
    case     200 : baudr = B200;
                   break;
    case     300 : baudr = B300;
                   break;
    case     600 : baudr = B600;
                   break;
    case    1200 : baudr = B1200;
                   break;
    case    1800 : baudr = B1800;
                   break;
    case    2400 : baudr = B2400;
                   break;
    case    4800 : baudr = B4800;
                   break;
    case    9600 : baudr = B9600;
                   break;
    case   19200 : baudr = B19200;
                   break;
    case   38400 : baudr = B38400;
                   break;
    case   57600 : baudr = B57600;
                   break;
    case  115200 : baudr = B115200;
                   break;
    case  230400 : baudr = B230400;
                   break;
    case  460800 : baudr = B460800;
                   break;
#if defined(__linux__)
    case  500000 : baudr = B500000;
                   break;
    case  576000 : baudr = B576000;
                   break;
    case  921600 : baudr = B921600;
                   break;
    case 1000000 : baudr = B1000000;
                   break;
    case 1152000 : baudr = B1152000;
                   break;
    case 1500000 : baudr = B1500000;
                   break;
    case 2000000 : baudr = B2000000;
                   break;
    case 2500000 : baudr = B2500000;
                   break;
    case 3000000 : baudr = B3000000;
                   break;
    case 3500000 : baudr = B3500000;
                   break;
    case 4000000 : baudr = B4000000;
                   break;
#endif
    default      : printf("invalid baudrate\n");
                   return(1);
                   break;
  }

  int cbits=CS8,
      cpar=0,
      ipar=IGNPAR,
      bstop=0;

  if(strlen(mode) != 3)
  {
    printf("invalid mode \"%s\"\n", mode);
    return(1);
  }

  switch(mode[0])
  {
    case '8': cbits = CS8;
              break;
    case '7': cbits = CS7;
              break;
    case '6': cbits = CS6;
              break;
    case '5': cbits = CS5;
              break;
    default : printf("invalid number of data-bits '%c'\n", mode[0]);
              return(1);
              break;
  }

  switch(mode[1])
  {
    case 'N':
    case 'n': cpar = 0;
              ipar = IGNPAR;
              break;
    case 'E':
    case 'e': cpar = PARENB;
              ipar = INPCK;
              break;
    case 'O':
    case 'o': cpar = (PARENB | PARODD);
              ipar = INPCK;
              break;
    default : printf("invalid parity '%c'\n", mode[1]);
              return(1);
              break;
  }

  switch(mode[2])
  {
    case '1': bstop = 0;
              break;
    case '2': bstop = CSTOPB;
              break;
    default : printf("invalid number of stop bits '%c'\n", mode[2]);
              return(1);
              break;
  }

/*
http://pubs.opengroup.org/onlinepubs/7908799/xsh/termios.h.html

http://man7.org/linux/man-pages/man3/termios.3.html
*/

  Cport[comport_number] = open(comports[comport_number], O_RDWR | O_NOCTTY | O_NDELAY);
  if(Cport[comport_number]==-1)
  {
    perror("unable to open comport ");
    return(1);
  }

  /* lock access so that another process can't also use the port */
  if(flock(Cport[comport_number], LOCK_EX | LOCK_NB) != 0)
  {
    close(Cport[comport_number]);
    perror("Another process has locked the comport.");
    return(1);
  }

  error = tcgetattr(Cport[comport_number], old_port_settings + comport_number);
  if(error==-1)
  {
    close(Cport[comport_number]);
    flock(Cport[comport_number], LOCK_UN);  /* free the port so that others can use it. */
    perror("unable to read portsettings ");
    return(1);
  }
  memset(&new_port_settings, 0, sizeof(new_port_settings));  /* clear the new struct */

  new_port_settings.c_cflag = cbits | cpar | bstop | CLOCAL | CREAD;
  if(flowctrl)
  {
    new_port_settings.c_cflag |= CRTSCTS;
  }
  new_port_settings.c_iflag = ipar;
  new_port_settings.c_oflag = 0;
  new_port_settings.c_lflag = 0;
  new_port_settings.c_cc[VMIN] = 0;      /* block untill n bytes are received */
  new_port_settings.c_cc[VTIME] = 0;     /* block untill a timer expires (n * 100 mSec.) */

  cfsetispeed(&new_port_settings, baudr);
  cfsetospeed(&new_port_settings, baudr);

  error = tcsetattr(Cport[comport_number], TCSANOW, &new_port_settings);
  if(error==-1)
  {
    tcsetattr(Cport[comport_number], TCSANOW, old_port_settings + comport_number);
    close(Cport[comport_number]);
    flock(Cport[comport_number], LOCK_UN);  /* free the port so that others can use it. */
    perror("unable to adjust portsettings ");
    return(1);
  }

/* http://man7.org/linux/man-pages/man4/tty_ioctl.4.html */

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    tcsetattr(Cport[comport_number], TCSANOW, old_port_settings + comport_number);
    flock(Cport[comport_number], LOCK_UN);  /* free the port so that others can use it. */
    perror("unable to get portstatus");
    return(1);
  }

  status |= TIOCM_DTR;    /* turn on DTR */
  status |= TIOCM_RTS;    /* turn on RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    tcsetattr(Cport[comport_number], TCSANOW, old_port_settings + comport_number);
    flock(Cport[comport_number], LOCK_UN);  /* free the port so that others can use it. */
    perror("unable to set portstatus");
    return(1);
  }

  return(0);
}


int RS232_PollComport(int comport_number, unsigned char *buf, int size)
{
  int n;

  n = read(Cport[comport_number], buf, size);

  if(n < 0)
  {
    if(errno == EAGAIN)  return 0;
  }

  return(n);
}


int RS232_SendByte(int comport_number, unsigned char byte)
{
  int n = write(Cport[comport_number], &byte, 1);
  if(n < 0)
  {
    if(errno == EAGAIN)
    {
      return 0;
    }
    else
    {
      return 1;
    }
  }

  return(0);
}


int RS232_SendBuf(int comport_number, unsigned char *buf, int size)
{
  int n = write(Cport[comport_number], buf, size);
  if(n < 0)
  {
    if(errno == EAGAIN)
    {
      return 0;
    }
    else
    {
      return -1;
    }
  }

  return(n);
}


void RS232_CloseComport(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_DTR;    /* turn off DTR */
  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }

  tcsetattr(Cport[comport_number], TCSANOW, old_port_settings + comport_number);
  close(Cport[comport_number]);

  flock(Cport[comport_number], LOCK_UN);  /* free the port so that others can use it. */
}

/*
Constant  Description
TIOCM_LE        DSR (data set ready/line enable)
TIOCM_DTR       DTR (data terminal ready)
TIOCM_RTS       RTS (request to send)
TIOCM_ST        Secondary TXD (transmit)
TIOCM_SR        Secondary RXD (receive)
TIOCM_CTS       CTS (clear to send)
TIOCM_CAR       DCD (data carrier detect)
TIOCM_CD        see TIOCM_CAR
TIOCM_RNG       RNG (ring)
TIOCM_RI        see TIOCM_RNG
TIOCM_DSR       DSR (data set ready)

http://man7.org/linux/man-pages/man4/tty_ioctl.4.html
*/

int RS232_IsDCDEnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_CAR) return(1);
  else return(0);
}


int RS232_IsRINGEnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_RNG) return(1);
  else return(0);
}


int RS232_IsCTSEnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_CTS) return(1);
  else return(0);
}


int RS232_IsDSREnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_DSR) return(1);
  else return(0);
}


void RS232_enableDTR(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status |= TIOCM_DTR;    /* turn on DTR */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}


void RS232_disableDTR(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_DTR;    /* turn off DTR */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}


void RS232_enableRTS(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status |= TIOCM_RTS;    /* turn on RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}


void RS232_disableRTS(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}


void RS232_flushRX(int comport_number)
{
  tcflush(Cport[comport_number], TCIFLUSH);
}


void RS232_flushTX(int comport_number)
{
  tcflush(Cport[comport_number], TCOFLUSH);
}


void RS232_flushRXTX(int comport_number)
{
  tcflush(Cport[comport_number], TCIOFLUSH);
}


#else  /* windows */

#define RS232_PORTNR  32

HANDLE Cport[RS232_PORTNR];


const char *comports[RS232_PORTNR]={"\\\\.\\COM1",  "\\\\.\\COM2",  "\\\\.\\COM3",  "\\\\.\\COM4",
                                    "\\\\.\\COM5",  "\\\\.\\COM6",  "\\\\.\\COM7",  "\\\\.\\COM8",
                                    "\\\\.\\COM9",  "\\\\.\\COM10", "\\\\.\\COM11", "\\\\.\\COM12",
                                    "\\\\.\\COM13", "\\\\.\\COM14", "\\\\.\\COM15", "\\\\.\\COM16",
                                    "\\\\.\\COM17", "\\\\.\\COM18", "\\\\.\\COM19", "\\\\.\\COM20",
                                    "\\\\.\\COM21", "\\\\.\\COM22", "\\\\.\\COM23", "\\\\.\\COM24",
                                    "\\\\.\\COM25", "\\\\.\\COM26", "\\\\.\\COM27", "\\\\.\\COM28",
                                    "\\\\.\\COM29", "\\\\.\\COM30", "\\\\.\\COM31", "\\\\.\\COM32"};

char mode_str[128];


int RS232_OpenComport(int comport_number, int baudrate, const char *mode, int flowctrl)
{
  if((comport_number>=RS232_PORTNR)||(comport_number<0))
  {
    printf("illegal comport number\n");
    return(1);
  }

  switch(baudrate)
  {
    case     110 : strcpy(mode_str, "baud=110");
                   break;
    case     300 : strcpy(mode_str, "baud=300");
                   break;
    case     600 : strcpy(mode_str, "baud=600");
                   break;
    case    1200 : strcpy(mode_str, "baud=1200");
                   break;
    case    2400 : strcpy(mode_str, "baud=2400");
                   break;
    case    4800 : strcpy(mode_str, "baud=4800");
                   break;
    case    9600 : strcpy(mode_str, "baud=9600");
                   break;
    case   19200 : strcpy(mode_str, "baud=19200");
                   break;
    case   38400 : strcpy(mode_str, "baud=38400");
                   break;
    case   57600 : strcpy(mode_str, "baud=57600");
                   break;
    case  115200 : strcpy(mode_str, "baud=115200");
                   break;
    case  128000 : strcpy(mode_str, "baud=128000");
                   break;
    case  256000 : strcpy(mode_str, "baud=256000");
                   break;
    case  500000 : strcpy(mode_str, "baud=500000");
                   break;
    case  921600 : strcpy(mode_str, "baud=921600");
                   break;
    case 1000000 : strcpy(mode_str, "baud=1000000");
                   break;
    case 1500000 : strcpy(mode_str, "baud=1500000");
                   break;
    case 2000000 : strcpy(mode_str, "baud=2000000");
                   break;
    case 3000000 : strcpy(mode_str, "baud=3000000");
                   break;
    default      : printf("invalid baudrate\n");
                   return(1);
                   break;
  }

  if(strlen(mode) != 3)
  {
    printf("invalid mode \"%s\"\n", mode);
    return(1);
  }

  switch(mode[0])
  {
    case '8': strcat(mode_str, " data=8");
              break;
    case '7': strcat(mode_str, " data=7");
              break;
    case '6': strcat(mode_str, " data=6");
              break;
    case '5': strcat(mode_str, " data=5");
              break;
    default : printf("invalid number of data-bits '%c'\n", mode[0]);
              return(1);
              break;
  }

  switch(mode[1])
  {
    case 'N':
    case 'n': strcat(mode_str, " parity=n");
              break;
    case 'E':
    case 'e': strcat(mode_str, " parity=e");
              break;
    case 'O':
    case 'o': strcat(mode_str, " parity=o");
              break;
    default : printf("invalid parity '%c'\n", mode[1]);
              return(1);
              break;
  }

  switch(mode[2])
  {
    case '1': strcat(mode_str, " stop=1");
              break;
    case '2': strcat(mode_str, " stop=2");
              break;
    default : printf("invalid number of stop bits '%c'\n", mode[2]);
              return(1);
              break;
  }

  if(flowctrl)
  {
    strcat(mode_str, " xon=off to=off odsr=off dtr=on rts=off");
  }
  else
  {
    strcat(mode_str, " xon=off to=off odsr=off dtr=on rts=on");
  }

/*
http://msdn.microsoft.com/en-us/library/windows/desktop/aa363145%28v=vs.85%29.aspx

http://technet.microsoft.com/en-us/library/cc732236.aspx

https://docs.microsoft.com/en-us/windows/desktop/api/winbase/ns-winbase-_dcb
*/

  Cport[comport_number] = CreateFileA(comports[comport_number],
                      GENERIC_READ|GENERIC_WRITE,
                      0,                          /* no share  */
                      NULL,                       /* no security */
                      OPEN_EXISTING,
                      0,                          /* no threads */
                      NULL);                      /* no templates */

  if(Cport[comport_number]==INVALID_HANDLE_VALUE)
  {
    printf("unable to open comport\n");
    return(1);
  }

  DCB port_settings;
  memset(&port_settings, 0, sizeof(port_settings));  /* clear the new struct  */
  port_settings.DCBlength = sizeof(port_settings);

  if(!BuildCommDCBA(mode_str, &port_settings))
  {
    printf("unable to set comport dcb settings\n");
    CloseHandle(Cport[comport_number]);
    return(1);
  }

  if(flowctrl)
  {
    port_settings.fOutxCtsFlow = TRUE;
    port_settings.fRtsControl = RTS_CONTROL_HANDSHAKE;
  }

  if(!SetCommState(Cport[comport_number], &port_settings))
  {
    printf("unable to set comport cfg settings\n");
    CloseHandle(Cport[comport_number]);
    return(1);
  }

  COMMTIMEOUTS Cptimeouts;

  Cptimeouts.ReadIntervalTimeout         = MAXDWORD;
  Cptimeouts.ReadTotalTimeoutMultiplier  = 0;
  Cptimeouts.ReadTotalTimeoutConstant    = 0;
  Cptimeouts.WriteTotalTimeoutMultiplier = 0;
  Cptimeouts.WriteTotalTimeoutConstant   = 0;

  if(!SetCommTimeouts(Cport[comport_number], &Cptimeouts))
  {
    printf("unable to set comport time-out settings\n");
    CloseHandle(Cport[comport_number]);
    return(1);
  }

  return(0);
}


int RS232_PollComport(int comport_number, unsigned char *buf, int size)
{
  int n;

/* added the void pointer cast, otherwise gcc will complain about */
/* "warning: dereferencing type-punned pointer will break strict aliasing rules" */

  if(!ReadFile(Cport[comport_number], buf, size, (LPDWORD)((void *)&n), NULL))
  {
    return -1;
  }

  return(n);
}


int RS232_SendByte(int comport_number, unsigned char byte)
{
  int n;

  if(!WriteFile(Cport[comport_number], &byte, 1, (LPDWORD)((void *)&n), NULL))
  {
    return(1);
  }

  if(n<0)  return(1);

  return(0);
}


int RS232_SendBuf(int comport_number, unsigned char *buf, int size)
{
  int n;

  if(WriteFile(Cport[comport_number], buf, size, (LPDWORD)((void *)&n), NULL))
  {
    return(n);
  }

  return(-1);
}


void RS232_CloseComport(int comport_number)
{
  CloseHandle(Cport[comport_number]);
}

/*
http://msdn.microsoft.com/en-us/library/windows/desktop/aa363258%28v=vs.85%29.aspx
*/

int RS232_IsDCDEnabled(int comport_number)
{
  int status;

  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

  if(status&MS_RLSD_ON) return(1);
  else return(0);
}


int RS232_IsRINGEnabled(int comport_number)
{
  int status;

  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

  if(status&MS_RING_ON) return(1);
  else return(0);
}


int RS232_IsCTSEnabled(int comport_number)
{
  int status;

  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

  if(status&MS_CTS_ON) return(1);
  else return(0);
}


int RS232_IsDSREnabled(int comport_number)
{
  int status;

  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

  if(status&MS_DSR_ON) return(1);
  else return(0);
}


void RS232_enableDTR(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], SETDTR);
}


void RS232_disableDTR(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], CLRDTR);
}


void RS232_enableRTS(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], SETRTS);
}


void RS232_disableRTS(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], CLRRTS);
}

/*
https://msdn.microsoft.com/en-us/library/windows/desktop/aa363428%28v=vs.85%29.aspx
*/

void RS232_flushRX(int comport_number)
{
  PurgeComm(Cport[comport_number], PURGE_RXCLEAR | PURGE_RXABORT);
}

void RS232_my_flushRX(int comport_number)
{
  PurgeComm(Cport[comport_number], PURGE_RXCLEAR);
}

void RS232_flushTX(int comport_number)
{
  PurgeComm(Cport[comport_number], PURGE_TXCLEAR | PURGE_TXABORT);
}

void RS232_my_flushTX(int comport_number)
{
  PurgeComm(Cport[comport_number], PURGE_TXCLEAR );
}

void RS232_flushRXTX(int comport_number)
{
  PurgeComm(Cport[comport_number], PURGE_RXCLEAR | PURGE_RXABORT);
  PurgeComm(Cport[comport_number], PURGE_TXCLEAR | PURGE_TXABORT);
}


#endif


void RS232_cputs(int comport_number, const char *text)  /* sends a string to serial port */
{
  while(*text != 0)   RS232_SendByte(comport_number, *(text++));
}


/* return index in comports matching to device name or -1 if not found */
int RS232_GetPortnr(const char *devname)
{
  int i;

  char str[32];

#if defined(__linux__) || defined(__FreeBSD__)   /* Linux & FreeBSD */
  strcpy(str, "/dev/");
#else  /* windows */
  strcpy(str, "\\\\.\\");
#endif
  strncat(str, devname, 16);
  str[31] = 0;

  for(i=0; i<RS232_PORTNR; i++)
  {
    if(!strcmp(comports[i], str))
    {
      return i;
    }
  }

  return -1;  /* device not found */
}

int RS232_OpenComport_2_0(int comport_number, int baudrate, const char *mode, int flowctrl, int * flag_ptr)
{
  

  printf("DEBUG : OpenComport (all'inizio) : comport_number passato : %d\n", comport_number);
  
  if((comport_number>=RS232_PORTNR)||(comport_number<0))
  {
    printf("illegal comport number\n");
    return(-1);
  }

  switch(baudrate)
  {
    case     110 : strcpy(mode_str, "baud=110");
                   break;
    case     300 : strcpy(mode_str, "baud=300");
                   break;
    case     600 : strcpy(mode_str, "baud=600");
                   break;
    case    1200 : strcpy(mode_str, "baud=1200");
                   break;
    case    2400 : strcpy(mode_str, "baud=2400");
                   break;
    case    4800 : strcpy(mode_str, "baud=4800");
                   break;
    case    9600 : strcpy(mode_str, "baud=9600");
                   break;
    case   19200 : strcpy(mode_str, "baud=19200");
                   break;
    case   38400 : strcpy(mode_str, "baud=38400");
                   break;
    case   57600 : strcpy(mode_str, "baud=57600");
                   break;
    case  115200 : strcpy(mode_str, "baud=115200");
                   break;
    case  128000 : strcpy(mode_str, "baud=128000");
                   break;
    case  256000 : strcpy(mode_str, "baud=256000");
                   break;
    case  500000 : strcpy(mode_str, "baud=500000");
                   break;
    case  921600 : strcpy(mode_str, "baud=921600");
                   break;
    case 1000000 : strcpy(mode_str, "baud=1000000");
                   break;
    case 1500000 : strcpy(mode_str, "baud=1500000");
                   break;
    case 2000000 : strcpy(mode_str, "baud=2000000");
                   break;
    case 3000000 : strcpy(mode_str, "baud=3000000");
                   break;
    default      : printf("invalid baudrate\n");
                   return(-1);
                   break;
  }

  if(strlen(mode) != 3)
  {
    printf("invalid mode \"%s\"\n", mode);
    return(-1);
  }

  switch(mode[0])
  {
    case '8': strcat(mode_str, " data=8");
              break;
    case '7': strcat(mode_str, " data=7");
              break;
    case '6': strcat(mode_str, " data=6");
              break;
    case '5': strcat(mode_str, " data=5");
              break;
    default : printf("invalid number of data-bits '%c'\n", mode[0]);
              return(-1);
              break;
  }

  switch(mode[1])
  {
    case 'N':
    case 'n': strcat(mode_str, " parity=n");
              break;
    case 'E':
    case 'e': strcat(mode_str, " parity=e");
              break;
    case 'O':
    case 'o': strcat(mode_str, " parity=o");
              break;
    default : printf("invalid parity '%c'\n", mode[1]);
              return(-1);
              break;
  }

  switch(mode[2])
  {
    case '1': strcat(mode_str, " stop=1");
              break;
    case '2': strcat(mode_str, " stop=2");
              break;
    default : printf("invalid number of stop bits '%c'\n", mode[2]);
              return(-1);
              break;
  }

  if(flowctrl)
  {
    strcat(mode_str, " xon=off to=off odsr=off dtr=on rts=off");
  }
  else
  {
    strcat(mode_str, " xon=off to=off odsr=off dtr=on rts=on");
  }

/*
http://msdn.microsoft.com/en-us/library/windows/desktop/aa363145%28v=vs.85%29.aspx

http://technet.microsoft.com/en-us/library/cc732236.aspx

https://docs.microsoft.com/en-us/windows/desktop/api/winbase/ns-winbase-_dcb
*/
   //AGGIUNTA
	RS_printCommInfo(comports[4]);
  //INIZIO MODIFICA NOSTRA
  printf("DEBUG : OpenComport INIZIO MODIFICA NOSTRA\n");
  
  int flag = 0;
  while( flag == 0 && comport_number < 32) {
   	printf("DEBUG : OpenComport ADESSO PROVO comport_number : %d\n", comport_number);

	Cport[comport_number] = CreateFileA(comports[comport_number],
						  GENERIC_READ|GENERIC_WRITE,
						  0,                          /* no share  */
						  NULL,                       /* no security */
						  OPEN_EXISTING,
						  0,                          /* no threads */
						  NULL);                      /* no templates */
	
	printf("DEBUG : OpenComport PROVA FINITA\n");

	if(Cport[comport_number]==INVALID_HANDLE_VALUE)
	{
	  printf("OpenComport ERRORE : unable to open comport number : %d\n", (comport_number+1));
	  comport_number++;
	}
	else {
	  printf("OpenComport TROVATA : opened comport number : %d\n", (comport_number+1));
	  
	  flag = Identify(comport_number);
	  if( flag == 0 ) {
		comport_number++;
	  }
	  else {
		*flag_ptr = flag;
	  }
	}
  }

  if( flag == 0 ) {
	printf("OpenComport ERRORE : unable to open comport. FINE\n");
    return(-1);
  }
  printf("OpenComport SUCCESSO : comport %d opened\n", (comport_number+1));
  //FINE MODIFICA NOSTRA


  DCB port_settings;
  memset(&port_settings, 0, sizeof(port_settings));  /* clear the new struct  */
  port_settings.DCBlength = sizeof(port_settings);

  if(!BuildCommDCBA(mode_str, &port_settings))
  {
    printf("unable to set comport dcb settings\n");
    CloseHandle(Cport[comport_number]);
    return(-1);
  }

  if(flowctrl)
  {
    port_settings.fOutxCtsFlow = TRUE;
    port_settings.fRtsControl = RTS_CONTROL_HANDSHAKE;
  }

  if(!SetCommState(Cport[comport_number], &port_settings))
  {
    printf("unable to set comport cfg settings\n");
    CloseHandle(Cport[comport_number]);
    return(-1);
  }

  COMMTIMEOUTS Cptimeouts;

  Cptimeouts.ReadIntervalTimeout         = MAXDWORD;
  Cptimeouts.ReadTotalTimeoutMultiplier  = 0;
  Cptimeouts.ReadTotalTimeoutConstant    = 0;
  Cptimeouts.WriteTotalTimeoutMultiplier = 0;
  Cptimeouts.WriteTotalTimeoutConstant   = 0;

  if(!SetCommTimeouts(Cport[comport_number], &Cptimeouts))
  {
    printf("unable to set comport time-out settings\n");
    CloseHandle(Cport[comport_number]);
    return(-1);
  }
  
  

  return(comport_number);
}

int Identify( int cport_nr ) {

	int n = 0;
	unsigned char zyboAck[2] = {0,0};
	
	int cont = 0;
	int flag = 0;
	
	while( cont < 10 && flag == 0 ) {
				
		Sleep(500);
		RS232_SendByte(cport_nr, '0');
		RS232_flushTX(cport_nr);

		while (n == 0){
			printf("in attesa di risposta...\n");
			n = RS232_PollComport(cport_nr, zyboAck, 2);
			RS232_flushRX(cport_nr);
		}
		if (n > 0) {
			printf("received %i bytes:\n message received: %.2s\n", n, ( char *)zyboAck);
			if( strstr( (char *)zyboAck, "OK" ) ) {
				flag = 1;
			}
		}
		cont++;
	}
	
	return flag;
}

void RS_printCommInfo(const char *comXX)
{
 BOOL rc;
 HANDLE com_handle;
 COMMPROP CommProp;

 printf("%s:",comXX);
 
 com_handle = CreateFile(
  comXX,         /* シリアルポートの文字列 */
  GENERIC_READ | GENERIC_WRITE   ,   /* アクセスモード */
  0,
  NULL,          /* セキュリティ属性 */
  OPEN_EXISTING,        /* 作成フラグ */
  0,           /* 属性 */
  NULL          /* テンプレートのハンドル */
 );
 if( com_handle == INVALID_HANDLE_VALUE) {
  printf(" Can't Open .");
 }else{
  rc = GetCommProperties(com_handle,&CommProp);
  if(rc) {
#if 0
  DWORD dwMaxTxQueue；   // バイト単位の最大送信バッファサイズ
  DWORD dwMaxRxQueue；   // バイト単位の最大受信バッファサイズ
  DWORD dwMaxBaud；      // ボーレート最大値
  DWORD dwProvSubType；   // 特定プロバイダータイプ
  DWORD dwProvCapabilities； // サポートされた関数
#endif
   printf("MaxTxQue=%ld,MaxRxQue=%ld,MaxBaud=%lx"
    ,CommProp.dwMaxTxQueue
    ,CommProp.dwMaxRxQueue
    ,CommProp.dwMaxBaud);
  //}

   printf("CurrentTxQue=%ld,CurrentRxQue=%ld,PacketLength=%d"
    ,CommProp.dwCurrentTxQueue
    ,CommProp.dwCurrentRxQueue  //4096
    ,CommProp.wPacketLength);
  }
	printf("\n");

#if 0
  static DCB com_dcb;
  memset(&com_dcb,0,sizeof(com_dcb));

  com_dcb.DCBlength = sizeof(DCB);
  rc = GetCommState(com_handle,&com_dcb);
//  if(rc) 
  {
//  com_dcb.BaudRate = baudrate;
//  com_dcb.ByteSize = 8;
//  com_dcb.Parity   = NOPARITY;
//  com_dcb.StopBits = ONESTOPBIT;
   printf("Baud=%ld bits=%d parity=%d stop=%d"
    ,com_dcb.BaudRate 
    ,com_dcb.ByteSize 
    ,com_dcb.Parity   
    ,com_dcb.StopBits 
   );
  }
#endif
  CloseHandle(com_handle);

 }
 printf(" \n");
 
 //Maggiori info qui:
 //https://cpp.hotexamples.com/it/examples/-/-/GetCommProperties/cpp-getcommproperties-function-examples.html
 //https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-commprop
 //https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcommproperties
 
}
//Controlla se ci sono errori di frame, overrun , parità ecc.
void MySerialStatus(int cport_nr) {    

    //HANDLE Cport[comport_number];    //prendere da OpenComport
    DWORD dwErrorFlags;
    COMSTAT ComStat;
	//printf("Sto nella MySerialStatus\n");
    if( ClearCommError( Cport[cport_nr], &dwErrorFlags, &ComStat ) ) {
		//printf("Sono nell'if della MySerialStatus\n");
        if(dwErrorFlags & CE_BREAK) printf("\nC'è stato un BREAK ERROR!\n");
        if(dwErrorFlags & CE_FRAME) printf("\nC'è stato un FRAME ERROR!\n");
        if(dwErrorFlags & CE_OVERRUN) printf("\nC'è stato un OVERRUN ERROR!\n");
        if(dwErrorFlags & CE_RXPARITY) printf("\nC'è stato un RXPARITY ERROR!\n");
        //*(wxSerialPort_EINFO*)args = einfo;
    }
}







