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
#if __linux__
#include <unistd.h>	// usleep
#include <stdio.h>	// sprintf

#endif

#include "intrface.h"
#include "opcode.h"
#include "debug.h"
#include "hardware.h"

#include "reedl-ictrl-cmd.h"

extern void print_out_string(char *stringOut);
#define ERR_LOG(x) print_out_string(x)
int trCount_max = 0;
int rxCount_max = 0;
int runclk_max = 0;
extern char g_Message[512];

static int cs_state;

/***********************************************************************
*
* Global variables.
*
***********************************************************************/
unsigned int a_uiCheckFailedRow = 0;
unsigned int a_uiRowCount		= 0;

/***********************************************************************
*
* Debug utility functions
*
***********************************************************************/

/**********************************************************************
* There are 2 functions here:
*
* dbgu_init() -	this function is responsible to initialize debug 
*					functionality.
*
* dbgu_putint() -	this function take 2 integers from SSPI Embedded
*					processing system.  It is up to the user to take
*					advantage of these integers.
***********************************************************************/

/***********************************************************************
* Function dbgu_init()
* Purpose: Initialize debugging unit.
*
* Return:		1 - succeed
*				0 - fail
*
* If you don't want to use the debug option, you don't need to 
* implement this function.
*
************************************************************************/

/*********************************************************************
* here you may implement debug initializing function.  
**********************************************************************/
int dbgu_init()
{
	return 1;
}

/***********************************************************************
* Function dbgu_putint(int debugCode, int debugCode2)
* Purpose: Return 2 integers from the core for user to run debug.
*
* If you don't want to use the debug option, you don't need to 
* implement this function.
*
* 0x[debugCode][debugCode2] forms a char number that will map to
* a constant string.  You may use these strings to implement flexible
* debugging option
************************************************************************/

/***********************************************************************
* here you may implement debug function.  
************************************************************************/
void dbgu_putint(int debugCode, int debugCode2)
{

}

/***********************************************************************
*
* Debug utility functions
*
************************************************************************/

/***********************************************************************
* The following functions may require user modification:
*
* SPI_init() -	this function is responsible to initialize SSPI 
*					port.
*
* SPI_final() -	this function is responsible to turn off SSPI port.
*
* wait() -		this function take the number of millisecond and
*					wait for the time specified.
*
* TRANS_transmitBytes() -
*					this function is responsible to transmit data over
*					SSPI port.
*
* TRANS_receiveBytes() -
*					this function is responsible to receive data 
*					through SSPI port.
*
* TRANS_starttranx() -
*					this function initiates a transmission by pulling
*					chip-select low.
*
* TRANS_endtranx() -
*					this function terminates the transmission by pulling
*					chip-select high.
*
* TRANS_cstoggle() -
*					this function pulls chip-select low, then pulls it
*					high.
* TRANS_trsttoggle() -
*					this function pulls CRESET low or high.
* TRANS_runClk() -
*					this function is responsible to drive extra clocks 
*					after chip-select is pulled high.
* 
*
* In order to use stream transmission, dataBuffer[] is required to 
* buffer the data.  Please refer to devices' specification for the
* number of bytes required.  For XP2-40, minimum is 423 bytes.
* Declare a little bit more than the minimum, just to be safe.
*
*
************************************************************************/
unsigned char dataBuffer[1024];
/************************************************************************
* Function SPI_init()
* Purpose: Initialize SPI port
* 
* Return:		1 - succeed
*				0 - fail
*
* If you already initialize the SPI port in your embedded system,
* simply make the function to return 1.
************************************************************************/

/************************************************************************
* here you may implement SSPI initialization functions.
************************************************************************/
int SPI_init()
{
	int rc;
	cs_state = 1;		// By default CS is in inactive state
	rc = reedl_ictrl_sspiem_init(1);		
	if (rc) {
		ERR_LOG("Can't init sspiem\n");
	}
	return !rc;
}
/************************************************************************
* Function SPI_final()
* Purpose: Finalize SPI port
* 
* Return:		1 - succeed
*				0 - fail
*
* If you plan to leave the SPI port on, or it is managed in your 
* embedded system, simply make the function return 1.
************************************************************************/

/************************************************************************
* here you may implement SSPI disable functions.
************************************************************************/
int SPI_final()
{
	int rc;
	ERR_LOG("=*= Final \n");
	rc = reedl_ictrl_sspiem_init(0);	// Try to restore FPGA back to normal operation

	return 1;
}

/************************************************************************
* Function wait(int ms)
* Purpose: Hold the process for some time (unit millisecond)
* Users must implement a delay to observe a_usTimeDelay, where
* bit 15 of the a_usTimeDelay defines the unit.
*      1 = milliseconds
*      0 = microseconds
* Example:
*      a_usTimeDelay = 0x0001 = 1 microsecond delay.
*      a_usTimeDelay = 0x8001 = 1 millisecond delay.
*
* This subroutine is called upon to provide a delay from 1 millisecond to a few 
* hundreds milliseconds each time. 
* It is understood that due to a_usTimeDelay is defined as unsigned short, a 16 bits
* integer, this function is restricted to produce a delay to 64000 micro-seconds 
* or 32000 milli-second maximum. The VME file will never pass on to this function
* a delay time > those maximum number. If it needs more than those maximum, the VME
* file will launch the delay function several times to realize a larger delay time
* cummulatively.
* It is perfectly alright to provide a longer delay than required. It is not 
* acceptable if the delay is shorter.
*
* Delay function example--using the machine clock signal of the native CPU------
* When porting ispVME to a native CPU environment, the speed of CPU or 
* the system clock that drives the CPU is usually known. 
* The speed or the time it takes for the native CPU to execute one for loop 
* then can be calculated as follows:
*       The for loop usually is compiled into the ASSEMBLY code as shown below:
*       LOOP: DEC RA;
*             JNZ LOOP;
*       If each line of assembly code needs 4 machine cycles to execute, 
*       the total number of machine cycles to execute the loop is 2 x 4 = 8.
*       Usually system clock = machine clock (the internal CPU clock). 
*       Note: Some CPU has a clock multiplier to double the system clock for 
*              the machine clock.
*
*       Let the machine clock frequency of the CPU be F, or 1 machine cycle = 1/F.
*       The time it takes to execute one for loop = (1/F ) x 8.
*       Or one micro-second = F(MHz)/8;
*
* Example: The CPU internal clock is set to 100Mhz, then one micro-second = 100/8 = 12
*
* The C code shown below can be used to create the millisecond accuracy. 
* Users only need to enter the speed of the cpu.
*
************************************************************************/
int wait(int a_msTimeDelay)
{
#if __linux__
	a_msTimeDelay = 1;	// AV: 10ms requested actualy -> 40. It is too much
	usleep(a_msTimeDelay * 1000);

#endif
#if 0
	unsigned short loop_index     = 0;
	unsigned short ms_index       = 0;
	unsigned short us_index       = 0;
	unsigned short g_usCpu_Frequency  = 1000;   /*Enter your CPU frequency here, unit in MHz.*/
	
	/*Users can replace the following section of code by their own*/
	for( ms_index = 0; ms_index < a_msTimeDelay; ms_index++)
	{
		/*Loop 1000 times to produce the milliseconds delay*/
		for (us_index = 0; us_index < 1000; us_index++)
		{ /*each loop should delay for 1 microsecond or more.*/
			loop_index = 0;
			do {
				/*The NOP fakes the optimizer out so that it doesn't toss out the loop code entirely*/
				__asm NOP
			}while (loop_index++ < ((g_usCpu_Frequency/8)+(+ ((g_usCpu_Frequency % 8) ? 1 : 0))));/*use do loop to force at least one loop*/
		}
	}
#endif
	sprintf(g_Message, "=*=    wait (%d)\n", a_msTimeDelay);
	ERR_LOG(g_Message);

	return 1;
}

/************************************************************************
* Function TRANS_transmitBytes(unsigned char *trBuffer, int trCount)
* Purpose: To transmit certain number of bits, indicating by trCount,
* over SPI port.
*
* Data for transmission is stored in trBuffer.
* trCount indicates number of bits to be transmitted.  It should be 
* divisible by 8.  If it is not divisible by 8, pad the data with 1's.
* The function returns 1 if success, or 0 if fail.
************************************************************************/

/************************************************************************
* here you may implement transmitByte function
************************************************************************/
int TRANS_transmitBytes(unsigned char *trBuffer, int trCount)
{
	int rc;
	trCount = trCount >> 3;
	if (trCount_max < trCount) trCount_max = trCount;

	int idx = sprintf(g_Message, "=====*= TX %d (%d): ", trCount, trCount_max);
	for (int i = 0; i < trCount; i++) {
		idx += sprintf(&g_Message[idx], "%02X ", trBuffer[i]);
	}
	sprintf(&g_Message[idx], "\n");
	ERR_LOG(g_Message);

	rc = reedl_ictrl_sspiem_write(trBuffer, trCount);
	if (rc) {
		ERR_LOG("Can't transmit\n");
		return -1;
	}
	return 1;
}

/************************************************************************
* Function TRANS_receiveBytes(unsigned char *rcBuffer, int rcCount)
* Purpose: To receive certain number of bits, indicating by rcCount,
* over SPI port.
*
* Data received can be stored in rcBuffer.
* rcCount indicates number of bits to receive.  It should be 
* divisible by 8.  If it is not divisible by 8, pad the data with 1's.
* The function returns 1 if success, or 0 if fail.
*************************************************************************/

/*********************************************************************
* here you may implement transmitByte function
*********************************************************************/
int TRANS_receiveBytes(unsigned char *rcBuffer, int rcCount)
{
	int rc;
	rcCount = rcCount >> 3;
	if (rxCount_max < rcCount) rxCount_max = rcCount;

	int idx = sprintf(g_Message, "=====*= RX %d (%d): ", rcCount, rxCount_max);

	rc = reedl_ictrl_sspiem_read(rcBuffer, rcCount);

	for (int i = 0; i < rcCount; i++) {
		idx += sprintf(&g_Message[idx], "%02X ", rcBuffer[i]);
	}
	sprintf(&g_Message[idx], "\n");
	print_out_string(g_Message);

	if (rc) {
		ERR_LOG("Can't receive\n");
		return -1;
	}

	return 1;
}

/************************************************************************
* Function TRANS_starttranx(unsigned char channel)
* Purpose: To start an SPI transmission
*
* Return:		1 - succeed
*				0 - fail
*
* This function is responsible to pull chip select low.
* If your embedded system has a dedicated SPI port that does not require
* manually pulling chip select low, simply return 1.
*************************************************************************/
/*********************************************************************
* here you should implement starting SPI transmission.
**********************************************************************/	
int TRANS_starttranx(unsigned char channel)
{
	int rc;
	// ERR_LOG("===*= CS EN\n");

	cs_state = 0;
	rc = reedl_ictrl_sspiem_cs(cs_state);

	if (rc) {
		ERR_LOG("Can't CS\n");
		return 0;
	}

	return 1;
}
/************************************************************************
* Function TRANS_endtranx()
* Purpose: To end an SPI transmission
*
* Return:		1 - succeed
*				0 - fail
*
* If your embedded system has a dedicated SPI port that does not require
* implementing this function, simply return 1.
*************************************************************************/

/*********************************************************************
* here you should implement ending SPI transmission.
**********************************************************************/
int TRANS_endtranx()
{
	int rc;
	// ERR_LOG("===*= CS DIS\n");

	cs_state = 1;
	rc = reedl_ictrl_sspiem_cs(cs_state);

	if (rc) {
		ERR_LOG("Can't CS\n");
		return 0;
	}

	return 1;

}
/************************************************************************
* Function TRANS_trsttoggle(unsigned char toggle)
* Purpose: To toggle CRESET (TRST) signal
*
* Return:		1 - succeed
*				0 - fail
*
*************************************************************************/
int TRANS_trsttoggle(unsigned char toggle)
{
	int rc;
	/*********************************************************************
	* here you should implement toggling CRESET signal.
	*
	* Currently it prints message on screen and in log file.
	**********************************************************************/
	if (toggle) {
		ERR_LOG("=*= Reset 1\n");
	}
	else {
		ERR_LOG("=*= Reset 0\n");
	}
	rc = reedl_ictrl_sspiem_reset(toggle);
	if (rc) {
		ERR_LOG("Can't toggle reset\n");
		return 0;
	}

	return 1;
}
/************************************************************************
* Function TRANS_cstoggle(unsigned char channel)
* Purpose: To toggle chip select (CS) of specific channel
*
* Return:		1 - succeed
*				0 - fail
*
* If your embedded system has a dedicated SPI port that does not 
* allow bit banging, simply transmit a byte of 0xFF to the device,
* and the device will ignore that.
*************************************************************************/
int TRANS_cstoggle(unsigned char channel)
{
	ERR_LOG("===*= CS toggle\n");

	if(channel != 0x00)
		return 0;
	else{
	/*********************************************************************
	* here you should implement toggling CS.
	*
	* Currently it prints message on screen and in log file.
	**********************************************************************/

		cs_state = !cs_state;
		reedl_ictrl_sspiem_cs(cs_state);

		return 1;
	}
}

/************************************************************************
* Function TRANS_runClk(int clk)
* Purpose: To drive extra clock.
*
* Return:		1 - succeed
*				0 - fail
*
* If your embedded system has a dedicated SPI port that does not 
* allow bit banging, simply transmit a byte of 0xFF on another channel
* that is not being used, so the device will only see the clock.
*************************************************************************/

/*********************************************************************
* here you should implement running free clock
**********************************************************************/
int TRANS_runClk(int clk)
{
	if (runclk_max < clk) runclk_max = clk;
	int rc = reedl_ictrl_sspiem_runclk();

	sprintf(g_Message, "===*= Runclk (%d)\n", clk);
	ERR_LOG(g_Message);

	if (rc) {
		ERR_LOG("Can't run the clocks\n");
		return -1;
	}

	return 1;
}
/************************************************************************
* Function TRANS_transceive_stream(int trCount, unsigned char *trBuffer, 
* 					int trCount2, int flag, unsigned char *trBuffer2
* Purpose: Transmits opcode and transceive data
*
* It will have the following operations depending on the flag:
*
*		NO_DATA: end of transmission.  trCount2 and trBuffer2 are discarded
*		BUFFER_TX: transmit data from trBuffer2
*		BUFFER_RX: receive data and compare it with trBuffer2
*		DATA_TX: transmit data from external source
*		DATA_RX: receive data and compare it with data from external source
*
* If the data is not byte bounded and your SPI port only transmit/ receive
* byte bounded data, you need to have padding to make it byte-bounded.
* If you are transmit non-byte-bounded data, put the padding at the beginning
* of the data.  If you are receiving data, do not compare the padding,
* which is at the end of the transfer.
*************************************************************************/

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
		/* calculate # of bytes being transmitted */
		tranxByte = (unsigned short) (trCount / 8);
		if(trCount % 8 != 0){
			tranxByte ++;
			trCount += (8 - (trCount % 8));
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

			/* do not remove the line below!  It handles the padding for 
			   non-byte-bounded data */
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