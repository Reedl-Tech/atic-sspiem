/************************************************************************
*
* intrface.c
* Version 4.0 - PC file system configuration
*
* This file has 3 parts:
* - algorithm utility functions
* - data utility functions
* - decompression utility functions
*
* The first 2 parts may require user modification to fit how algorithm
* and data files are retrieved by the embedded system.
*
************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "reedl-generic.h"
#include "opcode.h"
#include "debug.h"
#include "intrface.h"

extern void print_out_string(char* stringOut);
extern char g_Message[512];

/************************************************************************
*
* algorithm utility functions
*
************************************************************************/

	/************************************************************************
	*
	* algorithm utility functions
	*
	*		For those who do not wish to compile the algorithm with
	* the processing system, you are responsible to modify the
	* following functions or the processing system is not going to
	* run.
	*
	************************************************************************/

	/************************************************************************
	*
	* Design-dependent Global variable
	*
	* You may insert variable here to fit your design
	*
	************************************************************************/
	
	FILE *algoFilePtr        = 0;
	char g_algoFileName[64]  = {0};


	/************************************************************************
	*
	* Design-dependent Functions
	*
	* Here is the list of functions you need to modify:
	*
	* algoPreset()  - This function allows you to set where the algorithm
	*					is prior to running SSPI Embedded.  
	*					If the embedded system has a file system, you
	*					may set the name of the file to SSPI Embedded,
	*					then implement opening the file in function
	*					dataInit().
	*
	* algoInit()	  - In this function, you are responsible to initialize
	*					the algorithm stream.  If the embedded system has a 
	*					file system, you may open the file in this function.
	*
	* algoGetByte() - This function is responsible to get a byte from
	*					algorithm.
	*
	* algoFinal()	  - This function allows you to finalize the algorithm.
	*					If the embedded system has a file system, you may 
	*					implement closing the file here.
	*
	************************************************************************/

	int algoPreset(const char algoFileName[])
	{

		/************************************************************************
		* Start of design-dependent implementation
		*
		* You may put your code here.
		************************************************************************/

		strcpy(g_algoFileName, algoFileName);

		/************************************************************************
		* End of design-dependent implementation
		************************************************************************/

		return 1;
	}

	int algoInit()
	{

		/************************************************************************
		* Start of design-dependent implementation
		*
		* You may put your code here.
		************************************************************************/
		
		algoFilePtr = fopen(g_algoFileName, "rb");
		if(!algoFilePtr)
			return 0;
		/************************************************************************
		* End of design-dependent implementation
		*
		* After this initialization, the processing engine need to be able
		* to read algorithm byte by using algoGetByte()
		************************************************************************/
		
		return 1;
	}

	int algoGetByte(unsigned char *byteOut)	
	{

		/************************************************************************
		* Start of design-dependent implementation
		*
		* You may put your code here.
		************************************************************************/

		if(!feof(algoFilePtr)){
			*byteOut = (unsigned char)fgetc(algoFilePtr);
			return 1;
		}
		return 0;

		/************************************************************************
		* End of design-dependent implementation
		************************************************************************/
	}
	int algoFinal()
	{
		/********************************************************************
		* Start of design-dependent implementation
		*
		* You may put your code here.
		*********************************************************************/

		if(algoFilePtr){
			fclose(algoFilePtr);
			algoFilePtr = NULL;
		}
		/********************************************************************
		* End of design-dependent implementation
		*********************************************************************/
		
		return 1;
	}
/************************************************************************
*
* data Utility functions
* Version 3.0
*
* Version 3.0 Update:
* Requires data reset or data reaches the end in order to reset data
* File system sample
*
* This code contains data utility functions.  Data may be stored in
* The following method:
* 1. Data is compiled to be part of the SSPI embedded system.  
* 2. Data is stored in internal / external storage
* 3. Data is transmitted over ethernet / wireless
*
************************************************************************/

	/****************************************************************
	*
	* Design-dependent Global variable
	*
	* You may insert variable here to fit your design
	*
	*****************************************************************/
	#define DATA_BUFFER_SIZE	5
	FILE *dataFile          = 0;
	char g_dataFileName[64] = {0};
	DATA_BUFFER g_dataBufferArr[DATA_BUFFER_SIZE];
	/****************************************************************	
	*
	* Table of content definition
	*
	* This creates the number of table of content used to store
	* data set information.  Reducing the number may save some memory.
	* Depending on the device, the minimum number are different.  
	* 8 is the recommended minimum.
	*
	*
	****************************************************************/
	#define		D_TOC_NUMBER			16

	/****************************************************************
	*
	* Global variable / definition
	*
	****************************************************************/
	DATA_TOC d_toc[D_TOC_NUMBER];
	unsigned short int d_tocNumber        = 0;
	unsigned char		d_isDataInput     = 0;
	unsigned int		d_offset          = 0;
	unsigned int		d_currentAddress  = 0;
	unsigned char		d_requestNewData  = 0;
	unsigned int		d_currentSize     = 0;
	unsigned short int	d_currentDataSetIndex = 0;
	CSU					d_CSU;
	short int			d_SSPIDatautilVersion = 0;

	#define		SSPI_DATAUTIL_VERSION1		1
	#define		SSPI_DATAUTIL_VERSION2		2
	#define		SSPI_DATAUTIL_VERSION3		3

	/********************************************************************
	*
	* Design-dependent Functions
	*
	* Here is the list of functions you need to modify:
	*
	* dataPreset()  - This function allows you to set where the data
	*					is prior to running SSPI Embedded.  
	*					If the embedded system has a file system, you
	*					may set the name of the file to SSPI Embedded,
	*					then implement opening the file in function
	*					dataInit().
	*
	* dataInit()	  - In this function, you are responsible to initialize
	*					the data stream.  If the embedded system has a 
	*					file system, you may open the file in this function.
	*
	* dataReset()	  - In this function, you are responsible to reset the
	*					data to the same state as it is just been initialized.
	*
	* dataGetByte() - This function is responsible to get a byte from
	*					data.
	*
	* dataFinal()	  - This function allows you to finalize the data.  If
	*					the embedded system has a file system, you may 
	*					implement closing the file here.
	*
	**********************************************************************/

	int dataPreset(const char dataFileName[])
	{
		/********************************************************************
		* Start of design-dependent implementation
		* If no data as input, set d_isDataInput = 0, 
		* else set d_isDatatInput = 1
		*
		* You may put your code here.
		*********************************************************************/
		strcpy(g_dataFileName, dataFileName);
		dataFile = NULL;
		if(strcmp(g_dataFileName, ""))
			d_isDataInput = 1;
		else
			d_isDataInput = 0;

		/********************************************************************
		* End of design-dependent implementation
		*********************************************************************/
		return 1;
	}

	int dataInit()
	{
		unsigned char currentByte = 0;
		int temp                  = 0;
		int i                     = 0;
		d_offset                  = 0;
		d_currentDataSetIndex     = 0;
		if(d_isDataInput == 0)
			return PROC_COMPLETE;

		/********************************************************************
		* Start of design-dependent implementation
		*
		* You may put your code here.
		*********************************************************************/
		dataFile = fopen(g_dataFileName, "rb");
		if(!dataFile)
			return PROC_FAIL;

		/********************************************************************
		* End of design-dependent implementation
		*
		* After this initialization, the processing engine need to be able
		* to read data by using dataGetByte()
		*********************************************************************/

		for(i = 0; i < DATA_BUFFER_SIZE; i ++){
			g_dataBufferArr[i].ID = 0x00;
			g_dataBufferArr[i].address = 0;
		}
		if( !dataGetByte( &currentByte, 0, NULL ) )
			return PROC_FAIL;
		d_offset ++;
		if(currentByte == HCOMMENT){
			temp = dataReadthroughComment();
			if( !temp )
				return PROC_FAIL;
			d_offset += temp;
			if( !dataGetByte( &currentByte, 0, NULL ) )
				return PROC_FAIL;
			d_offset ++;
		}
		if(currentByte == HDATASET_NUM){
			d_SSPIDatautilVersion = SSPI_DATAUTIL_VERSION3;
			temp = dataLoadTOC(1);
			if( !temp )
				return PROC_FAIL;
			d_offset += temp;
			d_currentAddress = 0x00000000;
			d_requestNewData = 1;
			return PROC_COMPLETE;
		}
		else if(currentByte == 0x00 || currentByte == 0x01){
			d_SSPIDatautilVersion = SSPI_DATAUTIL_VERSION1;
			set_compression(currentByte);
			return PROC_COMPLETE;
		}
		else
			return PROC_FAIL;
	}

	int dataReset(unsigned char isResetBuffer)	
	{
		unsigned char currentByte    = 0;
		int i                        = 0;

		/********************************************************************
		* Start of design-dependent implementation
		*
		* You may put your code here.
		*********************************************************************/
		if(!dataFile)
			return PROC_FAIL;
		rewind(dataFile);
		
		/********************************************************************
		* End of design-dependent implementation
		*
		* After this, the data stream should be in the same condition as
		* it was initialized.  What dataGetByte() function would get must be
		* the same as what it got when being called in dataInit().
		*********************************************************************/

		if(isResetBuffer){
			for(i = 0; i < DATA_BUFFER_SIZE; i ++){
				g_dataBufferArr[i].ID = 0x00;
				g_dataBufferArr[i].address = 0;
			}
		}
		if( !dataGetByte( &currentByte, 0, NULL ) )
			return PROC_FAIL;
		if(currentByte == HCOMMENT){
			if(!dataReadthroughComment())
				return PROC_FAIL;
			if( !dataGetByte( &currentByte, 0, NULL ) )
				return PROC_FAIL;
		}

		if(d_SSPIDatautilVersion == SSPI_DATAUTIL_VERSION3){
			dataLoadTOC(0);
			d_currentAddress = 0x00000000;
			d_currentDataSetIndex = 0;
		}
		return PROC_COMPLETE;
	}

	int dataGetByte(unsigned char *byteOut, 
		short int incCurrentAddr, CSU *checksumUnit)
	{

		/********************************************************************
		* Start of design-dependent implementation
		*
		* You may put your code here.
		*********************************************************************/

		if( !dataFile || feof(dataFile) ){
			*byteOut = 0xFF;
			return PROC_FAIL;
		}
		/* read a byte and store in *byteOut */
		*byteOut = (unsigned char)fgetc(dataFile);
		/********************************************************************
		* End of design-dependent implementation
		*********************************************************************/
		if(checksumUnit)
			putChunk(checksumUnit, (unsigned int) (*byteOut) );
		if(incCurrentAddr)
			d_currentAddress ++;
		return PROC_COMPLETE;
	}

	int dataFinal()
	{

		/********************************************************************
		* Start of design-dependent implementation
		*
		* You may put your code here.
		*********************************************************************/

		if(dataFile){
			fclose(dataFile);
			dataFile = NULL;
		}

		/********************************************************************
		* End of design-dependent implementation
		*********************************************************************/

		return 1;
	}

	/********************************************************************
	*
	* The following functions does not need user modification
	*
	*********************************************************************/

	unsigned char getRequestNewData()
	{ 
		return d_requestNewData;
	}

	int HLDataGetByte(unsigned char dataSet, 
					  unsigned char *dataByte, 
					  unsigned int uncomp_bitsize)
	{
		int retVal              = 0;
		unsigned char tempChar  = 0;
		unsigned int bufferSize = 0;
		unsigned int i          = 0;

		if(d_SSPIDatautilVersion == SSPI_DATAUTIL_VERSION1){
			if(get_compression()){
				if (uncomp_bitsize != 0 && !decomp_initFrame(uncomp_bitsize) )
					return PROC_FAIL;
				retVal = decomp_getByte(dataByte);
			}
			else
				retVal = dataGetByte(dataByte, 1, &d_CSU);
			return retVal;
		}
		else {
			if(d_requestNewData || dataSet != d_toc[d_currentDataSetIndex].ID){
				if( !dataRequestSet( dataSet ) )
					return PROC_FAIL;
				d_currentSize = 0;
				/* check if buffer has any address */
				for (i = 0; i < DATA_BUFFER_SIZE; i ++){
					if(g_dataBufferArr[i].ID == dataSet){
						bufferSize = g_dataBufferArr[i].address;
						break;
					}
				}
				for(i = 0; i < bufferSize; i ++)
					HLDataGetByte(dataSet, &tempChar, uncomp_bitsize);
			}

			if(d_toc[d_currentDataSetIndex].uncomp_size == 0){
				*dataByte = 0xFF;
				return PROC_FAIL;
			}
			else if(d_currentSize < d_toc[d_currentDataSetIndex].uncomp_size){
				if(get_compression()){
					if(uncomp_bitsize != 0 && !decomp_initFrame(uncomp_bitsize) )
						return PROC_FAIL;
					retVal = decomp_getByte(dataByte);
				}
				else
					retVal = dataGetByte(dataByte, 1, &d_CSU);
				d_currentSize ++;
				/* store data buffer */
				for(i = 0; i < DATA_BUFFER_SIZE; i ++){
					if(g_dataBufferArr[i].ID == dataSet){
						if(d_currentSize != d_toc[d_currentDataSetIndex].uncomp_size)
							g_dataBufferArr[i].address = d_currentSize;
						else{
							g_dataBufferArr[i].ID = 0x00;
							g_dataBufferArr[i].address = 0;
						}
						break;
					}
				}
				if(i == DATA_BUFFER_SIZE){
					for(i = 0; i < DATA_BUFFER_SIZE; i ++){
						if(g_dataBufferArr[i].ID == 0x00){
							g_dataBufferArr[i].ID = dataSet;
							g_dataBufferArr[i].address = d_currentSize;
							break;
						}
					}
				}
				/* check 16 bit check sum */
				if(d_currentSize == d_toc[d_currentDataSetIndex].uncomp_size){
					d_currentDataSetIndex = 0;
					d_requestNewData = 1;
					if( !dataGetByte(&tempChar, 1, &d_CSU)  )
						return PROC_FAIL;	/* read upper check sum */
					if( !dataGetByte(&tempChar, 1, &d_CSU)  )
						return PROC_FAIL;	/* read lower check sum */
					if( !dataGetByte(&tempChar, 1, &d_CSU)  )
						return PROC_FAIL;	/* read 0xB9 */
					if( !dataGetByte(&tempChar, 1, &d_CSU)  )
						return PROC_FAIL;	/* read 0xB2 */
				}
				return retVal;
			}
			else{
				d_requestNewData = 1;
				return HLDataGetByte(dataSet, dataByte, uncomp_bitsize);
			}
		}
	}

	int dataReadthroughComment()
	{
		unsigned char currentByte = 0;
		int retVal                = 0;
		init_CS(&d_CSU, 16, 8);
		do{
			if( !dataGetByte( &currentByte, 0, NULL ) )
				break;
			retVal ++;
		}while(currentByte != HENDCOMMENT);
		if(currentByte != HENDCOMMENT)
			return PROC_FAIL;
		return retVal;
	}
	int dataLoadTOC(short int storeTOC)
	{
		unsigned char currentByte = 0;
		int i                     = 0;
		int j                     = 0;
		int retVal                = 0;
		if(storeTOC){
			for (i = 0; i < D_TOC_NUMBER; i++){
				d_toc[i].ID = 0;
				d_toc[i].uncomp_size = 0;
				d_toc[i].compression = 0;
				d_toc[i].address = 0x00000000;
			}
		}
		if( !dataGetByte( &currentByte, 0, NULL ) )
			return PROC_FAIL;
		retVal ++;
		if(storeTOC)
			d_tocNumber = currentByte;
		for (i = 0; i < d_tocNumber; i++){
			/* read HTOC */
			if( !dataGetByte( &currentByte, 0, NULL ) || currentByte != HTOC )
				return PROC_FAIL;
			retVal ++;
			/* read ID */
			if( !dataGetByte( &currentByte, 0, NULL ) )
				return PROC_FAIL;
			if(storeTOC)
				d_toc[i].ID = currentByte;
			retVal ++;
			/* read status */
			if( !dataGetByte( &currentByte, 0, NULL ) )
				return PROC_FAIL;
			retVal ++;
			/* read uncompressed data set size */
			if(storeTOC)
				d_toc[i].uncomp_size = 0;
			j = 0;
			do{
				if( !dataGetByte(&currentByte, 0, NULL ) )
					return PROC_FAIL;
				else{
					retVal ++;
					if(storeTOC)
						d_toc[i].uncomp_size += (unsigned long) ((currentByte & 0x7F) << (7 * j));
					j++;
				}
			}while(currentByte & 0x80);

			/* read compression */
			if( !dataGetByte( &currentByte, 0, NULL ) )
				return PROC_FAIL;
			if(storeTOC)
				d_toc[i].compression = currentByte;
			retVal ++;
			/* read address */
			if(storeTOC)
				d_toc[i].address = 0x00000000;
			for(j = 0; j <4; j++){
				if( !dataGetByte(&currentByte, 0, NULL) )
					return PROC_FAIL;
				retVal ++;
				if(storeTOC){
					d_toc[i].address <<= 8;
					d_toc[i].address += currentByte;
				}
			}			
		}
		return retVal;
	}

	int dataRequestSet(unsigned char dataSet)
	{
		int i                      = 0;
		unsigned char currentByte  = 0;
		for(i = 0; i < d_tocNumber; i++){
			if(d_toc[i].ID == dataSet){
				d_currentDataSetIndex = i;
				break;
			}
		}
		if(i == d_tocNumber)
			return PROC_FAIL;
		
		/****************************************************************** 
		* prepare data for reading
		* for streaming data, ignore data prior to the address
		* if the current address is bigger than requested address, reset
		* the stream
		******************************************************************/
		if(d_currentAddress > d_toc[d_currentDataSetIndex].address){
			i = d_currentDataSetIndex;
			dataReset(0);
			d_currentDataSetIndex = i;
		}
		set_compression(d_toc[d_currentDataSetIndex].compression);
		/* move currentAddress to requestAddress */
		while(d_currentAddress < d_toc[d_currentDataSetIndex].address){
			if( !dataGetByte( &currentByte, 1, NULL ) )
				return PROC_FAIL;
		}
		/* read BEGIN_OF_DATA */
		if( !dataGetByte( &currentByte, 1, &d_CSU ) )
			return PROC_FAIL;
		if( !dataGetByte( &currentByte, 1, &d_CSU ) )
			return PROC_FAIL;
		d_requestNewData = 0;
		return PROC_COMPLETE;
	}

	/********************************************************************
	*
	* End of Data Utility Functions
	*
	********************************************************************/


	/********************************************************************
	*
	* Decompression utility functions
	*
	********************************************************************/

	/********************************************************************
	* Global variable for decompression
	*********************************************************************/

	unsigned char compression          = 0;		
	unsigned char c_compByte           = 0;
	short int c_currentCounter         = 0;
	unsigned short int c_frameSize     = 0;
	unsigned short int c_frameCounter  = 0;

	void set_compression(unsigned char cmp){
		compression =  cmp;
	}
	unsigned char get_compression(){
		return compression;
	}


	/*********************************************************************
	* decomp_initFrame
	* Get a row of data from compressed data stream
	*
	*********************************************************************/

	short int decomp_initFrame(int bitSize)
	{
		unsigned char compressMethod = 0;
		if(!dataGetByte(&compressMethod, 1, &d_CSU)){
			return 0;
		}
		c_frameSize = (unsigned short int) (bitSize / 8);
		if(bitSize % 8 != 0)
			c_frameSize ++;	

		c_frameCounter = 0;

		switch(compressMethod){
		case 0x00:
			c_currentCounter = -1;
			break;
		case 0x01:
			c_currentCounter = 0;
			c_compByte = 0xFF;
			break;
		case 0x02:
			c_currentCounter = 0;
			c_compByte = 0x00;
			break;
		default:
			return 0;
		}
		return 1;
	}
	short int decomp_getByte(unsigned char *byteOut)
	{
 		if(c_frameCounter >= c_frameSize)
			return 0;
		switch(c_currentCounter){
		case -1:
			if(!dataGetByte(byteOut, 1, &d_CSU))
				return 0;
			else {
				c_frameCounter ++;
				return 1;
			}
			break;

		case 0:
			if(!dataGetByte(byteOut, 1, &d_CSU))
				return 0;
			if(*byteOut == c_compByte){
				 if(! decomp_getNum())
					 return 0;

				 c_currentCounter --;
				 c_frameCounter ++;
				 return 1;
			}
			else{
				c_frameCounter ++;
				return 1;
			}
			break;

		default:
			*byteOut = c_compByte;
			c_currentCounter --;
			c_frameCounter ++;
			return 1;
		}
	}

	short int decomp_getNum()
	{
		unsigned char byteIn = 0x80;

		if(!dataGetByte(&byteIn, 1, &d_CSU)){
			return 0;
		}
		else{
			c_currentCounter = (short int) byteIn;
		}

		return 1;
	}


int is_all_zeros(unsigned char* data, int data_len)
{
	int i;
	for (i = 0; i < data_len; i++) 
		if (0 != data[i]) return 0;

	return 1;
}
int algoTryProg64()
{
	int i, rc;
	fpos_t try_pos;
	fpos_t data_pos;

	int all_zeros = -1;		// 0 - zeros collection enabled; -1 - disabled. 
	// TODO: Zero collection needs to be debugged or modified
	// TODO: Using TRANS_NUM > 7 cause timeout.
	//       Probably related with USB native transaction size (64bytes)
	//       In case of TRANS_NUM > 7 the total packet is >64 bytes and thus 
	//       needs to be combined from several poackets. Never tried before.
	//       The same for verification.
	//       Needs to be investigated.

	// Using TRANS_NUM == 7 the total write time is ~1min for 200kB
	// The biggest delay is in transport layer. RX thread uses select and event
	// signaling with 10ms latency (system scheduler). 
	// Increasing the transaction size may reduce the time significantly.
	// Also, MCU uses 1msec delay after write transaction. Probably it may be 
	// reduced as well.

#define PROG64_TRANS_NUM_MAX  7
	uint8_t data_buff[PROG64_TRANS_NUM_MAX * 8];
	int data_buff_wr_idx = 0;

	//  Write 8 bytes and check status in loop
	struct mask_byte wr_tr[] = {
		{0xFF,  0x10},		// STARTTRAN
		{0xFF,		0x12},			// TRANSOUT
		{0xFF,		0x20},			// 32bit
		{0xFF,		0x22},			// ALGODATA
		{0xFF,			0x70},			// ? FPGA WR command ?
		{0xFF,			0x04},
		{0xFF,			0x00},
		{0xFF,			0x00},

		{0xFF,		0x12},			// TRANSOUT
		{0xFF,		0x40},			// 64bit data don't care
		//{0x00,			0x00},	// Read from data file
		//{0x00,			0x00},
		//{0x00,			0x00},
		//{0x00,			0x00},
		//{0x00,			0x00},
		//{0x00,			0x00},
		//{0x00,			0x00},
		//{0x00,			0x00},
		{0xFF,		0x27},			// PROGDATAEH
		{0xFF,	0x1f},			// ENDTRAN

		{0xFF, 0x43},		// LOOP
		{0x00, 0x0A},		// Number of repeats
		{0xFF,		0x40},		// WAIT
		{0x00,		0x01},		// number of miliseconds

		// Prepare buffer for transaction
		{0xFF,		0x10},		// STARTTRAN
		{0xFF,			0x12},		// TRANSOUT
		{0xFF,			0x20},		// 32bit
		{0xFF,			0x22},		// ALGODATA
		{0xFF,				0xF0}, 		// ? FPGA read status command ?
		{0xFF,				0x00},
		{0xFF,				0x00},
		{0xFF,				0x00},
		{0xFF,			0x13},		// TRANSIN
		{0xFF,			0x08},		// ???
		{0xFF,			0x21},		// Mask	cmd
		{0xFF,			0x80},		// Mask data
		{0xFF,			0x22},		// ALGODATA
		{0xFF,			0x00},			// 	Write status - zero expected
		{0xFF,		0x1f},		// ENDTRAN
		{0xFF, 0x44},		// ENDLOOP
	};

	while (data_buff_wr_idx < sizeof(data_buff) && (all_zeros < 128)) {

		rc = fgetpos(algoFilePtr, &try_pos);
		rc = fgetpos(dataFile, &data_pos);

		// Check template
		for (i = 0; i < COUNT_OF(wr_tr); i++) {
			uint8_t data = (unsigned char)fgetc(algoFilePtr);
			if (wr_tr[i].mask == 0) continue;	// ignore variable data
			if (wr_tr[i].data != data) break;
		}

		if (i < COUNT_OF(wr_tr)) {
			// Template missed - rewind file and write transaction if any was hit
			fsetpos(algoFilePtr, &try_pos);
			break;
		}

		// Transaction hit
		// Get data. Borrowed from TRANS_transceive_stream() see "case DATA_TX"
		for (i = 0; i < 8; i++) {
			if (!HLDataGetByte(0x27, &data_buff[data_buff_wr_idx + i], (i == 0) ? (8 * 8) : 0)) {
				return ERROR_INIT_DATA;
			}
		}

		// Check leading zeros
		if (all_zeros == 0) {
			if (is_all_zeros(&data_buff[data_buff_wr_idx], 8)) {
				// Init leading zeros collection
				all_zeros += 8;
			}
			else {
				// No leading zeros. Disable collecting and keep data buffer filling.
				data_buff_wr_idx += 8;
				all_zeros = -1;
			}
		}
		else if (all_zeros > 0) {
			// Zeros collecting is in progress
			if (is_all_zeros(&data_buff[data_buff_wr_idx], 8)) {
				all_zeros += 8;
			}
			else {
				// Some non-zeros received. 
				// Exit from loop, write zeros and last 8 data bytes
				data_buff_wr_idx += 8;
				break;
			}
		}
		else {
			// Zeros collecting disabled. Keep data buffer filling.
			data_buff_wr_idx += 8;
		}
	}

	if (all_zeros > 0) {
		sprintf(g_Message,
			"ProgZeros transaction hit 0x%02X. Algo pos: %-8ld, Data pos: %-8ld)\n",
			all_zeros, try_pos.__pos, data_pos.__pos);
		print_out_string(g_Message);

		rc = reedl_ictrl_sspiem_progZeros(all_zeros);
		if (rc) {
			print_out_string("Can't transmit\n");
			return ERROR_PROC_HARDWARE;
		}
	}

	if (data_buff_wr_idx > 0) {
		sprintf(g_Message, 
			"Prog64 transaction hit 0x%02X. Algo pos: %-8ld, Data pos: %-8ld)\n",
			data_buff_wr_idx, try_pos.__pos, data_pos.__pos);
		print_out_string(g_Message);

		rc = reedl_ictrl_sspiem_prog64(data_buff, data_buff_wr_idx);
		if (rc) {
			print_out_string("Can't transmit\n");
			return ERROR_PROC_HARDWARE;
		}
	}

	if (data_buff_wr_idx == 0) {
		return 0;
	}
	else if (data_buff_wr_idx < sizeof(data_buff)) {
		// Transaction not fully set. Next transaction is another type.
		return 0;
	}

	// if (data_buff_wr_idx == sizeof(data_buff)) {
	// Buffer is fully set. Let's try continue prog64
	return 1;
}


int algoTryVerify64()
{
	int i, rc;
	fpos_t try_pos;
	fpos_t data_pos;

#define VERIFY64_TRANS_NUM_MAX  7
	uint8_t data_buff[VERIFY64_TRANS_NUM_MAX * 8];	// Data read from *.SED
	uint8_t data_buff_verify[sizeof(data_buff)];	// Data read from FPGA

	int data_buff_wr_idx = 0;

	//  Write 8 bytes and check status in loop
	struct mask_byte wr_tr[] = {
		{0xFF,  0x13},		// TRANSIN
		{0xFF,	0x40},			
		{0xFF,	0x27},			// 32bit
		//{0x00,			0x00},	// Read from data file
		//{0x00,			0x00},
		//{0x00,			0x00},
		//{0x00,			0x00},
		//{0x00,			0x00},
		//{0x00,			0x00},
		//{0x00,			0x00},
		//{0x00,			0x00},
	};

	while (data_buff_wr_idx < sizeof(data_buff)) {

		rc = fgetpos(algoFilePtr, &try_pos);
		rc = fgetpos(dataFile, &data_pos);

		// Check template
		for (i = 0; i < COUNT_OF(wr_tr); i++) {
			uint8_t data = (unsigned char)fgetc(algoFilePtr);
			if (wr_tr[i].mask == 0) continue;	// ignore variable data
			if (wr_tr[i].data != data) break;
		}

		if (i < COUNT_OF(wr_tr)) {
			// Template missed - rewind file and write transaction if any was hit
			fsetpos(algoFilePtr, &try_pos);
			break;
		}

		// Transaction hit
		// Get data. Borrowed from TRANS_transceive_stream() see "case DATA_TX"
		for (i = 0; i < 8; i++) {
			if (!HLDataGetByte(0x27, &data_buff[data_buff_wr_idx], (i == 0) ? (8 * 8) : 0)) {
				return ERROR_INIT_DATA;
			}
			data_buff_wr_idx++;
		}
	}

	if (data_buff_wr_idx > 0) {
		sprintf(g_Message,
			"Verify64 transaction hit 0x%02X. Algo pos: %-8ld, Data pos: %-8ld)\n",
			data_buff_wr_idx, try_pos.__pos, data_pos.__pos);
		print_out_string(g_Message);

		rc = reedl_ictrl_sspiem_verify64(data_buff_verify, data_buff_wr_idx);
		if (rc) {
			print_out_string("Can't transmit\n");
			return ERROR_PROC_HARDWARE;
		}

		if (0 != memcmp(data_buff, data_buff_verify, data_buff_wr_idx)) {
			print_out_string("Data invalid\n");
			return ERROR_VERIFICATION;
		}

	}

	if (data_buff_wr_idx == 0) {
		return 0;
	}
	else if (data_buff_wr_idx < sizeof(data_buff)) {
		// Transaction not fully set. Next transaction is another type.
		return 0;
	}

	// if (data_buff_wr_idx == sizeof(data_buff)) {
	// Buffer is fully set. Let's try continue prog64
	return 1;
}
