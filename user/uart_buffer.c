//
// Created by MightyPork on 2017/08/24.
//

#include "uart_buffer.h"
#include "uart_driver.h"
#include "uart_handler.h"
#include <esp8266.h>
#include <uart_register.h>

//#define buf_dbg(format, ...) printf(format "\r\n", ##__VA_ARGS__)
#define buf_dbg(format, ...) (void)format

struct UartBuffer {
	uint32 UartBuffSize;
	uint8 *pUartBuff;
	uint8 *pInPos;
	uint8 *pOutPos;
	uint16 Space;
};

static struct UartBuffer *pTxBuffer = NULL;
static struct UartBuffer *pRxBuffer = NULL;

static u8 rxArray[UART_RX_BUFFER_SIZE];
static u8 txArray[UART_TX_BUFFER_SIZE];

static struct UartBuffer *UART_AsyncBufferInit(uint32 buf_size, u8 *buffer);

void ICACHE_FLASH_ATTR UART_AllocBuffers(void)
{
	pTxBuffer = UART_AsyncBufferInit(UART_TX_BUFFER_SIZE, txArray);
	pRxBuffer = UART_AsyncBufferInit(UART_RX_BUFFER_SIZE, rxArray);
}

/******************************************************************************
 * FunctionName : Uart_Buf_Init
 * Description  : tx buffer enqueue: fill a first linked buffer
 * Parameters   : char *pdata - data point  to be enqueue
 * Returns      : NONE
*******************************************************************************/
static struct UartBuffer *ICACHE_FLASH_ATTR
UART_AsyncBufferInit(uint32 buf_size, u8 *buffer)
{
	uint32 heap_size = system_get_free_heap_size();
	if (heap_size <= buf_size) {
		error("uart buf malloc fail, out of memory");
		return NULL;
	}
	else {
		struct UartBuffer *pBuff = (struct UartBuffer *) malloc(sizeof(struct UartBuffer));
		pBuff->UartBuffSize = buf_size;
		pBuff->pUartBuff = buffer != NULL ? buffer : (uint8 *) malloc(pBuff->UartBuffSize);
		pBuff->pInPos = pBuff->pUartBuff;
		pBuff->pOutPos = pBuff->pUartBuff;
		pBuff->Space = (uint16) pBuff->UartBuffSize;
		return pBuff;
	}
}

static void ICACHE_FLASH_ATTR
UART_AsyncBufferReset(struct UartBuffer *pBuff)
{
	pBuff->pInPos = pBuff->pUartBuff;
	pBuff->pOutPos = pBuff->pUartBuff;
	pBuff->Space = (uint16) pBuff->UartBuffSize;
}

/**
 * Copy data onto Buffer
 * @param pCur - buffer
 * @param pdata - data src
 * @param data_len - data len
 */
static void ICACHE_FLASH_ATTR
UART_WriteToAsyncBuffer(struct UartBuffer *pCur, const char *pdata, uint16 data_len)
{
	if (data_len == 0) return;

	buf_dbg("WTAB %d, space %d", data_len, pCur->Space);

	uint16 tail_len = (uint16) (pCur->pUartBuff + pCur->UartBuffSize - pCur->pInPos);
	if (tail_len >= data_len) {  //do not need to loop back  the queue
		buf_dbg("tail %d, no fold", tail_len);
		memcpy(pCur->pInPos, pdata, data_len);
		pCur->pInPos += (data_len);
		pCur->pInPos = (pCur->pUartBuff + (pCur->pInPos - pCur->pUartBuff) % pCur->UartBuffSize);
		pCur->Space -= data_len;
	}
	else {
		buf_dbg("tail only %d, folding", tail_len);
		memcpy(pCur->pInPos, pdata, tail_len);
		buf_dbg("chunk 1, %d", tail_len);
		pCur->pInPos += (tail_len);
		pCur->pInPos = (pCur->pUartBuff + (pCur->pInPos - pCur->pUartBuff) % pCur->UartBuffSize);
		pCur->Space -= tail_len;
		buf_dbg("chunk 2, %d", data_len - tail_len);
		memcpy(pCur->pInPos, pdata + tail_len, data_len - tail_len);
		pCur->pInPos += (data_len - tail_len);
		pCur->pInPos = (pCur->pUartBuff + (pCur->pInPos - pCur->pUartBuff) % pCur->UartBuffSize);
		pCur->Space -= (data_len - tail_len);
	}
	buf_dbg("new space %d", pCur->Space);
}

/******************************************************************************
 * FunctionName : uart_buf_free
 * Description  : deinit of the tx buffer
 * Parameters   : struct UartBuffer* pTxBuff - tx buffer struct pointer
 * Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR UART_FreeAsyncBuffer(struct UartBuffer *pBuff)
{
	free(pBuff->pUartBuff);
	free(pBuff);
}

u16 ICACHE_FLASH_ATTR UART_AsyncRxCount(void)
{
	return (u16) (pRxBuffer->UartBuffSize - pRxBuffer->Space);
}

/**
 * Retrieve some data from the RX buffer
 * @param pdata - target
 * @param data_len - max data to retrieve
 * @return real nr of bytes retrieved
 */
uint16 ICACHE_FLASH_ATTR
UART_ReadAsync(char *pdata, uint16 data_len)
{
	uint16 buf_len = (uint16) (pRxBuffer->UartBuffSize - pRxBuffer->Space);
	uint16 tail_len = (uint16) (pRxBuffer->pUartBuff + pRxBuffer->UartBuffSize - pRxBuffer->pOutPos);
	uint16 len_tmp = 0;
	len_tmp = ((data_len > buf_len) ? buf_len : data_len);
	if (pRxBuffer->pOutPos <= pRxBuffer->pInPos) {
		memcpy(pdata, pRxBuffer->pOutPos, len_tmp);
		pRxBuffer->pOutPos += len_tmp;
		pRxBuffer->Space += len_tmp;
	}
	else {
		if (len_tmp > tail_len) {
			memcpy(pdata, pRxBuffer->pOutPos, tail_len);
			pRxBuffer->pOutPos += tail_len;
			pRxBuffer->pOutPos = (pRxBuffer->pUartBuff +
								  (pRxBuffer->pOutPos - pRxBuffer->pUartBuff) % pRxBuffer->UartBuffSize);
			pRxBuffer->Space += tail_len;

			memcpy(pdata + tail_len, pRxBuffer->pOutPos, len_tmp - tail_len);
			pRxBuffer->pOutPos += (len_tmp - tail_len);
			pRxBuffer->pOutPos = (pRxBuffer->pUartBuff +
								  (pRxBuffer->pOutPos - pRxBuffer->pUartBuff) % pRxBuffer->UartBuffSize);
			pRxBuffer->Space += (len_tmp - tail_len);
		}
		else {
			//os_printf("case 3 in rx deq\n\r");
			memcpy(pdata, pRxBuffer->pOutPos, len_tmp);
			pRxBuffer->pOutPos += len_tmp;
			pRxBuffer->pOutPos = (pRxBuffer->pUartBuff +
								  (pRxBuffer->pOutPos - pRxBuffer->pUartBuff) % pRxBuffer->UartBuffSize);
			pRxBuffer->Space += len_tmp;
		}
	}
	// this maybe shouldnt be here??
	if (pRxBuffer->Space >= UART_FIFO_LEN) {
		uart_rx_intr_enable(UART0);
	}
	return len_tmp;
}

//move data from uart fifo to rx buffer
void UART_RxFifoCollect(void)
{
	uint8 fifo_len, buf_idx;
	uint8 fifo_data;
	fifo_len = (uint8) ((READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT);
	if (fifo_len >= pRxBuffer->Space) {
		fifo_len = (uint8) (pRxBuffer->Space - 1);
		UART_WriteChar(UART1, '#', 10);
		// discard contents of the FIFO - would loop forever
		buf_idx = 0;
		while (buf_idx < fifo_len) {
			buf_idx++;
			fifo_data = (uint8) (READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
			(void)fifo_data; // pretend we use it
		}
		return;
	}

	buf_idx = 0;
	while (buf_idx < fifo_len) {
		buf_idx++;
		fifo_data = (uint8) (READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
		*(pRxBuffer->pInPos++) = fifo_data;
		if (pRxBuffer->pInPos == (pRxBuffer->pUartBuff + pRxBuffer->UartBuffSize)) {
			pRxBuffer->pInPos = pRxBuffer->pUartBuff;
		}
	}
	pRxBuffer->Space -= fifo_len;
//
	// this is called by the processing routine, no need here
//		if (pRxBuffer->Space >= UART_FIFO_LEN) {
//			uart_rx_intr_enable(UART0);
//		}
}

u16 ICACHE_FLASH_ATTR UART_AsyncTxGetEmptySpace(void)
{
	return pTxBuffer->Space;
}

u16 ICACHE_FLASH_ATTR UART_AsyncTxCount(void)
{
	return (u16) (pTxBuffer->UartBuffSize - pTxBuffer->Space);
}

/**
 * Schedule data to be sent
 * @param pdata
 * @param data_len - can be -1 for strlen
 */
void ICACHE_FLASH_ATTR
UART_SendAsync(const char *pdata, int data_len)
{
	size_t real_len = (data_len) <= 0 ? strlen(pdata) : (size_t) data_len;

	buf_dbg("Send Async %d", real_len);
	if (real_len <= pTxBuffer->Space) {
		buf_dbg("accepted, space %d", pTxBuffer->Space);
		UART_WriteToAsyncBuffer(pTxBuffer, pdata, (uint16) real_len);
	}
	else {
		buf_dbg("FULL!");
		UART_WriteChar(UART1, '=', 10);
	}

	// Here we enable TX empty interrupt that will take care of sending the content
	SET_PERI_REG_MASK(UART_CONF1(UART0), (UART_TX_EMPTY_THRESH_VAL & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S);
	SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
}


//--------------------------------
static void UART_TxFifoEnq(struct UartBuffer *pTxBuff, uint8 data_len, uint8 uart_no)
{
	uint8 i;
	for (i = 0; i < data_len; i++) {
		WRITE_PERI_REG(UART_FIFO(uart_no), *(pTxBuff->pOutPos++));
		if (pTxBuff->pOutPos == (pTxBuff->pUartBuff + pTxBuff->UartBuffSize)) {
			pTxBuff->pOutPos = pTxBuff->pUartBuff;
		}
	}
	pTxBuff->pOutPos = (pTxBuff->pUartBuff + (pTxBuff->pOutPos - pTxBuff->pUartBuff) % pTxBuff->UartBuffSize);
	pTxBuff->Space += data_len;
}

volatile bool next_empty_it_only_for_notify = false;
/******************************************************************************
 * FunctionName : TxFromBuffer
 * Description  : get data from the tx buffer and fill the uart tx fifo, co-work with the uart fifo empty interrupt
 * Parameters   : uint8 uart_no - uart port num
 * Returns      : NONE
*******************************************************************************/
void UART_DispatchFromTxBuffer(uint8 uart_no)
{
	const uint8 tx_fifo_len = (uint8) ((READ_PERI_REG(UART_STATUS(uart_no)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
	const uint8 fifo_remain = (uint8) (UART_FIFO_LEN - tx_fifo_len);
	uint8 len_tmp;
	uint16 data_len;

	data_len = (uint16) (pTxBuffer->UartBuffSize - pTxBuffer->Space);
	buf_dbg("rem %d",data_len);
	if (data_len > fifo_remain) {
		len_tmp = fifo_remain;
		UART_TxFifoEnq(pTxBuffer, len_tmp, uart_no);
		SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
	}
	else {
		len_tmp = (uint8) data_len;
		UART_TxFifoEnq(pTxBuffer, len_tmp, uart_no);

		// We get one more IT after fifo ends even if we have 0 more bytes,
		// for notify. Otherwise we would say we have space while the FIFO
		// was still running
		if (next_empty_it_only_for_notify) {
			notify_empty_txbuf();
			next_empty_it_only_for_notify = 0;
		} else {
			// Done sending
			next_empty_it_only_for_notify = 1;
			SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
		}
	}
}
