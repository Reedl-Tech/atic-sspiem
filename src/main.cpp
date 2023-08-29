/**************************************************************
*
* Lattice Semiconductor Corp. Copyright 2011
* 
* Slave SPI Embedded (SSPI Embedded) is embedded software that enables 
* the user to Program Lattice Semiconductor's Field Programmable Gated Array 
* (FPGA) with Slave SPI port.  Depending on users' system configuration, 
* some of the source codes need to be modified to fit the system.
*
*
* sspi Embedded C Source comprised with 4 modules:
* main.c is the module provides input and output support.
* SSPIEm.c is the modeule provides Main processing engine
* intrface.c is the module provides the interface to get the algorithm and programming data
* core.c is the module interpret the algorithm and data files.
* hardware.c is the module access the SSPI port of the device(s).                 
*
*
***************************************************************/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "SSPIEm.h"
#include "usb_hw.h"
#include "debug.h"

/***************************************************************
*
* Supported SSPIEm versions.
*
***************************************************************/
#define VME_VERSION_NUMBER "4.0"
/***************************************************************
*
* ispVM Embedded Return Codes.
*
***************************************************************/

#define VME_VERIFICATION_FAILURE		-1
#define VME_FILE_READ_FAILURE			-2
#define VME_VERSION_FAILURE				-3
#define VME_INVALID_FILE				-4
#define VME_ARGUMENT_FAILURE			-5
#define VME_CRC_FAILURE					-6


/***************************************************************
*
* Global variables.
*
***************************************************************/
char USBDriverName[256] = {"FTUSB-0"};
extern unsigned int a_uiRowCount;
extern int a_uiDebug;
extern int g_TCKDelay;	
/***************************************************************
*
* print_out_string
*
* Send a text string out to the output resource if available. 
* The monitor is the default output resource. 
*
*
***************************************************************/
void print_out_string(char *stringOut)
{
	if(stringOut)
	{
		printf("%s",stringOut);
	}
}
/***************************************************************
*
* print_usage
*
***************************************************************/
void print_usage(){
	print_out_string( "\nUsage: sspiem.exe sea_file sed_file [-pa][-tck_delay][-debug]\n" );
	print_out_string( "option           -pa: USB port address < FTUSB-0 ... FTUSB-15>\n" );
	print_out_string( "                 -pa FTUSB-1 :  Use the USB port address FTUSB-1\n" );
	print_out_string( "                 Default use the USB port address FTUSB-0\n" );
	print_out_string( "Example: sspiem.exe algo.iea data.ied -pa FTUSB-0\n" );
	print_out_string( "option	-debug: print out debug information\n");
	print_out_string( "option	-tck_delay: Clock frequency delay\n");
	print_out_string( "Example: sspiem.exe algo.iea data.ied -pa FTUSB-0 -tck_delay 4 -debug\n" );
	print_out_string( "\n\n");
}
/***************************************************************
*
* printError
*
***************************************************************/
void printError(int code){
	char Message[512];
	sprintf(Message, "Error Code: %d\n\n", code);
	print_out_string(Message);
	switch(code){
	case ERROR_INIT_ALGO:
		print_out_string("Initialize algorithm file fail.\n");
		break;
	case ERROR_INIT_DATA:
		print_out_string("Initialize data file fail.\n");
		break;
	case ERROR_INIT_VERSION:
		print_out_string("Version not supported.\n");
		break;
	case ERROR_INIT_CHECKSUM:
		print_out_string("Header checksum fail.\n");
		break;
	case ERROR_INIT_SPI:
		print_out_string("Initialize SPI fail.\n");
		break;
	case ERROR_INIT:
		print_out_string("Initialization fail.\n");
		break;
	case ERROR_PROC_ALGO:
		print_out_string("Incorrect algorithm format.\n");
		break;
	case ERROR_PROC_DATA:
		print_out_string("Invalid data.\n");
		break;
	case ERROR_PROC_HARDWARE:
		print_out_string("Hardware fail.\n");
		break;
	case ERROR_VERIFICATION:
		print_out_string("Verification fail.\n");
		if(a_uiRowCount > 0)
		{
			sprintf(Message, "Failed on Frame %d\n",a_uiRowCount);
			print_out_string(Message);
		}
		break;
	case ERROR_IDCODE:
		print_out_string("IDCODE verification fail.\n");
		break;
	case ERROR_USERCODE:
		print_out_string("USERCODE verification fail.\n");
		break;
	case ERROR_SED:
		print_out_string("SED CRC verification fail.\n");
		break;
	case ERROR_TAG:
		print_out_string("TAG Memory verification fail.\n");
		break;
	case ERROR_LOOP_COND:
		print_out_string("LOOP condition fail.\n");
		break;
	default:
		print_out_string("Process fail.\n");
		break;
	}
}

/***************************************************************
*
* main
*
***************************************************************/

int main( int argc, char * argv[] )
{
	char Message[512];
	unsigned short iCommandLineIndex	= 0;
	short siRetCode						= 0;
	char szExtension[ 5 ]				= { 0 };
	char szCommandLineArg[ 300 ]		= { 0 };
	char *szCommandLineArgPtr;
	time_t tStartTime, tEndTime;	
	unsigned short usProcessTime;

	print_out_string( "                 Lattice Semiconductor Corp." );
	sprintf(Message,"\n             SSPI Embedded(tm) V%s 2015\n",VME_VERSION_NUMBER);
	print_out_string( Message );
	if ( ( argc < 3 ) || ( argc > 8 ) ) {
		print_usage();
		exit( -1 );
	}
	// user interface
	for ( iCommandLineIndex = 1; iCommandLineIndex < argc; iCommandLineIndex++ ) {
		strcpy( szCommandLineArg, argv[ iCommandLineIndex ] );
		if( strchr(szCommandLineArg, '\"') != strrchr(szCommandLineArg, '\"') ){
			szCommandLineArgPtr = strchr(szCommandLineArg, '\"');
			*strrchr(szCommandLineArg, '\"') = 0;
		}
		else{
			szCommandLineArgPtr = strchr(szCommandLineArg, '\"');
			if(szCommandLineArgPtr)
				*szCommandLineArgPtr = 0;
			else
				szCommandLineArgPtr = szCommandLineArg;
		}
		strcpy( szExtension, &szCommandLineArgPtr[ strlen( szCommandLineArg ) - 4 ] );
		strlwr( szExtension );
		if(!strcmp( strlwr( szCommandLineArg ), "-pa" ))
		{
			if ( ++iCommandLineIndex >= argc ) {
				printf( "Error: USB port address value not given\n\n" );
				exit( -1 );
			}

			strcpy(USBDriverName ,argv[ iCommandLineIndex ] );
			if ( !strcmp(USBDriverName,"") ) {
				printf( "Error: USB port address value not given\n\n" );
				exit( -1 );
			}
			printf("%s ", USBDriverName);
		}
		if(!strcmp( strlwr( szCommandLineArg ), "-debug" ))
		{
			a_uiDebug = 1;
		}
		if(!strcmp( strlwr( szCommandLineArg ), "-tck_delay" ))
		{
			if ( ++iCommandLineIndex >= argc ) {
				printf( "Error: TCK Delay value not given\n\n" );
				exit( -1 );
			}
			g_TCKDelay = atoi(argv[ iCommandLineIndex ] );
			printf("TCK Delay %d ", g_TCKDelay);
		}
	}
	printf("\n\n");
	// end user interface
	time( &tStartTime );
	siRetCode = (short int)fnEnableFTUSBSupport(USBDriverName);
	if(siRetCode < 0){
		printf( "Error: Unable to start FTDI USB cable.\n\n" );
		return 0;
	}
	siRetCode = (short)SSPIEm_preset(argv[ 1 ], argv[ 2 ]);
	siRetCode = (short)SSPIEm(0xFFFFFFFF);
	fnDisableFTUSBSupport();
	time( &tEndTime );
	usProcessTime = ( unsigned short )(( unsigned short ) tEndTime - ( unsigned short ) tStartTime);
	sprintf(szCommandLineArg, "Processing time: %d secs\n\n", usProcessTime );
	printf(szCommandLineArg);
	if ( siRetCode != 2 ) {
		printf ("\n\n");
		printf( "+=======+\n" );
		printf( "| FAIL! |\n" );
		printf( "+=======+\n\n" );
		printError(siRetCode);
	} 
	else {
		printf( "+=======+\n" );
		printf( "| PASS! |\n" );
		printf( "+=======+\n\n" );
	}
	exit( siRetCode );
	return siRetCode;
} 


