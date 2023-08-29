/**************************************************************
*
* Lattice Semiconductor Corp. Copyright 2008
* 
* ispVME Embedded allows programming of Lattice's suite of FPGA
* devices on embedded systems through the JTAG port.  The software
* is distributed in source code form and is open to re - distribution
* and modification where applicable.
*
*
***************************************************************/
/**************************************************************
* 
* Revision History:
* 
*
*
***************************************************************/
#include <windows.h>
#include <conio.h>
#include <direct.h>
#include <process.h>
#include <stdio.h>
#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mmsystem.h>
#include <ctype.h>
#include "usb_hw.h"
#include "FTCJTAG.h"
#include "ft2232hmpssejtag.h"
#include <vector>
#include <map>
#include <string>
using namespace std;
/***************************************************************
*
* Stores the status of USB support.
*
***************************************************************/
BOOL FTUSBSupport  = FALSE;
char g_cUSBDriverName[1024] = {"FTUSB-0"};
HANDLE g_hUSBDevice = NULL;
int g_TCKDelay = 1;		// TCK Delay time used to stretch the clock.

class CUSBCable
{
public:
	CUSBCable(){
		m_strPortValue = "";
		m_strDescValue = "";
	}
	virtual ~CUSBCable() {
		m_strPortValue = "";
		m_strDescValue = "";
	};

	void SetPortValue( const char *port )
	{
		if(!port)
		{
			m_strPortValue = "";
			return;
		}
		m_strPortValue = port;
	}

	const char * GetPortValue() const
	{
		return m_strPortValue.c_str();
	}

	void SetDescValue( const char *desc )
	{
		if(!desc)
		{
			m_strDescValue = "";
			return;
		}
		m_strDescValue = desc;
	}

	const char * GetDescValue() const
	{
		return m_strDescValue.c_str();
	}

protected:
	string m_strPortValue;
	string m_strDescValue;
};

typedef vector <CUSBCable> t_USBCable;
vector< CUSBCable> g_listUSBCable;
void writeTRST(  unsigned char a_ucValue )
{
	if(FTUSBSupport)
	{
		SPI_SetTRST((DWORD)g_hUSBDevice,a_ucValue);
	}
}
void writeCS( unsigned char a_ucValue )
{
	if(FTUSBSupport)
	{
		SPI_SetISPEN((DWORD)g_hUSBDevice,a_ucValue);
	}
} 
void runClocks( int a_ucValue )
{
	if(FTUSBSupport)
	{
		SPI_GenerateClockPulses((DWORD)g_hUSBDevice, a_ucValue);
	}
}
/***************************************************************
*
* ispVMDelay
*
*
* The user must implement a delay to observe a_usMicroSecondDelay, where
* a_usMicroSecondDelay is the number of micro - seconds that must pass before
* data is read from in_port.  Since platforms and processor speeds
* vary greatly, this task is left to the user.
* This subroutine is called upon to provide a delay from 1 millisecond
* to a few hundreds milliseconds each time. That is the
* reason behind using unsigned long integer in this subroutine.
* It is OK to provide longer delay than required. It is not
* acceptable if the delay is shorter than required. 
*
* Note: user must re - implement to target specific hardware.
*
* Example: Use the for loop to create the micro-second delay.
*  
*          Let the CPU clock (system clock) be F Mhz. 
*                                   
*          Let the for loop represented by the 2 lines of machine code:
*                    LOOP:  DEC RA;
*                           JNZ LOOP;
*          Let the for loop number for one micro-second be L.
*          Lets assume 4 system clocks for each line of machine code.
*          Then 1 us = 1/F (micro-seconds per clock) 
*                       x (2 lines) x (4 clocks per line) x L
*                     = 8L/F          
*          Or L = F/8;
*        
*          Lets assume the CPU clock is set to 48MHZ. The C code then is:
*
*          unsigned long F = 48;  
*          unsigned long L = F/8;
*          unsigned long index;
*          if (L < 1) L = 1;
*          for (index=0; index < a_usMicroSecondDelay * L; index++);
*          return 0;
*           
*	
* The timing function used here is for PC only by hocking the clock chip. 
*
***************************************************************/

void ispVMDelay( unsigned short a_usMicroSecondDelay )
{
	unsigned short usiDelayTime = 0;  /* Stores the delay time in milliseconds */
	
	if ( a_usMicroSecondDelay & 0x8000 ) {

		/***************************************************************
		*
		* Since MSB is set, the delay time must be decoded to 
		* millisecond. The SVF2VME encodes the MSB to represent 
		* millisecond.
		*
		***************************************************************/

		a_usMicroSecondDelay &= ~0x8000;
		usiDelayTime = a_usMicroSecondDelay;
	}
	else {

		/***************************************************************
		*
		* Since MSB is not set, the delay time is given as microseconds.
		* Convert the microsecond delay to millisecond.
		*
		***************************************************************/

		usiDelayTime = (unsigned short) (a_usMicroSecondDelay / 1000);
	}
	JTAG_BufferAccess((DWORD)g_hUSBDevice);
	DWORD dwCurrentTime = 0;
	DWORD dwStartTime = 0;
	dwStartTime = time( NULL );
	timeBeginPeriod( 1 );
	dwCurrentTime = timeGetTime();    
	while ( ( dwStartTime = timeGetTime() - dwCurrentTime ) < ( DWORD ) ( usiDelayTime ) )
	{}
	timeEndPeriod( 1 );
}
/***************************************************************
*
* fnDisableUSBSupport
*
* Disables USB driver support and close the driver
*
***************************************************************/
int fnDisableFTUSBSupport()
{
	if(FTUSBSupport)
	{
		if (g_hUSBDevice != 0)
		{
			JTAG_BufferAccess((DWORD)g_hUSBDevice);
			int rcode = JTAG_Close((DWORD)g_hUSBDevice);
			if(rcode)
			{
				return(-87);
			}
			g_hUSBDevice = 0;
			FTUSBSupport = false;
		}
	}
	return(0);
}
int fnSearchFTUSBDriver(char *szDriverName)
{
	DWORD dwNumDevices = 0;
	FTC_STATUS ftStatus = -78;
	char szDeviceName[256];
	char szSerialNumber[256];
	char szChannel[5];
	DWORD dwDeviceType = 0;
	DWORD dwLocationID = 0;
	DWORD dwDeviceIndex = 0;
	DWORD dwTotalNumDevices = 0;
	CUSBCable USBList;
	CHAR	 Buff[MAX_PATH]; 
	g_listUSBCable.clear();
	fnDisableFTUSBSupport();
#ifndef _WINDOWS
	JTAG_OpenClass();
#endif
	ftStatus = JTAG_GetNumDevices(&dwNumDevices);
	if ((ftStatus == FTC_SUCCESS) && (dwNumDevices > 0))
	{
		do
		{
			ftStatus = JTAG_GetDeviceNameLocID(dwDeviceIndex, 
				szDeviceName, 
				256, 
				&dwLocationID, 
				szChannel, 5, 
				&dwDeviceType,
				szSerialNumber);

			if(ftStatus == FTC_SUCCESS )
			{
				sprintf(Buff,"FTUSB-%d",dwTotalNumDevices);
				USBList.SetPortValue(Buff);
				sprintf(Buff,"%s Location %.4X Serial %s", szDeviceName, dwLocationID, szSerialNumber);
				USBList.SetDescValue(Buff);
				g_listUSBCable.push_back(USBList);
				dwTotalNumDevices++;
			}
			dwDeviceIndex = dwDeviceIndex + 1;
		}while ((ftStatus == FTC_SUCCESS) && (dwDeviceIndex < dwNumDevices));
	}
	return(dwTotalNumDevices);
}
/***************************************************************
*
* fnEnableUSBSupport
*
* This function enables USB driver support.  
* Input:
*	szFilename : The Lattice's USB cable micro-code hex file
*				 The default name is lscispvmusbfx2.hex
*				 The file should be located in the same directory
*				 as the executable file
*	szDriverName: The USB port address
*				 The default is "ezusb-0" which is the first cable plug in
*				 The supported address could be from "ezusb-0"..
*				 "ezusb-1".."ezusb-2"...."ezusb15"
* Output:
*	If passed then enable the USB support and return
*	If failed then return error
***************************************************************/
int fnEnableFTUSBSupport(char *szDriverName)
{
	int rcode = 0;
	int i;
	char strtmp[1024];
	unsigned char VCCDetect = 0;
	unsigned long int fFrequency = 0;
	char *pdest;
	char *marker;
	int USBDriverIndex = -1;
	bool FTUSBSelected = FALSE;
	strcpy(strtmp,szDriverName);
	strupr(strtmp);
	pdest = strstr( strtmp, "FTUSB-" );
	if (pdest)
	{
		FTUSBSelected = TRUE;
	}
	strcpy(strtmp,szDriverName);
	marker = strrchr(strtmp, '-');
	if (marker)
	{
		*marker++;
		USBDriverIndex = atoi(marker);
	}
	if(!FTUSBSupport)
	{
		g_hUSBDevice = NULL;
		if(FTUSBSelected)
		{
			fnSearchFTUSBDriver(szDriverName);
			if(!g_listUSBCable.empty() && strcmp(szDriverName,""))
			{
				for(i = 0; i< g_listUSBCable.size();i++)
				{	
					if(!strcmp(g_listUSBCable[i].GetPortValue(),szDriverName))
					{
						strcpy(szDriverName,g_listUSBCable[i].GetDescValue());
						break;
					}
				}

			}
			char szDeviceName[100];
			char szChannel[5];
			DWORD dwLocationID = 0;
			DWORD dwHiSpeedDeviceType = 0;
			DWORD dwNumDevices = 0;
			char szSerialNumber[256];
			char oldBuff[1024];					
			rcode = JTAG_GetNumDevices(&dwNumDevices);
			if ((rcode == FTC_SUCCESS) && (dwNumDevices > 0))
			{
				rcode = JTAG_GetDeviceNameLocID(USBDriverIndex, 
					szDeviceName, 
					100, 
					&dwLocationID, 
					szChannel, 5, 
					&dwHiSpeedDeviceType,
					szSerialNumber);
				if(rcode == FTC_SUCCESS )
				{
					char Buff[1024];
					sprintf(Buff," Location %.4X",dwLocationID);
					char strBuffer[1024];
					int k = 0;
					for (int j = 0; j < strlen(szDeviceName) ; j++)
					{
						if(szDeviceName[j] == '<' || szDeviceName[j] == '>')
						{
							strBuffer[k] = '-';
						}
						else
							strBuffer[k] = szDeviceName[j];
						k++;
						if(szDeviceName[j] == '&')
						{
							strBuffer[k++] = 'a';
							strBuffer[k++] = 'm';
							strBuffer[k++] = 'p';
							strBuffer[k++] = ';';										
						}
					}
					strBuffer[k] = '\0';
					strcat(strBuffer,Buff);
					sprintf(oldBuff,"%s",szDeviceName);
#ifdef _WINDOWS						
					rcode = JTAG_Open((PDWORD)&g_hUSBDevice, USBDriverIndex );
#else
					rcode = JTAG_OpenEx(oldBuff, dwLocationID, szSerialNumber, (PDWORD)&g_hUSBDevice);
#endif
					if ((rcode == FTC_SUCCESS) && ((g_hUSBDevice != 0)))
					{
						rcode  = JTAG_InitDevice((DWORD)g_hUSBDevice, 0); 
						if (rcode == FTC_SUCCESS)
						{
							rcode  = SPI_InitDevice((DWORD)g_hUSBDevice,1); 
							if (rcode == FTC_SUCCESS)
							{
								FTUSBSupport = TRUE;
								JTAG_SetClock( (DWORD)g_hUSBDevice, 0 , &fFrequency);
								JTAG_SetClock(  (DWORD)g_hUSBDevice, g_TCKDelay , &fFrequency);
								printf("FTUSB Download cable detected.\n");
							}
							else
							{
								JTAG_Close((DWORD)g_hUSBDevice);
								printf("Error communicating with FTUSB cable.\n\n");
								return(-78);
							}
						}
						else
						{
							JTAG_Close((DWORD)g_hUSBDevice);
							printf("Error communicating with FTUSB cable.\n\n");
							return(-78);
						}
					}
				}
			}
		}
	}
	else if(FTUSBSupport && g_hUSBDevice != ( HANDLE *)NULL)
	{
		rcode  = SPI_InitDevice((DWORD)g_hUSBDevice, 1); 
		if (rcode != FTC_SUCCESS)
		{
			JTAG_Close((DWORD)g_hUSBDevice);
			printf("Error communicating with FTUSB cable.\n\n");
			return(-87);
		}
	}
	return(0);
}
/***************************************************************
*
* fnUSBPumpData
* 
*	Feeds data to the USB bus.
*	Input:
*	a_ucpByIn - buffer of data to write out to USB
*	The byte data order is MSB....LSB
*	The shifting order is TDI-->LSB..MSB-->TDO
*	a_ucpByOut - if a_iread is enabled, read back data is stored into this buffer
*	If a_iread = 1 The readback byte data order is LSB....MSB
*	If a_iread = 2 The readback byte data order is MSB....LSB
*	a_iClocks - number of clock cycles
*	a_ijtag - specifies if this is an ISP (0) or JTAG (1) devices
*	For JTAG devices the driver will move the JTAG state machine
*	from SHIFTIR or SHIFTDR state to IRPAUSE or DRPAUSE after
*	shifting the last bit of data.
*	For ISP devices the JTAG state machine will stay at IRSHIFT or DRSHIFT
*	The flag can be used to performing cascading shifting data
*	a_iread - specifies if we should read back data from USB
*
***************************************************************/
int fnUSBPumpData(unsigned char *a_ucpByIn,
				  unsigned char *a_ucpByOut,
				  long int a_iClocks,
				  int a_ijtag,
				  int a_iread)
{
	int rcode = 0;
	unsigned char *ReadData = NULL;
	unsigned char *PumpData = NULL;
	if((PumpData = new unsigned char[(a_iClocks+7)/8]) == NULL)
		return(-1);
	BYTE Data = 0;
	int index = 0;
	if(a_ucpByIn){	
		for (int i = 0; i<(a_iClocks+7)/8;i++) {
			Data = a_ucpByIn[i];
			BYTE FlipData = 0;
			for (int j=0;j<8;j++) {
				FlipData <<= 1;
				if (Data & 0x1)
					FlipData |= 0x1;
				Data >>= 1;
			}
			PumpData[index++] = FlipData;
		}
	}
	else{
		for (int i = 0; i<(a_iClocks+7)/8;i++) {
			PumpData[index++] = 0xFF;
		}
	}
	index = 0;
	if(a_iread)
	{		
		if((ReadData = new unsigned char[(a_iClocks+7)/8]) == NULL)
		{
			if(PumpData)
				delete PumpData;
			PumpData = 0;
			return(-1);
		}
		DWORD ByteReturn = 0;
		if(a_iClocks > 524280)
		{
			int TDIIndex = 0;
			DWORD i = 0;
			unsigned char *_WriteData = NULL;
			unsigned char *_ReadData = NULL;
			int ReadIndex = 0;
			DWORD wIndex = ( DWORD)0;
			DWORD wXferCount = (DWORD)0;
			bool boolContinue = TRUE;
			DWORD wBytesToXfer = a_iClocks / ( WORD)8;
			if( a_iClocks % ( WORD)8)
			{
				wBytesToXfer++;
			}
			if( wBytesToXfer)
			{
				while( boolContinue)
				{
					JTAG_BufferAccess((DWORD)g_hUSBDevice);	
					if( ( DWORD)( wBytesToXfer - wIndex) > (DWORD)60000)
					{
						wXferCount = ( DWORD)60000;										
						if((_WriteData = new unsigned char[wXferCount]) == NULL)
							return -1;
						for (i=0; i<wXferCount; i++) {
							_WriteData[i] = PumpData[TDIIndex++];
						}
						if((_ReadData = new unsigned char[wXferCount]) == NULL)
							return -1;
						rcode = SPI_WriteRead((DWORD)g_hUSBDevice, 
												(DWORD)(wXferCount*8),
												(PWriteDataByteBuffer)_WriteData,
												(DWORD)wXferCount,
												(PReadDataByteBuffer)_ReadData,
												(LPDWORD)&ByteReturn);
						if(rcode)
						{
							if(_WriteData)
								delete [] _WriteData;
							if(_ReadData)
								delete [] _ReadData;
							if(ReadData)
								delete [] ReadData;
							if(PumpData)
								delete [] PumpData;	
							printf("Error communicating with FTUSB cable.\n");
							char ErrorMessage[1024];
							JTAG_GetErrorCodeString("EN",rcode,&ErrorMessage[0],1024);
							printf("%s\n\n",ErrorMessage);	
							return(-1);
						}
						for (i=0; i<wXferCount; i++) {
							ReadData[ReadIndex++] = _ReadData[i];
						}
						if(_ReadData)
							delete [] _ReadData;
						if(_WriteData)
							delete [] _WriteData;
						wIndex += ( DWORD)wXferCount;
					}
					else
					{
						wXferCount = ( DWORD)(wBytesToXfer - wIndex);
						if((_WriteData = new unsigned char[wXferCount]) == NULL)
							return -1;
						for (i=0; i<wXferCount; i++) {
							_WriteData[i] = PumpData[TDIIndex++];
						}
						if((_ReadData = new unsigned char[wXferCount]) == NULL)
							return -1;
						rcode = SPI_WriteRead((DWORD)g_hUSBDevice,  
											(DWORD)(wXferCount*8),
											(PWriteDataByteBuffer)_WriteData,
											(DWORD)wXferCount,
											(PReadDataByteBuffer)_ReadData,
											(LPDWORD)&ByteReturn);
						if(rcode)
						{
							if(_WriteData)
								delete [] _WriteData;
							if(_ReadData)
								delete [] _ReadData;
							if(ReadData)
								delete [] ReadData;
							if(PumpData)
								delete [] PumpData;
							printf("Error communicating with FTUSB cable.\n");
							char ErrorMessage[1024];
							JTAG_GetErrorCodeString("EN",rcode,&ErrorMessage[0],1024);
							printf("%s\n\n",ErrorMessage);								
							return(-1);
						}
						for (i=0; i<wXferCount; i++) {
							ReadData[ReadIndex++] = _ReadData[i];
						}
						if(_ReadData)
							delete [] _ReadData;
						if(_WriteData)
							delete [] _WriteData;	
						boolContinue = FALSE;
					}
				}
			}
		}
		else
		{
			rcode = SPI_WriteRead((DWORD)g_hUSBDevice,  
								(DWORD)a_iClocks,
								(PWriteDataByteBuffer)PumpData,
								(DWORD)(a_iClocks+7)/8,
								(PReadDataByteBuffer)ReadData,
								(LPDWORD)&ByteReturn);
			if(rcode)
			{
				if(ReadData)
					delete [] ReadData;
				if(PumpData)
					delete [] PumpData;
				printf("Error communicating with FTUSB cable.\n");
				char ErrorMessage[1024];
				JTAG_GetErrorCodeString("EN",rcode,&ErrorMessage[0],1024);
				printf("%s\n\n",ErrorMessage);	
				return(-1);
			}
		}
		if(a_ucpByOut){
			Data = 0;
			index = 0;
			for (int i = 0; i<(a_iClocks+7)/8;i++) {
				Data = ReadData[i];
				BYTE FlipData = 0;
				for (int j=0;j<8;j++) {
					FlipData <<= 1;
					if (Data & 0x1)
						FlipData |= 0x1;
					Data >>= 1;
				}
				a_ucpByOut[index++] = FlipData;
			}
		}
	}
	else{
		DWORD ByteReturn = 0;
		if(a_iClocks > 524280)
		{
			int TDIIndex = 0;
			DWORD i = 0;
			unsigned char *_WriteData = NULL;
			DWORD wIndex = ( DWORD)0;
			DWORD wXferCount = (DWORD)0;
			bool boolContinue = TRUE;
			DWORD wBytesToXfer = a_iClocks / ( WORD)8;
			if( a_iClocks % ( WORD)8)
			{
				wBytesToXfer++;
			}
			if( wBytesToXfer)
			{
				while( boolContinue)
				{
					JTAG_BufferAccess((DWORD)g_hUSBDevice);		
					if( ( DWORD)( wBytesToXfer - wIndex) > (DWORD)60000)
					{
						wXferCount = ( DWORD)60000;										
						if((_WriteData = new unsigned char[wXferCount]) == NULL)
							return -1;
						for (i=0; i<wXferCount; i++) {
							_WriteData[i] = PumpData[TDIIndex++];
						}
						rcode = SPI_WriteData((DWORD)g_hUSBDevice, 
												(DWORD)(wXferCount*8),
												(PWriteDataByteBuffer)_WriteData,
												(DWORD)(wXferCount));
						if(rcode)
						{
							if(_WriteData)
								delete [] _WriteData;
							if(ReadData)
								delete [] ReadData;
							if(PumpData)
								delete [] PumpData;
							printf("Error communicating with FTUSB cable.\n");
							char ErrorMessage[1024];
							JTAG_GetErrorCodeString("EN",rcode,&ErrorMessage[0],1024);
							printf("%s\n\n",ErrorMessage);
							return(-1);
						}
						if(_WriteData)
							delete [] _WriteData;
						wIndex += ( DWORD)wXferCount;
					}
					else
					{
						wXferCount = ( DWORD)(wBytesToXfer - wIndex);
						if((_WriteData = new unsigned char[wXferCount]) == NULL)
							return -1;
						for (i=0; i<wXferCount; i++) {
							_WriteData[i] = PumpData[TDIIndex++];
						}
						rcode = SPI_WriteData((DWORD)g_hUSBDevice, 
											(DWORD)(wXferCount*8),
											(PWriteDataByteBuffer)_WriteData,
											(DWORD)(wXferCount));
						if(rcode)
						{
							if(_WriteData)
								delete [] _WriteData;
							if(ReadData)
								delete [] ReadData;
							if(PumpData)
								delete [] PumpData;
							printf("Error communicating with FTUSB cable.\n");
							char ErrorMessage[1024];
							JTAG_GetErrorCodeString("EN",rcode,&ErrorMessage[0],1024);
							printf("%s\n\n",ErrorMessage);							
							return(-1);
						}
						if(_WriteData)
							delete [] _WriteData;	
						boolContinue = FALSE;
					}
				}
			}
		}
		else
		{
			rcode = SPI_WriteData((DWORD)g_hUSBDevice, 
								(DWORD)a_iClocks,
								(PWriteDataByteBuffer)PumpData,
								(DWORD)(a_iClocks+7)/8);
			if(rcode)
			{
				if(PumpData)
					delete [] PumpData;
				if(ReadData)
					delete [] ReadData;
				printf("Error communicating with FTUSB cable.\n");
				char ErrorMessage[1024];
				JTAG_GetErrorCodeString("EN",rcode,&ErrorMessage[0],1024);
				printf("%s\n\n",ErrorMessage);
				return(-1);
			}
		}
	}
	if(ReadData)
		delete ReadData;
	ReadData = 0;
	if(PumpData)
		delete PumpData;
	PumpData = 0;
	return(rcode);
}

