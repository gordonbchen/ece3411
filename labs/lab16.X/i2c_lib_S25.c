#include "i2c_lib_S25.h"

void TWI_Stop()
{
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}

void TWI_Host_Initialize()
{
	//Step 1: Set communication rate
	TWI0.MBAUD = 95; // SCLK = 80kHz; rise time = 10ns

	//Step 2: enable the I2C
	TWI0.MCTRLA |= TWI_ENABLE_bm; 

	// Step 3: force the bus state to IDLE
	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc; 
}



void TWI_Address(uint8_t Address, uint8_t mode)
{
	while (1) {
		//Step 1: Shift Address left by 1
		uint8_t addressWithMode = (Address << 1);

		// Step 2: bitwise OR address with mode
        addressWithMode |= mode;

		// Step 3: Set the Address Register to updated address
        TWI0.MADDR = addressWithMode;

		// Step 4: Create a flag variable and set to correct interrupt flag based on mode
		uint8_t flag;

        if (mode == TW_WRITE) {
            flag = TWI_WIF_bp;  // Set the flag for write mode
        } else {
            flag = TWI_RIF_bp;  // Set the flag for read mode
        }

		// Step 5: Wait for the flag bit is set
		loop_until_bit_is_set(TWI0.MSTATUS, flag);

		// Step 6: Check if the client acknowledged (ACK) else keep infinite loop going trying to send address
		if (TWI0.MSTATUS & TWI_RXACK_bm) {
			TWI_Stop();
		}

		// Step 7: Check for errors not there break out of infinite loop both have to be false
        if (!(TWI0.MSTATUS & TWI_ARBLOST_bm) && !(TWI0.MSTATUS & TWI_BUSERR_bm)) {
            break;
        }
        
	}
}


int TWI_Transmit_Data(uint8_t data)
{
    // Step 1: Start the data transfer by writing the data to register
	TWI0.MDATA = data;

	// Step 2: Wait for the Write Interrupt Flag (WIF) to be set
	loop_until_bit_is_set(TWI0.MSTATUS, TWI_WIF_bp);


	// Step 3: Check for errors after the flag is set two errors
	// Check if there was an arbitration lost or a bus error
    if (TWI0.MSTATUS & TWI_ARBLOST_bm) {
        // Arbitration was lost, return -1 to indicate an error
        return -1;
    }
    else if (TWI0.MSTATUS & TWI_BUSERR_bm) {
        // Bus error occurred, return -1 to indicate an error
        return -1;
    }
    else {
        // No errors, transmission was successful, return 0
        return 0;
    }

}

uint8_t TWI_Receive_Data()
{
	// Step 1: Wait until the Read Interrupt Flag (RIF) is set 
	loop_until_bit_is_set(TWI0.MSTATUS, TWI_RIF_bp);

	// Step 2: Read data register into a variable to hold data
	uint8_t data = TWI0.MDATA;

	// Step 3:  Respond with NACK 
	TWI0.MCTRLB |= TWI_ACKACT_bm;

	// Step 4: return data
	return data;
}

// Function used for writing to TC74 temp sensor
void TWI_Host_Write(uint8_t Address, uint8_t Command, uint8_t Data)
{
	// Step 1: Write the Address
	TWI_Address(Address, TW_WRITE);

	// Step 2: Transmit the Command/Register you want to write to
	TWI_Transmit_Data(Command);

	// Step 3: Transmit the data you want to write to the register
	TWI_Transmit_Data(Data);

	// Step 4: Stop Transmission
	TWI_Stop();
	
}

// Function used for reading from TC74 temp sensor
uint8_t TWI_Host_Read(uint8_t Address, uint8_t Command)
{
	// Step 1: Write the Address
	TWI_Address(Address, TW_WRITE);

	// Step 2: Transmit the Command/Register you want to write to
	TWI_Transmit_Data(Command);

	// Step 3: Transmit the Command/Register you want to write to
	TWI_Address(Address, TW_READ);

	// Step 4: Recieve Data
	uint8_t data = TWI_Receive_Data();

	// Step 5: Stop Transmission
	TWI_Stop();

	// Step 6: Return Data
	return data;
}




    





