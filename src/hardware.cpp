/*********************************************************************
* Lattice Semiconductor Corp. Copyright 2011
* hardware.cpp
* Version 4.0
*
* This file contains interface between SSPI Embedded and the hardware.
* Depending on hardware availability, implementation may be different.
* Therefore the user is responsible to modify functions here in order
*	to use SSPI Embedded engine to drive the hardware.
***********************************************************************/
#include <stdio.h>
#include "usb_hw.h"
#include "intrface.h"
#include "opcode.h"
#include "debug.h"
#include "hardware.h"

/***********************************************************************
*
* Global variables.
*
***********************************************************************/
unsigned int a_uiCheckFailedRow = 0;
unsigned int a_uiRowCount		= 0;
int a_uiDebug					= 0;
//*====================================================================
//*
//* Debug utility functions
//*
//*====================================================================
unsigned char dataBuffer[1024];

//*********************************************************************
//* There are 2 functions here:
//*
//* dbgu_init() -	this function is responsible to initialize debug 
//*					functionality.
//*
//* dbgu_putint() -	this function take 2 integers from SSPI Embedded
//*					processing system.  It is up to the user to take
//*					advantage of these integers.
//*********************************************************************

//*--------------------------------------------------------------------
//* Function dbgu_init()
//* Purpose: Initialize debugging unit.
//*
//* Return:		1 - succeed
//*				0 - fail
//*
//* If you don't want to use the debug option, you don't need to 
//* implement this function.
//*
//*--------------------------------------------------------------------

//*********************************************************************
//* here you may implement debug initializing function.  
//*********************************************************************

int dbgu_init()
{
	if(a_uiDebug)
		printf("Debug mode enable.\n");
	return 1;
}

//*--------------------------------------------------------------------
//* Function dbgu_putint(int debugCode, int debugCode2)
//* Purpose: Return 2 integers from the core for user to run debug.
//*
//* If you don't want to use the debug option, you don't need to 
//* implement this function.
//*
//* 0x[debugCode][debugCode2] forms a char number that will map to
//* a constant string.  You may use these strings to implement flexible
//* debugging option
//*--------------------------------------------------------------------

//*********************************************************************
//* here you may implement debug function.  
//*********************************************************************

void dbgu_putint(int debugCode, int debugCode2){

}

//*====================================================================
//*
//* Debug utility functions
//*
//*====================================================================

//*********************************************************************
//* The following functions may require user modification:
//*
//* SPI_init() -	this function is responsible to initialize SSPI 
//*					port.
//*
//* SPI_final() -	this function is responsible to turn off SSPI port.
//*
//* wait() -		this function take the number of millisecond and
//*					wait for the time specified.
//*
//* TRANS_transmitBytes() -
//*					this function is responsible to transmit data over
//*					SSPI port.
//*
//* TRANS_receiveBytes() -
//*					this function is responsible to receive data 
//*					through SSPI port.
//*
//* TRANS_starttranx() -
//*					this function initiates a transmission by pulling
//*					chip-select low.
//*
//* TRANS_endtranx() -
//*					this function terminates the transmission by pulling
//*					chip-select high.
//*
//* TRANS_cstoggle() -
//*					this function pulls chip-select low, then pulls it
//*					high.
//*
//* TRANS_trsttoggle() -
//*					this function pulls CRESET low or high.
//* TRANS_runClk() -
//*					this function is responsible to drive at least
//*					3 extra clocks after chip-select is pulled high.
//* 
//*********************************************************************

//*--------------------------------------------------------------------
//* Function SPI_init()
//* Purpose: Initialize SPI port
//* 
//* Return:		1 - succeed
//*				0 - fail
//*
//* If you already initialize the SPI port in your embedded system,
//* simply make the function to return 1.
//*--------------------------------------------------------------------

//*********************************************************************
//* here you may implement SSPI initialization functions.
//*********************************************************************
int SPI_init()
{
	if(a_uiDebug)
		printf("Initialize SPI...\n");	
	return 1;
}


//*--------------------------------------------------------------------
//* Function SPI_final()
//* Purpose: Finalize SPI port
//* 
//* Return:		1 - succeed
//*				0 - fail
//*
//* If you plan to leave the SPI port on, or it is managed in your 
//* embedded system, simply make the function return 1.
//*--------------------------------------------------------------------

//*********************************************************************
//* here you may implement SSPI disable functions.
//*********************************************************************

int SPI_final(){
	if(a_uiDebug)
		printf("Finalize SPI...\n");
	return 1;
}

//*--------------------------------------------------------------------
//* Function wait(int ms)
//* Purpose: Hold the process for some time (unit millisecond)
//*--------------------------------------------------------------------

//*********************************************************************
//* here you should implement wait function that wait in unit of 
//* millisecond, which is specified in variable ms
//*********************************************************************

int wait(int ms)
{
	if(a_uiDebug)
		printf("wait for %d milliseconds.\n", ms);
	while(ms > 0x8000){
		ispVMDelay(0x7FFF + 0x8000);
		ms -= 0x7FFF;
	}
	ispVMDelay( (unsigned short)(ms + 0x8000) );	
	return 1;
}

//*--------------------------------------------------------------------
//* Function TRANS_transmitBytes(unsigned char *trBuffer, int trCount)
//* Purpose: To transmit certain number of bits, indicating by trCount,
//* over SPI port.
//*
//* Data for transmission is stored in trBuffer.
//* trCount indicates number of bits to be transmitted.  It should be 
//* divisible by 8.  If it is not divisible by 8, pad the data with 1's.
//* The function returns 1 if success, or 0 if fail.
//*--------------------------------------------------------------------

//*********************************************************************
//* here you may implement transmitByte function
//*********************************************************************

int TRANS_transmitBytes(unsigned char *trBuffer, int trCount)
{
	if(fnUSBPumpData(trBuffer, NULL, trCount, 0, 1) < 0)
		return PROC_FAIL;
	else
		return PROC_COMPLETE;
}

//*--------------------------------------------------------------------
//* Function TRANS_receiveBytes(unsigned char *rcBuffer, int rcCount)
//* Purpose: To receive certain number of bits, indicating by rcCount,
//* over SPI port.
//*
//* Data received can be stored in rcBuffer.
//* rcCount indicates number of bits to receive.  It should be 
//* divisible by 8.  If it is not divisible by 8, pad the data with 1's.
//* The function returns 1 if success, or 0 if fail.
//*--------------------------------------------------------------------

//*********************************************************************
//* here you may implement transmitByte function
//*********************************************************************

int TRANS_receiveBytes(unsigned char *rcBuffer, int rcCount){
	if(fnUSBPumpData(NULL, rcBuffer, rcCount, 0, 2) < 0)
		return PROC_FAIL;
	else
		return PROC_COMPLETE;
}
//*--------------------------------------------------------------------
//* Function TRANS_starttranx(unsigned char channel)
//* Purpose: To start an SPI transmission
//*
//* Return:		1 - succeed
//*				0 - fail
//*
//* This function is responsible to pull chip select low.
//* If your embedded system has a dedicated SPI port that does not require
//* manually pulling chip select low, simply return 1.
//*--------------------------------------------------------------------

//*********************************************************************
//* here you should implement starting SPI transmission.
//*********************************************************************	
int TRANS_starttranx(unsigned char channel)
{
	if(a_uiDebug)
		printf("Start of transmission on Channel 0x%x.\n", channel);
	writeCS( 0x00 );
	return 1;
}

//*--------------------------------------------------------------------
//* Function TRANS_endtranx()
//* Purpose: To end an SPI transmission
//*
//* Return:		1 - succeed
//*				0 - fail
//*
//* If your embedded system has a dedicated SPI port that does not require
//* implementing this function, simply return 1.
//*--------------------------------------------------------------------

//*********************************************************************
//* here you should implement ending SPI transmission.
//*********************************************************************
int TRANS_endtranx()
{
	if(a_uiDebug)
		printf("End of transmission.\n");
	writeCS( 0x01 );
	return 1;
}
//************************************************************************
//* Function TRANS_trsttoggle(unsigned char toggle)
//* Purpose: To toggle CRESET (TRST) signal
//*
//* Return:		1 - succeed
//*				0 - fail
//*
//*************************************************************************/
int TRANS_trsttoggle(unsigned char toggle)
{
	if(a_uiDebug)
		printf("Toggle CRESET %d.\n", toggle);
	writeTRST( toggle );
	return 1;
}
//*--------------------------------------------------------------------
//* Function TRANS_cstoggle(unsigned char channel)
//* Purpose: To toggle chip select (CS) of specific channel
//*
//* Return:		1 - succeed
//*				0 - fail
//*
//* If your embedded system has a dedicated SPI port that does not 
//* allow bit banging, simply transmit a byte of 0xFF to the device,
//* and the device will ignore that.
//*--------------------------------------------------------------------
int TRANS_cstoggle(unsigned char channel)
{
	if(channel != 0x00)
		return 0;
	else
	{
	//*********************************************************************
	//* here you should implement toggling CS.
	//*
	//* Currently it prints message on screen and in log file.
	//*********************************************************************
		if(a_uiDebug)
			printf("Toggle chip select.\n", channel);
  		writeCS( 0x01 );
  		writeCS( 0x00 );
  		writeCS( 0x01 );
		return 1;
	}
}
//*--------------------------------------------------------------------
//* Function TRANS_runClk(int clk)
//* Purpose: To drive extra clock.
//*
//* Return:		1 - succeed
//*				0 - fail
//*
//* If your embedded system has a dedicated SPI port that does not 
//* allow bit banging, simply transmit a byte of 0xFF on another channel
//* that is not being used, so the device will only see the clock.
//*--------------------------------------------------------------------

//*********************************************************************
//* here you should implement running free clock
//*********************************************************************
int TRANS_runClk(int clk)
{
	if(a_uiDebug)
		printf("Run extra %d clocks.\n",clk);
  	runClocks( clk );
  	return 1;
}


//*--------------------------------------------------------------------
//* Function TRANS_transceive_stream(int trCount, unsigned char *trBuffer, 
//* 					int trCount2, int flag, unsigned char *trBuffer2
//* Purpose: Transmits opcode and transceive data
//*
//* It will have the following operations depending on the flag:
//*
//*		NO_DATA: end of transmission.  trCount2 and trBuffer2 are discarded
//*		BUFFER_TX: transmit data from trBuffer2
//*		BUFFER_RX: receive data and compare it with trBuffer2
//*		DATA_TX: transmit data from external source
//*		DATA_RX: receive data and compare it with data from external source
//*
//* If the data is not byte bounded and your SPI port only transmit/ receive
//* byte bounded data, you need to have padding to make it byte-bounded.
//* If you are transmit non-byte-bounded data, put the padding at the beginning
//* of the data.  If you are receiving data, do not compare the padding,
//* which is at the end of the transfer.
//*--------------------------------------------------------------------

#define NO_DATA		0
#define BUFFER_TX	1
#define BUFFER_RX	2
#define DATA_TX		3
#define DATA_RX		4

int TRANS_transceive_stream(int trCount, unsigned char *trBuffer, 
							int trCount2, int flag, unsigned char *trBuffer2,
							int mask_flag, unsigned char *maskBuffer)
{
	int i                         = 0;
	unsigned short int tranxByte  = 0;
	unsigned char trByte          = 0;
	unsigned char dataByte        = 0;
	int mismatch                  = 0;
	unsigned char dataID          = 0;
	if(trCount > 0)
	{
		// calculate # of bytes being transmitted
		tranxByte = (unsigned short) (trCount / 8);
		if(trCount % 8 != 0){
			tranxByte ++;
			trCount += (8 - (trCount % 8));
		}

		for (i=0; i<tranxByte; i++){
			if(a_uiDebug)
				printf("transmit 1 byte of data 0x%0.2x.\n", trBuffer[i]);
		}
		if( !TRANS_transmitBytes(trBuffer, trCount) )
			return ERROR_PROC_HARDWARE;
	}
	switch(flag){
	case NO_DATA:
		return 1;
		break;
	case BUFFER_TX:
		tranxByte = (unsigned short) (trCount2 / 8);
		if(trCount2 % 8 != 0){
			tranxByte ++;
			trCount2 += (8 - (trCount2 % 8));
		}
		for (i=0; i<tranxByte; i++){
			if(a_uiDebug)
				printf("transmit 1 byte of data 0x%0.2x.\n", trBuffer2[i]);
		}	
		if(!TRANS_transmitBytes(trBuffer2, trCount2) )
			return ERROR_PROC_HARDWARE;
		return 1;
		break;

	case BUFFER_RX:
		tranxByte = (unsigned short)(trCount2 / 8);
		if(trCount2 % 8 != 0){
			tranxByte ++;
			trCount2 += (8 - (trCount2 % 8));
		}

		if( !TRANS_receiveBytes(trBuffer2, trCount2) )
			return ERROR_PROC_HARDWARE;

		return 1; 
		break;
	case DATA_TX:
		tranxByte = (unsigned short)((trCount2 + 7) / 8);
		if(trCount2 % 8 != 0){
			trByte = (unsigned char)(0xFF << (trCount2 % 8));
		}
		else
			trByte = 0;

		if(trBuffer2 != 0)
			dataID = *trBuffer2;
		else
			dataID = 0x04;

		for (i=0; i<tranxByte; i++){
			if(i == 0){
				if( !HLDataGetByte(dataID, &dataByte, trCount2) )
					return ERROR_INIT_DATA;
			}
			else{
				if( !HLDataGetByte(dataID, &dataByte, 0) )
					return ERROR_INIT_DATA;
			}
			if(trCount2 % 8 != 0)
				trByte += (unsigned char) (dataByte >> (8- (trCount2 % 8)));
			else
				trByte = dataByte;

			dataBuffer[i] = trByte;
			if(a_uiDebug)
				printf("transmit 1 byte of data 0x%0.2x.\n", dataBuffer[i]);		
			// do not remove the line below!  It handles the padding for 
			// non-byte-bounded data
			if(trCount2 % 8 != 0)
				trByte = (unsigned char)(dataByte << (trCount2 % 8));
		}
		if(trCount2 % 8 != 0){
			trCount2 += (8 - (trCount2 % 8));
		}
		if(!TRANS_transmitBytes(dataBuffer, trCount2))
			return ERROR_PROC_HARDWARE;
		return 1;
		break;
	case DATA_RX:
		tranxByte = (unsigned short)(trCount2 / 8);
		if(trCount2 % 8 != 0){
			tranxByte ++;
		}
		if(trBuffer2 != 0)
			dataID = *trBuffer2;
		else
			dataID = 0x04;
		if(!TRANS_receiveBytes(dataBuffer, (tranxByte * 8) ))
			return ERROR_PROC_HARDWARE;
		for(i=0; i<tranxByte; i++){
			if(i == 0){
				if( !HLDataGetByte(dataID, &dataByte, trCount2) )
					return ERROR_INIT_DATA;
			}
			else{
				if( !HLDataGetByte(dataID, &dataByte, 0) )
					return ERROR_INIT_DATA;
			}

			trByte = dataBuffer[i];
			if(mask_flag)
			{
				trByte = trByte & maskBuffer[i];
				dataByte = dataByte & maskBuffer[i];
			}
			if(a_uiDebug)
				printf("receive 1 byte of data 0x%0.2x.\n", trByte);
			if(i == tranxByte - 1){
				trByte = (unsigned char)(trByte ^ dataByte) & 
					(unsigned char)(0xFF << (8 - (trCount2 % 8)));
			}
			else
				trByte = (unsigned char)(trByte ^ dataByte);

			if(trByte)
				mismatch ++;
		}
		if(mismatch == 0)
		{
			if(a_uiCheckFailedRow)
			{
				a_uiRowCount++;
			}
			return 1;
		}
		else{
			if(a_uiDebug)
				printf("Number of mismatch: %d.\n", mismatch);
			if(dataID == 0x01 && a_uiRowCount == 0)
			{
				return ERROR_IDCODE;
			}
			else if(dataID == 0x05)
			{
				return ERROR_USERCODE;
			}
			else if(dataID == 0x06)
			{
				return ERROR_SED;
			}
			else if(dataID == 0x07)
			{
				return ERROR_TAG;
			}
			return ERROR_VERIFICATION;
		}
		break;
	default:
		return ERROR_INIT_ALGO;
	}
}