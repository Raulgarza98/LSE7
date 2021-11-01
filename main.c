/*
 * main.c
 *
 * Created: 10/12/2021 9:23:39 AM
 * Author : raul9
 */ 


#include "sam.h"
#include "myprintf.h"
#include "uart.h"
#include "spi.h"

#define RXBUFSIZE 0x400
#define LENGTH_R1 0x03
#define LENGTH_R3 0x07
#define LENGTH_R7 0x07

void initCycles(void);

#define SIZE_SD_CMD 0x06
#define kCMD00 0x40
#define kCMD08 0x48
#define kCMD55 0x77
#define kCMD41 0x69
#define kCMD58 0x7A

const uint8_t CMD00[SIZE_SD_CMD]  ={0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
const uint8_t CMD08[SIZE_SD_CMD]  ={0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};
uint8_t CMD17[SIZE_SD_CMD]  ={0x51, 0x00, 0x00, 0x00, 0x00, 0x01};
uint8_t CMD172[SIZE_SD_CMD]  ={0x51, 0x00, 0x00, 0x08, 0x00, 0x01};
const uint8_t CMD18[SIZE_SD_CMD]  ={0x52, 0x00, 0x00, 0x00, 0x00, 0x01};
const uint8_t CMD55[SIZE_SD_CMD]  ={0x77, 0x00, 0x00, 0x00, 0x00, 0x65};
const uint8_t CMD41[SIZE_SD_CMD] = {0x69, 0x40, 0x00, 0x00, 0x00, 0x77};

const uint8_t CMD58[SIZE_SD_CMD] = {0x7A, 0x00, 0x00, 0x00, 0x00, 0x75};

uint8_t RxBuffer[RXBUFSIZE];

uint32_t spiXchg(const uint8_t * send_buff, uint32_t bc, uint8_t * receive_buff );
void rcvr_datablock(const uint8_t * send_buff, uint32_t lba, uint8_t * receive_buff, uint32_t bs );

int main(void)
{
	bool FlagCMD00 = false, FlagCMD08 = false, FlagV = false, isReady = false;
	int mem;
	uint32_t temp = 0xFF;
	/* Initialize the SAM system */
	SystemInit();

	UARTInit();
	spiInit();

	initCycles();
	myprintf("\n");
	
	myprintf("CMD00 send\n");
	while(FlagCMD00 == false){
		spiXchg( CMD00, SIZE_SD_CMD, RxBuffer );  /* reset instruction */
		
		for(int i=0; i<LENGTH_R1; i++){
			if(RxBuffer[i] == 0x01){
				FlagCMD00 = true;
			}
		}
	}
	myprintf("\n");
	
	myprintf("CMD08 send\n");
	spiXchg(CMD08, SIZE_SD_CMD, RxBuffer); /*request the contents of the operating conditions register for the connected card.*/
	for(int i=0; i<LENGTH_R7; i++){
		if(RxBuffer[i] == 1 || RxBuffer[i] == 5){
			FlagCMD08 = true;
		}
	}
	myprintf("\n");
	
	if(FlagCMD08 == true){
		for(int i=0; i<LENGTH_R7; i++){
			if(RxBuffer[i] == 0xaa){
				FlagV = true;
				mem = i;
			}
		}
		
		if(FlagV == false){
			myprintf("\nUnusable Card\n");
			goto EXIT;
			}else{
			if(RxBuffer[mem-1] != 1){
				myprintf("\nUnusable Card\n");
				goto EXIT;
			}
		}
	}
	
	if(FlagCMD08 == false){
		myprintf("CMD58 send\n");
		spiXchg( CMD58, SIZE_SD_CMD, RxBuffer);
		myprintf("\n");
		
		if(!(RxBuffer[3] == 0xff || RxBuffer[4] == 0xff || RxBuffer[5] == 0xff)){
			myprintf("\nUnusable Card\n");
			goto EXIT;
		}
	}
	
	Command:
	myprintf("CMD55 send\n");
	spiXchg(CMD55, SIZE_SD_CMD, RxBuffer);
	myprintf("\n");
	
	myprintf("CMD41 send\n");
	spiXchg(CMD41, SIZE_SD_CMD, RxBuffer);
	myprintf("\n");
	
	for(int i=0; i<LENGTH_R1-1; i++){
		if(RxBuffer[i] == 0){
			isReady = true;
		}
	}
	
	if(isReady == false) {
		goto Command;
	}
	
	if(FlagCMD08 == false){
		myprintf("Standard Capacity Card");
		myprintf("\n");
	}
	
	if(FlagCMD08 == true){
		myprintf("CMD58 send\n");
		spiXchg( CMD58, SIZE_SD_CMD, RxBuffer);
		myprintf("\n");
		
		if(RxBuffer[0] == 1 || RxBuffer[1] == 1){
			myprintf("High Capacity Card");
			} else if(RxBuffer[0] == 0 || RxBuffer[1] == 0){
			myprintf("Standard Capacity Card");
		}
		myprintf("\n");
	}
	
	rcvr_datablock(CMD17, 0x00000000, RxBuffer, 0x200);
	
	EXIT:
	myprintf("\nDone\n");

}

void initCycles(void){
	uint32_t i;
	REG_PORT_OUTSET0 = PORT_PA18;
	for(i=0;i<76;i++)
	spiSend(0xFF);
}

//ADD spiXchg FUNCTION TO THE “spi.c” FILE

void CleanBuff (uint8_t * receive_buff,  uint32_t bc){
	uint32_t i;
	
	for(i=0; i< bc; i++) {
		receive_buff[i] = 0;
	}
}

uint32_t spiXchg(const uint8_t * send_buff, uint32_t bc, uint8_t * receive_buff ) {
	uint8_t temp = 0xFF;
	uint32_t i;
	uint8_t temp_cmd = send_buff[0];
	
	CleanBuff(receive_buff, bc);
	
	myprintf("Data received while sending: ");
	REG_PORT_OUTCLR0 = PORT_PA18;
	for(i=0; i< bc; i++) {
		temp = spiSend(*(send_buff++));
		myprintf(" %x", temp);
	}
	myprintf("\n");
	myprintf("Command response: ");
	switch(temp_cmd) {
		case kCMD00 : // Software Reset
		for(i=0; i<LENGTH_R1; i++) {
			temp = spiSend(0xFF);
			receive_buff[i] = temp;
			myprintf(" %x", temp);
		}
		myprintf("\n");
		break;
		case kCMD08 : // Verificar SD Ver. 2+
		for(i=0; i<LENGTH_R7; i++) {
			temp = spiSend(0xFF);
			receive_buff[i] = temp;
			myprintf(" %x", temp);
		}
		myprintf("\n");
		break;
		case kCMD41 : // Inicializar SDC
		for(i=0; i<LENGTH_R1-1; i++) {
			temp = spiSend(0xFF);
			receive_buff[i] = temp;
			myprintf(" %x", temp);
		}
		spiSend(0xFF);
		myprintf("\n");
		break;
		case kCMD55 : // Secuencia de comandos ACMD
		for(i=0; i<LENGTH_R1; i++) {
			temp = spiSend(0xFF);
			receive_buff[i] = temp;
			myprintf(" %x", temp);
		}
		myprintf("\n");
		break;
		case kCMD58 : // Secuencia de comandos ACMD
		for(i=0; i<LENGTH_R3; i++) {
			temp = spiSend(0xFF);
			receive_buff[i] = temp;
			myprintf(" %x", temp);
		}
		myprintf("\n");
		break;
		default :
		myprintf("\n Error in CMD");
	}
	REG_PORT_OUTSET0 = PORT_PA18;
	return(temp);
}

void rcvr_datablock(const uint8_t * send_buff, uint32_t lba, uint8_t * receive_buff, uint32_t bs ) {
	uint8_t temp = 0xFF;
	uint32_t i;
	
	REG_PORT_OUTCLR0 = PORT_PA18;
	myprintf("\n\n");

//  CMD17
	temp = send_buff[0]; 
	temp = spiSend(temp);
	myprintf(" %x", temp);
	
	temp = ((uint8_t*)&lba)[3];
	temp = spiSend(temp);
	myprintf(" %x", temp);
	
	temp = ((uint8_t*)&lba)[2];
	temp = spiSend(temp);
	myprintf(" %x", temp);

	temp = ((uint8_t*)&lba)[1];
	temp = spiSend(temp);
	myprintf(" %x", temp);
	
	temp = ((uint8_t*)&lba)[0];
	temp = spiSend(temp);
	myprintf(" %x", temp);

	temp = send_buff[5];
	temp = spiSend(temp);
	myprintf(" %x", temp);

	// Reading to find the beginning of the sector

	temp = spiSend(0xFF);
	while(temp != 0xFE){
		temp = spiSend(0xFF);
		myprintf(" %x", temp);
	}
	
	// Receiving the memory sector/block
	
	myprintf("\n\n");
	for(i=0; i< bs; i++) {
		while(SERCOM1->SPI.INTFLAG.bit.DRE == 0);
		SERCOM1->SPI.DATA.reg = 0xFF;
		while(SERCOM1->SPI.INTFLAG.bit.TXC == 0);
		while(SERCOM1->SPI.INTFLAG.bit.RXC == 0);
		temp = SERCOM1->SPI.DATA.reg;
		*(receive_buff++) = temp;
		myprintf(" %x", temp);
	}
	REG_PORT_OUTSET0 = PORT_PA18;
	myprintf("\n\n");
}