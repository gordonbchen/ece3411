#ifndef I2C_H
#define I2C_H

#include <avr/io.h>  
#include <util//twi.h>

// Define the I2C pins
#define I2C_PORT     PORTA
#define I2C_SDA      PIN2_bm
#define I2C_SCL      PIN3_bm
#define TW_WRITE     0
#define TW_READ      1

void TWI_Stop();

void TWI_Host_Initialize();


void TWI_Address(uint8_t Address, uint8_t mode);

int TWI_Transmit_Data(uint8_t data);

uint8_t TWI_Receive_Data();


// Function used for writing to TC74 temp sensor
void TWI_Host_Write(uint8_t Address, uint8_t Command, uint8_t Data);
  
// Function used for reading from TC74 temp sensor
uint8_t TWI_Host_Read(uint8_t Address, uint8_t Command);
   
#endif 


