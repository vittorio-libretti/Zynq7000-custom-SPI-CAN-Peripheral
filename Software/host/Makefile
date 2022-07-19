#
#
# Author: Teunis van Beelen
#
# email: teuniz@protonmail.com
#
#

CC = gcc
CFLAGS = -Wall -Wextra -o2

objects = rs232.o analysis.o

all: host

host : $(objects) host.o 
	$(CC) $(objects) host.o -o host

host.o : host.c rs232.h
	$(CC) $(CFLAGS) -c host.c -o host.o

analysis.o : analysis.c analysis.h
	$(CC) $(CFLAGS) -c analysis.c -o analysis.o

rs232.o : rs232.h rs232.c
	$(CC) $(CFLAGS) -c rs232.c -o rs232.o

clean :
	$(RM) host $(objects) host.o 

#
#
#
#





