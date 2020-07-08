#ifndef SHT21CTL_H
#define SHT21CTL_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "i2c.h"

/* macro definitions */
#define BUS_ADDR 		1
#define NB_READ 		3
#define DEV_ADDR 		0x40
#define T_TEMP 			0xE3
#define T_RH 			0xE5
#define T_TEMP_NHM 		0xF3
#define T_RH_NHM 		0xF5
#define R_USER 			0xE7
#define W_USER 			0xE6
#define RESET_ADDR 		0xFE

/* structure definitions */
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
	uint8_t bus;
	uint32_t numRdBytes;
	uint32_t numWrBytes;
	bool nhm;
	bool cont;
	uint8_t wData;
	uint8_t devAddr;
	uint8_t * subAddr;
	uint8_t * outData;
} i2cArgs_s;

/* function declarations */
void usage(char*);
void fmtDispData (uint8_t*, enum measType);
uint8_t mergeData(uint8_t*, uint8_t, enum subOpType);
void fmtUserData (uint8_t*);
void sleepMillisec(uint32_t, uint32_t);
i2cArgs_s * parseArgs (int, char**);

#endif