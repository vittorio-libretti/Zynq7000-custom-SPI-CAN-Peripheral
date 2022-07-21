# Zynq7000-custom-SPI-CAN-Peripheral

# Project Overview
This is a project written in C language for university purposes that use also FreeRTOS. It was created in Vivado/Vitis Xilinx suite. 
It allows you to use a Zybo device to receive CAN messages from a CAN acquisition system and possibly forward them to an host system with Windows OS. 
A real custom asynchronous communication protocol between host and device has been specifically designed for it.


# File Organization
The resources provided are divided into 3 folders:
 - Host;
 - project_from_scratch;
 - SPI_Hardware_Design_Z700.

Folder Host contains the source code for the host: 	
 - host.c: contains the core logic of the program, that is the main function and a switch-case instruction depending on the command diven by the user;
 - analysis.h/analysis.c: high level library with utility functions for the main program, such as the function that analyzes of the messages received;
 - rs232.h/rs232.c: low level library for Linux or Windows for the serial communication via RS232 standard using UART. This library has been modified for the purpose of the project.


Folder project_from_scratch cntains the Vitis project for the Zybo device:
 - main.c: contains the main function in witch defining and initializing the two task managed with FreeRTOS,them starts the scheduler. 
           the two tasks regulate decouple between the CAN message and Host request.
           In particular: the "talkWithSPICAN_Task" receive the CAN mex and store it in the FreeRTOS circular queue.
                          the "talkWithHost_Task" responds to Host requests
 - Devicelib.h/Devicelib.c: high level library with utility functions for the main program, such as the functions that take messages from the custom peripheral SPI Controller;
 - SPI.h/SPI.c: low level library to interact with the custom peripheral SPI Controller, as well as a function to boot the Pmod CAN module.


Folder SPI_Hardware_Design_Z700 contains the hardware platform on which to develop the software on Zybo device, such as the BSP level. This folder must be imported into Vitis IDE. Also contains the bitstream file (subfolder /bitstream/SPI_Hardware_Design.bit) and the XSA file (/hw/SPI_Hardware_Design_Z700.xsa).
