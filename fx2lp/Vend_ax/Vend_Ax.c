//-----------------------------------------------------------------------------
//   File:      bulkloop.c
//   Contents:  Hooks required to implement USB peripheral function.
//
// $Archive: /USB/Examples/Fx2lp/vend_ax/Vend_Ax.c $
// $Date: 9/02/03 7:32p $
// $Revision: 1 $
//
//
//-----------------------------------------------------------------------------
// Copyright 2003, Cypress Semiconductor Corporation
//-----------------------------------------------------------------------------
#pragma NOIV					// Do not generate interrupt vectors

#include "lp.h"
#include "lpregs.h"

extern BOOL	GotSUD;			// Received setup data flag
extern BOOL	Sleep;
extern BOOL	Rwuen;
extern BOOL	Selfpwr;

BYTE	Configuration;		// Current configuration
BYTE	AlternateSetting;	// Alternate settings

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define	VR_UPLOAD		0xc0
#define VR_DOWNLOAD		0x40
#define VR_EEPROM		0xa2 // loads (uploads) small EEPROM
#define	VR_RAM			0xa3 // loads (uploads) external ram
#define VR_GET_CHIP_REV 0xa6 // Rev A, B = 0, Rev C = 2 // NOTE: New TNG Rev
#define VR_RENUM	    0xa8 // renum
#define VR_DB_FX	    0xa9 // Force use of double byte address EEPROM (for FX)
#define VR_I2C_100      0xaa // put the i2c bus in 100Khz mode
#define VR_I2C_400      0xab // put the i2c bus in 400Khz mode

#define SERIAL_ADDR		0x50
#define EP0BUFF_SIZE	0x40

#define GET_CHIP_REV()		((CPUCS >> 4) & 0x00FF) // EzUSB Chip Rev Field

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
BYTE			DB_Addr;					//TPM Dual Byte Address stat
BYTE			I2C_Addr;					//TPM I2C address

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
void EEPROMWrite(WORD addr, BYTE length, BYTE xdata *buf); //TPM EEPROM Write
void EEPROMRead(WORD addr, BYTE length, BYTE xdata *buf);  //TPM EEPROM Read

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//	The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------

void TD_Init(void) 				// Called once at startup
{
	Rwuen = TRUE;				// Enable remote-wakeup
        EZUSB_InitI2C();			// Initialize I2C Bus
}

void TD_Poll(void) 				// Called repeatedly while the device is idle
{
}

BOOL TD_Suspend(void) 			// Called before the device goes into suspend mode
{
	return(TRUE);
}

BOOL TD_Resume(void) 			// Called after the device resumes
{
	return(TRUE);
}

//-----------------------------------------------------------------------------
// Device Request hooks
//	The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------


BOOL DR_GetDescriptor(void)
{
   return(TRUE);
}

BOOL DR_SetConfiguration(void)   // Called when a Set Configuration command is received
{
   Configuration = SETUPDAT[2];
   return(TRUE);            // Handled by user code
}

BOOL DR_GetConfiguration(void)   // Called when a Get Configuration command is received
{
   EP0BUF[0] = Configuration;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);            // Handled by user code
}

BOOL DR_SetInterface(void)       // Called when a Set Interface command is received
{
   AlternateSetting = SETUPDAT[2];
   return(TRUE);            // Handled by user code
}

BOOL DR_GetInterface(void)       // Called when a Set Interface command is received
{
   EP0BUF[0] = AlternateSetting;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);            // Handled by user code
}

BOOL DR_GetStatus(void)
{
   return(TRUE);
}

BOOL DR_ClearFeature(void)
{
   return(TRUE);
}

BOOL DR_SetFeature(void)
{
   return(TRUE);
}

BOOL DR_VendorCmnd(void)
{
	WORD		addr, len, bc;
	WORD		ChipRev;
	WORD i;

	// Determine I2C boot eeprom device address; addr = 0x0 for 8 bit addr eeproms (24LC00)
	I2C_Addr = SERIAL_ADDR | ((I2CS & 0x10) >> 4); // addr=0x01 for 16 bit addr eeprom (LC65)
	// Indicate if it is a dual byte address part
	DB_Addr = (BOOL)(I2C_Addr & 0x01); //TPM: ID1 is 16 bit addr bit - set by rocker sw or jumper

	switch(SETUPDAT[1])
	{ //TPM handle new commands

		case VR_GET_CHIP_REV:
					ChipRev = GET_CHIP_REV();
					*EP0BUF = ChipRev;
					EP0BCH = 0;
					EP0BCL = 1; // Arm endpoint with # bytes to transfer
					EP0CS |= bmHSNAK; // Acknowledge handshake phase of device request
			break;
		case VR_I2C_100:
			I2CTL &= ~bm400KHZ;
			EP0BCH = 0;
			EP0BCL = 0;
			break;
		case VR_I2C_400:
			I2CTL |= bm400KHZ;
			EP0BCH = 0;
			EP0BCL = 0;
			break;
		case VR_RENUM:
					*EP0BUF = 7;
					EP0BCH = 0;
					EP0BCL = 1; // Arm endpoint with # bytes to transfer
					EP0CS |= bmHSNAK; // Acknowledge handshake phase of device request
					EZUSB_Delay(1000);
					EZUSB_Discon(TRUE);		// renumerate until setup received
			break;
		case VR_DB_FX:
			DB_Addr = 0x01;		//TPM: need to assert double byte
			I2C_Addr |= 0x01;	//TPM: need to assert double byte
      		addr = SETUPDAT[2];		// Get address and length
			addr |= SETUPDAT[3] << 8;
			len = SETUPDAT[6];
			len |= SETUPDAT[7] << 8;
			// Is this an upload command ?
			if(SETUPDAT[0] == VR_UPLOAD)
			{
				while(len)					// Move requested data through EP0IN 
				{							// one packet at a time.

               while(EP0CS & bmEPBUSY);

					if(len < EP0BUFF_SIZE)
						bc = len;
					else
						bc = EP0BUFF_SIZE;

					for(i=0; i<bc; i++)
						*(EP0BUF+i) = 0xcd;
					EEPROMRead(addr,(WORD)bc,(WORD)EP0BUF);

					EP0BCH = 0;
					EP0BCL = (BYTE)bc; // Arm endpoint with # bytes to transfer

					addr += bc;
					len -= bc;
				}
			}
			// Is this a download command ?
			else if(SETUPDAT[0] == VR_DOWNLOAD)
			{
				while(len)					// Move new data through EP0OUT 
				{							// one packet at a time.
					// Arm endpoint - do it here to clear (after sud avail)
					EP0BCH = 0;
					EP0BCL = 0; // Clear bytecount to allow new data in; also stops NAKing

					while(EP0CS & bmEPBUSY);

					bc = EP0BCL; // Get the new bytecount

					EEPROMWrite(addr,bc,(WORD)EP0BUF);

					addr += bc;
					len -= bc;
				}
			}
		
		break;
		case VR_RAM:
		  // NOTE: This case falls through !	
		case VR_EEPROM:
			DB_Addr = 0x00;		//TPM: need to assert double byte
			I2C_Addr |= 0x00;	//TPM: need to assert double byte
			addr = SETUPDAT[2];		// Get address and length
			addr |= SETUPDAT[3] << 8;
			len = SETUPDAT[6];
			len |= SETUPDAT[7] << 8;
			// Is this an upload command ?
			if(SETUPDAT[0] == VR_UPLOAD)
			{
				while(len)					// Move requested data through EP0IN 
				{							// one packet at a time.

               while(EP0CS & bmEPBUSY);

					if(len < EP0BUFF_SIZE)
						bc = len;
					else
						bc = EP0BUFF_SIZE;

					// Is this a RAM upload ?
					if(SETUPDAT[1] == VR_RAM)
					{
						for(i=0; i<bc; i++)
							*(EP0BUF+i) = *((BYTE xdata *)addr+i);
					}
					else
					{
						for(i=0; i<bc; i++)
							*(EP0BUF+i) = 0xcd;
						EEPROMRead(addr,(WORD)bc,(WORD)EP0BUF);
					}

					EP0BCH = 0;
					EP0BCL = (BYTE)bc; // Arm endpoint with # bytes to transfer

					addr += bc;
					len -= bc;

				}
			}
			// Is this a download command ?
			else if(SETUPDAT[0] == VR_DOWNLOAD)
			{
				while(len)					// Move new data through EP0OUT 
				{							// one packet at a time.
					// Arm endpoint - do it here to clear (after sud avail)
					EP0BCH = 0;
					EP0BCL = 0; // Clear bytecount to allow new data in; also stops NAKing

					while(EP0CS & bmEPBUSY);

					bc = EP0BCL; // Get the new bytecount

					// Is this a RAM download ?
					if(SETUPDAT[1] == VR_RAM)
					{
						for(i=0; i<bc; i++)
							 *((BYTE xdata *)addr+i) = *(EP0BUF+i);
					}
					else
						EEPROMWrite(addr,bc,(WORD)EP0BUF);

					addr += bc;
					len -= bc;
				}
			}
			break;
	}
	return(FALSE); // no error; command handled OK
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//	The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav(void) interrupt 0
{
   // enable the automatic length feature of the Setup Data Autopointer
   // in case a previous transfer disbaled it
   SUDPTRCTL |= bmSDPAUTO;

   GotSUD = TRUE;            // Set flag
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUDAV;         // Clear SUDAV IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok(void) interrupt 0
{
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUTOK;         // Clear SUTOK IRQ
}

void ISR_Sof(void) interrupt 0
{
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSOF;            // Clear SOF IRQ
}

void ISR_Ures(void) interrupt 0
{
   if (EZUSB_HIGHSPEED())
   {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
   }
   else
   {
      pConfigDscr = pFullSpeedConfigDscr;
      pOtherConfigDscr = pHighSpeedConfigDscr;
   }
   
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmURES;         // Clear URES IRQ
}

void ISR_Susp(void) interrupt 0
{
   Sleep = TRUE;
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUSP;
}

void ISR_Highspeed(void) interrupt 0
{
   if (EZUSB_HIGHSPEED())
   {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
   }
   else
   {
      pConfigDscr = pFullSpeedConfigDscr;
      pOtherConfigDscr = pHighSpeedConfigDscr;
   }

   EZUSB_IRQ_CLEAR();
   USBIRQ = bmHSGRANT;
}

void ISR_Ep0ack(void) interrupt 0
{
}
void ISR_Stub(void) interrupt 0
{
}
void ISR_Ep0in(void) interrupt 0
{
}
void ISR_Ep0out(void) interrupt 0
{
}
void ISR_Ep1in(void) interrupt 0
{
}
void ISR_Ep1out(void) interrupt 0
{
}
void ISR_Ep2inout(void) interrupt 0
{
}
void ISR_Ep4inout(void) interrupt 0
{
}
void ISR_Ep6inout(void) interrupt 0
{
}
void ISR_Ep8inout(void) interrupt 0
{
}
void ISR_Ibn(void) interrupt 0
{
}
void ISR_Ep0pingnak(void) interrupt 0
{
}
void ISR_Ep1pingnak(void) interrupt 0
{
}
void ISR_Ep2pingnak(void) interrupt 0
{
}
void ISR_Ep4pingnak(void) interrupt 0
{
}
void ISR_Ep6pingnak(void) interrupt 0
{
}
void ISR_Ep8pingnak(void) interrupt 0
{
}
void ISR_Errorlimit(void) interrupt 0
{
}
void ISR_Ep2piderror(void) interrupt 0
{
}
void ISR_Ep4piderror(void) interrupt 0
{
}
void ISR_Ep6piderror(void) interrupt 0
{
}
void ISR_Ep8piderror(void) interrupt 0
{
}
void ISR_Ep2pflag(void) interrupt 0
{
}
void ISR_Ep4pflag(void) interrupt 0
{
}
void ISR_Ep6pflag(void) interrupt 0
{
}
void ISR_Ep8pflag(void) interrupt 0
{
}
void ISR_Ep2eflag(void) interrupt 0
{
}
void ISR_Ep4eflag(void) interrupt 0
{
}
void ISR_Ep6eflag(void) interrupt 0
{
}
void ISR_Ep8eflag(void) interrupt 0
{
}
void ISR_Ep2fflag(void) interrupt 0
{
}
void ISR_Ep4fflag(void) interrupt 0
{
}
void ISR_Ep6fflag(void) interrupt 0
{
}
void ISR_Ep8fflag(void) interrupt 0
{
}
void ISR_GpifComplete(void) interrupt 0
{
}
void ISR_GpifWaveform(void) interrupt 0
{
}

void EEPROMWriteByte(WORD addr, BYTE value)
{
	BYTE		i = 0;
	BYTE xdata 	ee_str[3];

	if(DB_Addr)
		ee_str[i++] = MSB(addr);

	ee_str[i++] = LSB(addr);
	ee_str[i++] = value;

	EZUSB_WriteI2C(I2C_Addr, i, ee_str);
   EZUSB_WaitForEEPROMWrite(I2C_Addr);
}


void EEPROMWrite(WORD addr, BYTE length, BYTE xdata *buf)
{
	BYTE	i;
	for(i=0;i<length;++i)
		EEPROMWriteByte(addr++,buf[i]);
}

void EEPROMRead(WORD addr, BYTE length, BYTE xdata *buf)
{
	BYTE		i = 0;
	BYTE		j = 0;
	BYTE xdata 	ee_str[2];

	if(DB_Addr)
		ee_str[i++] = MSB(addr);

	ee_str[i++] = LSB(addr);

	EZUSB_WriteI2C(I2C_Addr, i, ee_str);

//	for(j=0; j < length; j++)
//		*(buf+j) = 0xcd;

	EZUSB_ReadI2C(I2C_Addr, length, buf);
}


