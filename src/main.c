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
#include <stdio.h>
#include <locale.h>
#include <sys/syslog.h>

#include "SSPIEm.h"
#include "debug.h"

#define REEDL_ICTRL

#ifdef REEDL_ICTRL
#include "reedl-ictrl.h"
#include "reedl-ictrl-cmd.h"
#endif
int wait(int a_msTimeDelay);
/***************************************************************
*
* Supported SSPIEm versions.
*
***************************************************************/

#define VME_VERSION_NUMBER "4.0"

/***************************************************************
*
* SSPI Embedded Return Codes.
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
* External variables declared in hardware.c module.
*
***************************************************************/
extern unsigned int a_uiRowCount;


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
	print_out_string( "\nUsage: sspiem sea_file sed_file\n" );
	print_out_string( "Example: sspiem algo.sea data.sed\n" );
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

reedl_ictrl_t *ictrl;

int main( int argc, char * argv[] )
{
	int rc;
	int siRetCode = 0;
	char Message[512];
	
	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog(*argv, LOG_CONS | LOG_PID, LOG_DAEMON);

	setlocale(LC_CTYPE, "en_US.UTF-8");
	
	print_out_string( "                 Lattice Semiconductor Corp.\n" );
	sprintf(Message,"\n             SSPI Embedded(tm) V%s 2012", VME_VERSION_NUMBER);
	print_out_string( Message );

	if ( argc < 2 ) {
		print_usage();
		return ( 1 );
	}

#ifdef REEDL_ICTRL
	rc = reedl_ictrl_init((argc >= 3) ? argv[3] : NULL);
	if (rc) {
		sprintf(Message,"\n Can't init REEDL ICTRL - %s (%d)",
			(rc == -1) ? "serial port" :
			(rc == -2) ? "bad_signature" : 
			"unknown", rc);
		print_out_string("Can't init REEDL ICTRL\n");
		return ERROR_INIT;
	}
#endif

	uint8_t tr1_out[8] = {0xA4, 0xC6, 0xF4, 0x8A};
	uint8_t readid_inout[8]     = {0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t readstatus_inout[8] = {0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	uint8_t tr_in[8] = {-1};
	rc = 0;

#if 0
	// AV: it alway returns zeros :(
	rc |= reedl_ictrl_sspiem_init(1);
	rc |= reedl_ictrl_sspiem_reset(1);
	rc |= reedl_ictrl_sspiem_reset(0);
	rc |= reedl_ictrl_sspiem_runclk();
	rc |= reedl_ictrl_sspiem_write(&tr1_out[0], 4);
	rc |= reedl_ictrl_sspiem_reset(0);
	rc |= reedl_ictrl_sspiem_reset(1);
	wait(10);
	rc |= reedl_ictrl_sspiem_write(&readstatus_inout[0], 8);
	rc |= reedl_ictrl_sspiem_write(&readid_inout[0], 8);
	rc |= reedl_ictrl_sspiem_write(&readid_inout[0], 4);
	rc |= reedl_ictrl_sspiem_read(&readid_inout[4], 4);
	rc = reedl_ictrl_sspiem_init(0);
#endif

	siRetCode = SSPIEm_preset(argv[ 1 ], argv[ 2 ]);
	siRetCode = SSPIEm(0xFFFFFFFF);
	if ( siRetCode != 2 ) {

		print_out_string ("\n\n");
		print_out_string( "+=======+\n" );
		print_out_string( "| FAIL! |\n" );
		print_out_string( "+=======+\n\n" );
		printError(siRetCode);

	} 
	else {
		print_out_string( "+=======+\n" );
		print_out_string( "| PASS! |\n" );
		print_out_string( "+=======+\n\n" );
	}
	return siRetCode;
} 



