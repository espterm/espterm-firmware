/**
 * Low level UART peripheral support functions
 */

/*
 * File : uart.h
 * Copyright (C) 2013 - 2016, Espressif Systems
 * Copyright (C) 2016, Ondřej Hruška (cleaning, modif.)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef UART_APP_H
#define UART_APP_H

#include "uart_register.h"

#include "eagle_soc.h"
#include "c_types.h"


// ===========

// timeout for sending / receiving a char (default)
#define UART_TIMEOUT_US 5000

#define UART_TX_FULL_THRESH_VAL (UART_FIFO_LEN - 2) // if more than this many bytes in queue, don't write more
#define UART_TX_EMPTY_THRESH_VAL 16

// ===========


typedef enum {
	UART0 = 0,
	UART1 = 1
} UARTn;


typedef enum {
	FIVE_BITS = 0x0,
	SIX_BITS = 0x1,
	SEVEN_BITS = 0x2,
	EIGHT_BITS = 0x3
} UartBitsNum4Char;


typedef enum {
	ONE_STOP_BIT             = 0x1,
	ONE_HALF_STOP_BIT        = 0x2,
	TWO_STOP_BIT             = 0x3
} UartStopBitsNum;


typedef enum {
	PARITY_NONE = 0x2,
	PARITY_ODD   = 1,
	PARITY_EVEN = 0
} UartParityMode;


typedef enum {
	PARITY_DIS   = 0,
	PARITY_EN    = 1
} UartExistParity;


typedef enum {
	UART_None_Inverse = 0x0,
	UART_Rxd_Inverse = UART_RXD_INV,
	UART_CTS_Inverse = UART_CTS_INV,
	UART_Txd_Inverse = UART_TXD_INV,
	UART_RTS_Inverse = UART_RTS_INV,
} UART_LineLevelInverse;


typedef enum {
	BIT_RATE_300 = 300,
	BIT_RATE_600 = 600,
	BIT_RATE_1200 = 1200,
	BIT_RATE_2400 = 2400,
	BIT_RATE_4800 = 4800,
	BIT_RATE_9600   = 9600,
	BIT_RATE_19200  = 19200,
	BIT_RATE_38400  = 38400,
	BIT_RATE_57600  = 57600,
	BIT_RATE_74880  = 74880,
	BIT_RATE_115200 = 115200,
	BIT_RATE_230400 = 230400,
	BIT_RATE_460800 = 460800,
	BIT_RATE_921600 = 921600,
	BIT_RATE_1843200 = 1843200,
	BIT_RATE_3686400 = 3686400,
} UartBautRate;


typedef enum {
	NONE_CTRL,
	HARDWARE_CTRL,
	XON_XOFF_CTRL
} UartFlowCtrl;


typedef enum {
	USART_HWFlow_None = 0x0,
	USART_HWFlow_RTS = 0x1,
	USART_HWFlow_CTS = 0x2,
	USART_HWFlow_CTS_RTS = 0x3
} UART_HwFlowCtrl;


typedef enum {
	EMPTY,
	UNDER_WRITE,
	WRITE_OVER
} RcvMsgBuffState;


typedef struct {
	uint32     RcvBuffSize;
	uint8     *pRcvMsgBuff;
	uint8     *pWritePos;
	uint8     *pReadPos;
	uint8      TrigLvl; //JLU: may need to pad
	RcvMsgBuffState  BuffState;
} RcvMsgBuff;


typedef struct {
	uint32   TrxBuffSize;
	uint8   *pTrxBuff;
} TrxMsgBuff;


typedef enum {
	BAUD_RATE_DET,
	WAIT_SYNC_FRM,
	SRCH_MSG_HEAD,
	RCV_MSG_BODY,
	RCV_ESC_CHAR,
} RcvMsgState;


typedef struct {
	UartBautRate     baut_rate;
	UartBitsNum4Char data_bits;
	UartExistParity  exist_parity;
	UartParityMode   parity;
	UartStopBitsNum  stop_bits;
	UartFlowCtrl     flow_ctrl;
	RcvMsgBuff       rcv_buff;
	TrxMsgBuff       trx_buff;
	RcvMsgState      rcv_state;
	int              received;
	int              buff_uart_no;  //indicate which uart use tx/rx buffer
} UartDevice;

// UartDev is defined and initialized in rom code.
extern UartDevice UartDev;


//==============================================

// FIFO used count
#define UART_TxFifoCount(uart_no) ((READ_PERI_REG(UART_STATUS((uart_no))) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT)
#define UART_RxFifoCount(uart_no) ((READ_PERI_REG(UART_STATUS((uart_no))) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT)

STATUS UART_WriteCharCRLF(UARTn uart_no, uint8 c, uint32 timeout_us);
STATUS UART_WriteChar(UARTn uart_no, uint8 c, uint32 timeout_us);
STATUS UART_WriteString(UARTn uart_no, const char *str, uint32 timeout_us);
STATUS UART_WriteBuffer(UARTn uart_no, const uint8 *buffer, size_t len, uint32 timeout_us);

//==============================================

void UART_SetWordLength(UARTn uart_no, UartBitsNum4Char len);
void UART_SetStopBits(UARTn uart_no, UartStopBitsNum bit_num);
void UART_SetLineInverse(UARTn uart_no, UART_LineLevelInverse inverse_mask);
void UART_SetParity(UARTn uart_no, UartParityMode Parity_mode);
void UART_SetBaudrate(UARTn uart_no, uint32 baud_rate);
void UART_SetFlowCtrl(UARTn uart_no, UART_HwFlowCtrl flow_ctrl, uint8 rx_thresh);
void UART_WaitTxFifoEmpty(UARTn uart_no , uint32 time_out_us); //do not use if tx flow control enabled
void UART_ResetFifo(UARTn uart_no);
void UART_ClearIntrStatus(UARTn uart_no, uint32 clr_mask);
void UART_SetIntrEna(UARTn uart_no, uint32 ena_mask);
void UART_SetPrintPort(UARTn uart_no);
bool UART_CheckOutputFinished(UARTn uart_no, uint32 time_out_us);

//==============================================

#endif

