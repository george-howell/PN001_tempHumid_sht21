// SHT21CTL.CPP ============================================ //
//
// Simple program that provides a number of functions for 
// reading the temperature and humiditiy from a SHT-21 sensor,
// as well as accessing the user register. Uses the standard 
// linux device handle.
//
// FUNCTIONS
// =========
// readtemp [options1]	-> reads a single temperature value
// readrh [options1]	-> reads a single relative humidity 
// 						   value
// readall [options1]	-> reads both temperature and rh values
// readUser 			-> reads the user register and outputs
// writeuser [options2] -> writes certain user data values 
//
// [options1]			-> -nhm, 'no hold master' option when 
// 						   taking measurements
// 			   			   -c, 'continuous' mode which keeps 
// 						   reading data untill stopped
//
// [options2] 			-> -res, data resolution
// 						   -> 1 = rh 12-bit & temp 14 bit (default)
// 						   -> 2 = rh 8-bit & temp 12 bit
// 						   -> 3 = rh 10-bit & temp 13 bit
// 						   -> 4 = rh 11-bit & temp 11 bit
// 						-> -heat, on-chip heater
//						   -> on = enables heater
// 						   -> off = disables heater (default)
// 						-> -otp, OTP Reload
//						   -> on = enables OTP
// 						   -> off = disables OTP (default)
//
// Created By: George Howell
// Date: 02/2020
//
// ========================================================= //

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

const int BUS_ADDR = 1;
const int NB_READ = 3;
const uint8_t DEV_ADDR  = 0x40;
const uint8_t T_TEMP = 0xE3;
const uint8_t T_RH = 0xE5;
const uint8_t T_TEMP_NHM = 0xF3;
const uint8_t T_RH_NHM = 0xF5;
const uint8_t R_USER = 0xE7;
const uint8_t W_USER = 0xE6;
const uint8_t RESET_ADDR = 0xFE;

// i2c linux device handle
int g_i2cFile;

enum opType {
	MEAS,
	RUSER,
	WUSER,
	RESET
};

enum measType {
	TEMP,
	RH
};

enum subOpType {
	RES_OP,
	HEAT_OP,
	OTP_OP
};

typedef struct _arg {
	enum opType op;
	enum subOpType subOp;
	enum measType meas;
	int bus;
	int numRdBytes;
	int numWrBytes;
	bool nhm;
	bool cont;
	uint8_t wData;
	uint8_t devAddr;
	uint8_t * subAddr;
	uint8_t * outData;
} i2cArgs_s;

// usage statement
void usage(char * progname)
{	
	printf("Usage: %s  readtemp [-options1]\n", progname);
	printf("Usage: %s  readrh [-options1]\n", progname);
	printf("Usage: %s  readall [-options1]\n", progname);
	printf("Usage: %s  readuser\n", progname);
	printf("Usage: %s  writeuser [-options2]\n", progname);
	printf("Usage: %s  reset\n\n", progname);

	printf("[-options1]   [-nhm] -> 'no holds master' mode\n");
	printf("              [-c]   -> continuous reading mode\n");
	printf("[-options2]   [-res mode] -> resolution of measurement\n");
	printf("                             -> 1 = rh 12-bit & temp 14 bit (default)\n");
	printf("                             -> 2 = rh 8-bit & temp 12 bit\n");
	printf("                             -> 3 = rh 10-bit & temp 13 bit\n");
	printf("                             -> 4 = rh 11-bit & temp 11 bit\n");
	printf("              [-heat mode] -> on-chip heater\n");
	printf("                             -> on  = enables heater\n");
	printf("                             -> off = disables heater (default)\n");
	printf("              [-otp mode]  -> otp reload\n");
	printf("                             -> on  = enables opt\n");
	printf("                             -> off = disables otp (default)\n\n");

    exit(EXIT_FAILURE);
}

// open the i2c device
void i2cOpen(int bus)
{	
	// create char array argument
	char buf[32];
	snprintf(buf, 32, "/dev/i2c-%d", bus);

	// open i2c bus
	g_i2cFile = open(buf, O_RDWR);

	// error check if i2c bus is open
 	if (g_i2cFile < 0) {
 		fprintf(stderr, "ERROR: failed to open i2c bus %d", bus);
		exit(1);
	}
}

// close the i2c device
void i2cClose()
{
	close(g_i2cFile);
}

// format and display the data
void fmtDispData (uint8_t * data, enum measType meas) {

	// assign both 8-bit data bytes to single 16-bit value
	uint16_t data16;
	data16 = data[0] << 8;
	data16 |= data[1];

	double fmtData;

	if (meas == TEMP) {
		// convert to temp data
	    fmtData = -46.85+175.72*((double)data16/65536);

	    // print output
	    printf("Temp [degC]: %f", fmtData);
	} else {
		// convert to temp data
	    fmtData = -6+125*((double)data16/65536);

	    // print output
	    printf("Humid [%]: %f", fmtData);
	}

}

// merge current user data and new values for write user operation
uint8_t mergeData(uint8_t * currData, uint8_t newData, enum subOpType subOp) {

	int res, heat, otp;

	if (subOp==RES_OP) {
		res = *currData & ~0x81;
		newData += res;
	}
	else if (subOp==HEAT_OP) {
		heat = *currData & ~0x4;
		newData += heat;
	}
	else {
		otp = *currData & ~0x2;
		newData += otp;
	}

	return newData;

}

// format read user data
void fmtUserData (uint8_t * userData) {

	int res, batt, heat, otp;

	res = *userData & 0x81;
	batt = *userData & 0x40;
	heat = *userData & 0x4;
	otp = *userData & 0x2;

	char resDisp [36];
	char battDisp [14];
	char heatDisp [20];
	char otpDisp [20];

	if (!res) {
		strcpy(resDisp,"RH: 12-bit & Temp: 14-bit (Default)");
	}
	else if (res==1) {
		strcpy(resDisp,"RH: 8-bit & Temp: 12-bit");
	}
	else if (res==0x80) {
		strcpy(resDisp,"RH: 10-bit & Temp: 13-bit");
	}
	else {
		strcpy(resDisp,"RH: 11-bit & Temp: 11-bit");
	}

	if (!batt) {
		strcpy(battDisp,"Good (>2.5V)");
	} 
	else {
		strcpy(battDisp,"Low (<2.5V)");
	}

	if (!heat) {
		strcpy(heatDisp,"Disabled (Default)");
	} 
	else {
		strcpy(heatDisp,"Enabled");
	}

	if (!otp) {
		strcpy(otpDisp,"Enabled");
	} 
	else {
		strcpy(otpDisp,"Disabled  (Default)");
	}

	// display output
	printf("User Reg    : 0x%04x\n", *userData);
	printf("Resolution  : %s\n", resDisp);
	printf("Src Voltage : %s\n", battDisp);
	printf("Chip Heater : %s\n", heatDisp);
	printf("OTP Reload  : %s\n", otpDisp);

}

// write data
void writeData (uint8_t devAddr, uint8_t * wrData, int numWrBytes) {

	// sets the device address
	if (ioctl(g_i2cFile, I2C_SLAVE, devAddr) < 0) {
		fprintf(stderr,"ERROR: failed to set the i2c address 0x%02x\n", devAddr);
		exit(1);
	}

	// writes to the command address
	if (write(g_i2cFile, wrData, numWrBytes) != numWrBytes) {
		fprintf(stderr,"ERROR: i2c write failed\n");
	}
	
}

// read data
uint8_t * readData (int numRdBytes, bool nhm) {

	if (nhm) {
		sleep(1);
	}

	uint8_t * tmpData;
	tmpData = (uint8_t*) malloc(sizeof(uint8_t) * numRdBytes);
	if (!tmpData) {
		fprintf(stderr,"ERROR: Failed to allocate output data buffer\n");
		exit(1);
	}

    // read bytes of data from i2c bus
    if (read(g_i2cFile, tmpData, numRdBytes) != numRdBytes) {
    	fprintf(stderr,"ERROR: Failed to read i2c data\n");
        exit(1);
    }

	return tmpData;

}

// parse input arguments, output format for i2c
i2cArgs_s * parseArgs (int argc, char ** argv) {

	// create i2c argument structure and reserve memory
	i2cArgs_s * i2cArgs = (i2cArgs_s*) malloc(sizeof(i2cArgs_s));

	i2cArgs->subAddr = (uint8_t*) malloc(sizeof(uint8_t));

	// initalise i2c data
	i2cArgs->op = RESET;
	i2cArgs->subOp = RES_OP;
	i2cArgs->meas = TEMP;
	i2cArgs->bus = 1;
	i2cArgs->numRdBytes = 1;
	i2cArgs->numWrBytes = 1;
	i2cArgs->wData = 0x0;
	i2cArgs->nhm = false;
	i2cArgs->cont = false;

	// set static device address
	i2cArgs->devAddr = DEV_ADDR;
	i2cArgs->bus = BUS_ADDR;

	// error check the number of input arguments
	if (argc<2 || argc>5) {
		fprintf(stderr,"ERROR: incorrect number of arguments\n");
		usage(argv[0]);
	}

	// initialise flag to allow options
	bool optFlg1 = false;
	bool optFlg2 = false;

	// determines what the command is
	if (!strcmp(argv[1], "readtemp")) {
		*i2cArgs->subAddr = T_TEMP;
		i2cArgs->numRdBytes = NB_READ;
		i2cArgs->op = MEAS;
		i2cArgs->meas = TEMP;
		optFlg1 = true;
	} else if (!strcmp(argv[1], "readrh")) {
		*i2cArgs->subAddr = T_RH;
		i2cArgs->numRdBytes = NB_READ;
		i2cArgs->op = MEAS;
		i2cArgs->meas = RH;
		optFlg1 = true;
	} else if (!strcmp(argv[1], "readall")) {
		*i2cArgs->subAddr = T_TEMP;
		i2cArgs->numRdBytes = NB_READ;
		i2cArgs->op = MEAS;
		optFlg1 = true;
	} else if (!strcmp(argv[1], "readuser")) {
		*i2cArgs->subAddr = R_USER;
		i2cArgs->op = RUSER;
	} else if (!strcmp(argv[1], "writeuser")) {
		*i2cArgs->subAddr = W_USER;
		i2cArgs->op = WUSER;
		i2cArgs->numWrBytes = 2;
		optFlg2 = true;
	} else if (!strcmp(argv[1], "reset")) {
		*i2cArgs->subAddr = RESET_ADDR;
		i2cArgs->op = RESET;
	} else if (!strcmp(argv[1], "usage")) {
		usage(argv[0]);
	} else {
		fprintf(stderr,"ERROR: invalid operation\n");
		usage(argv[0]);
	}

	// determines option 1 arguments
	if (optFlg1) {

		// TODO - make this into for loop
		if (argc>2) {
			if (!strcmp(argv[2], "-nhm")) {
				i2cArgs->nhm = true;
				if (!strcmp(argv[1], "readrh")) {
					*i2cArgs->subAddr = T_RH_NHM;
				} else {
					*i2cArgs->subAddr = T_TEMP_NHM;
				}
			} else if (!strcmp(argv[2], "-c")) {
				i2cArgs->cont = true;
			} else {
				fprintf(stderr,"ERROR: invalid operation\n");
				usage(argv[0]);
			}
		}

		if (argc>3) {
			if (!strcmp(argv[3], "-nhm")) {
				i2cArgs->nhm = true;
				if (!strcmp(argv[1], "readrh")) {
					*i2cArgs->subAddr = T_RH_NHM;
				} else {
					*i2cArgs->subAddr = T_TEMP_NHM;
				}
			} else if (!strcmp(argv[3], "-c")) {
				i2cArgs->cont = true;
			} else {
				fprintf(stderr,"ERROR: invalid operation\n");
				usage(argv[0]);
			}
		}
	}

	if (optFlg2) {
		if (argc>2) {
			if (!strcmp(argv[2], "-res")) {
				i2cArgs->subOp = RES_OP;
				if (!strcmp(argv[3], "1")) {
					i2cArgs->wData = 0x0;
				} else if (!strcmp(argv[3], "2")) {
					i2cArgs->wData = 0x1;
				} else if (!strcmp(argv[3], "3")) {
					i2cArgs->wData = 0x80;
				} else if (!strcmp(argv[3], "4")) {
					i2cArgs->wData = 0x81;
				} else {
					fprintf(stderr,"ERROR: invalid operation\n");
					usage(argv[0]);
				}
			} else if (!strcmp(argv[2], "-heat")) {
				i2cArgs->subOp = HEAT_OP;
				if (!strcmp(argv[3], "on")) {
					i2cArgs->wData = 0x4;
				} else if (!strcmp(argv[3], "off")) {
					i2cArgs->wData = 0x0;
				} else {
					fprintf(stderr,"ERROR: invalid operation\n");
					usage(argv[0]);
				}
			} else if (!strcmp(argv[2], "-otp")) {
				i2cArgs->subOp = OTP_OP;
				if (!strcmp(argv[3], "on")) {
					i2cArgs->wData = 0x0;
				} else if (!strcmp(argv[3], "off")) {
					i2cArgs->wData = 0x2;
				} else {
					fprintf(stderr,"ERROR: invalid operation\n");
					usage(argv[0]);
				}
			} else {
				fprintf(stderr,"ERROR: invalid operation\n");
				usage(argv[0]);
			}
		} else {
			fprintf(stderr,"ERROR: invalid operation\n");
			usage(argv[0]);
		}
	}

	return i2cArgs;
}

int main (int argc, char ** argv) {

	// initialise i2c data structure
	i2cArgs_s * i2cArgs;

	// parse input arguments and form i2c data structure
	i2cArgs = parseArgs(argc, argv);

	// open i2c device
	i2cOpen(i2cArgs->bus);

	bool loopMeas = true;
	uint8_t newUsrData;
	uint8_t * tmpSubAddr = (uint8_t*) malloc(sizeof(uint8_t));
	uint8_t * fullData = (uint8_t*) malloc(sizeof(uint8_t)*2);

	switch (i2cArgs->op) {

		case MEAS:

			while (loopMeas) {

				// temp measurement
				writeData(i2cArgs->devAddr, i2cArgs->subAddr, i2cArgs->numWrBytes);
				i2cArgs->outData = readData(i2cArgs->numRdBytes, i2cArgs->nhm);
				fmtDispData(i2cArgs->outData, i2cArgs->meas);

				if (!strcmp(argv[1], "readall")) {

					i2cArgs->meas = RH;

					// set sub address to rh, no hold master if true
					if (i2cArgs->nhm) {
						*i2cArgs->subAddr = T_RH_NHM;
					} else {
						*i2cArgs->subAddr = T_RH;
					}

					// delay between measurements
					sleep(1);

					// rh measurement
					writeData(i2cArgs->devAddr, i2cArgs->subAddr, i2cArgs->numWrBytes);
					i2cArgs->outData = readData(i2cArgs->numRdBytes, i2cArgs->nhm);
					fmtDispData(i2cArgs->outData, i2cArgs->meas);

					// set sub address back to temp, no hold master if true
					if (i2cArgs->nhm) {
						*i2cArgs->subAddr = T_TEMP_NHM;
					} else {
						*i2cArgs->subAddr = T_TEMP;
					}

					i2cArgs->meas = TEMP;
				}

				// create new line for new measurement data
				printf("\n");

				// break out of loop if not in continuous mode
				if (!i2cArgs->cont) {
					loopMeas = false;
				} else {
					// delay between measurements
					sleep(1);
				}
			}
			break;

		case RUSER:
			// toggle read user register
			writeData(i2cArgs->devAddr, i2cArgs->subAddr, i2cArgs->numWrBytes);

			// read user data register
			i2cArgs->outData = readData(i2cArgs->numRdBytes, i2cArgs->nhm);

			// format and display output
			fmtUserData(i2cArgs->outData);

			break;

		case WUSER:
			// toggle read user data and read current value
			*tmpSubAddr = R_USER;
			writeData(i2cArgs->devAddr, tmpSubAddr, 1);
			i2cArgs->outData = readData(i2cArgs->numRdBytes, i2cArgs->nhm);

			// merge current user data and write data
			newUsrData = mergeData(i2cArgs->outData, i2cArgs->wData, i2cArgs->subOp);

			// cat sub address and user data
			*fullData = *i2cArgs->subAddr;
			*(fullData+1) = newUsrData;

			// toggle user register, then write new value
			writeData(i2cArgs->devAddr, fullData, i2cArgs->numWrBytes);

			break;

		case RESET:
			// toggle reset register
			writeData(i2cArgs->devAddr, i2cArgs->subAddr, i2cArgs->numWrBytes);

			// delay for 1 second (TODO - this is probably excessive)
			sleep(1);

			break;

		default:
			usage(argv[0]);
	}

	// close i2c bus
	i2cClose();

	return 0;

}

	

