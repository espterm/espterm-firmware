//
// Created by MightyPork on 2017/08/24.
//

#include "uart_buffer.h"
#include "uart_driver.h"
#include "uart_handler.h"
#include <esp8266.h>
#include <uart_register.h>

#define UART_TX_BUFFER_SIZE 512  //Ring buffer length of tx buffer
#define UART_RX_BUFFER_SIZE 512 //Ring buffer length of rx buffer

struct UartBuffer {
	uint32 UartBuffSize;
	uint8 *pUartBuff;
	uint8 *pInPos;
	uint8 *pOutPos;
	uint16 Space;
};

static struct UartBuffer *pTxBuffer = NULL;
static struct UartBuffer *pRxBuffer = NULL;

static struct UartBuffer *UART_AsyncBufferInit(uint32 buf_size);

void ICACHE_FLASH_ATTR UART_AllocBuffers(void)
{
	pTxBuffer = UART_AsyncBufferInit(UART_TX_BUFFER_SIZE);
	pRxBuffer = UART_AsyncBufferInit(UART_RX_BUFFER_SIZE);
}

/******************************************************************************
 * FunctionName : Uart_Buf_Init
 * Description  : tx buffer enqueue: fill a first linked buffer
 * Parameters   : char *pdata - data point  to be enqueue
 * Returns      : NONE
*******************************************************************************/
static struct UartBuffer *ICACHE_FLASH_ATTR
UART_AsyncBufferInit(uint32 buf_size)
{
	uint32 heap_size = system_get_free_heap_size();
	if (heap_size <= buf_size) {
		error("uart buf malloc fail, out of memory");
		return NULL;
	}
	else {
		struct UartBuffer *pBuff = (struct UartBuffer *) os_malloc(sizeof(struct UartBuffer));
		pBuff->UartBuffSize = buf_size;
		pBuff->pUartBuff = (uint8 *) os_malloc(pBuff->UartBuffSize);
		pBuff->pInPos = pBuff->pUartBuff;
		pBuff->pOutPos = pBuff->pUartBuff;
		pBuff->Space = (uint16) pBuff->UartBuffSize;
		return pBuff;
	}
}


/**
 * Copy data onto Buffer
 * @param pCur - buffer
 * @param pdata - data src
 * @param data_len - data len
 */
static void UART_WriteToAsyncBuffer(struct UartBuffer *pCur, const char *pdata, uint16 data_len)
{
	if (data_len == 0) return;

	uint16 tail_len = (uint16) (pCur->pUartBuff + pCur->UartBuffSize - pCur->pInPos);
	if (tail_len >= data_len) {  //do not need to loop back  the queue
		os_memcpy(pCur->pInPos, pdata, data_len);
		pCur->pInPos += (data_len);
		pCur->pInPos = (pCur->pUartBuff + (pCur->pInPos - pCur->pUartBuff) % pCur->UartBuffSize);
		pCur->Space -= data_len;
	}
	else {
		os_memcpy(pCur->pInPos, pdata, tail_len);
		pCur->pInPos += (tail_len);
		pCur->pInPos = (pCur->pUartBuff + (pCur->pInPos - pCur->pUartBuff) % pCur->UartBuffSize);
		pCur->Space -= tail_len;
		os_memcpy(pCur->pInPos, pdata + tail_len, data_len - tail_len);
		pCur->pInPos += (data_len - tail_len);
		pCur->pInPos = (pCur->pUartBuff + (pCur->pInPos - pCur->pUartBuff) % pCur->UartBuffSize);
		pCur->Space -= (data_len - tail_len);
	}

}

/******************************************************************************
 * FunctionName : uart_buf_free
 * Description  : deinit of the tx buffer
 * Parameters   : struct UartBuffer* pTxBuff - tx buffer struct pointer
 * Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR UART_FreeAsyncBuffer(struct UartBuffer *pBuff)
{
	os_free(pBuff->pUartBuff);
	os_free(pBuff);
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
		os_memcpy(pdata, pRxBuffer->pOutPos, len_tmp);
		pRxBuffer->pOutPos += len_tmp;
		pRxBuffer->Space += len_tmp;
	}
	else {
		if (len_tmp > tail_len) {
			os_memcpy(pdata, pRxBuffer->pOutPos, tail_len);
			pRxBuffer->pOutPos += tail_len;
			pRxBuffer->pOutPos = (pRxBuffer->pUartBuff +
								  (pRxBuffer->pOutPos - pRxBuffer->pUartBuff) % pRxBuffer->UartBuffSize);
			pRxBuffer->Space += tail_len;

			os_memcpy(pdata + tail_len, pRxBuffer->pOutPos, len_tmp - tail_len);
			pRxBuffer->pOutPos += (len_tmp - tail_len);
			pRxBuffer->pOutPos = (pRxBuffer->pUartBuff +
								  (pRxBuffer->pOutPos - pRxBuffer->pUartBuff) % pRxBuffer->UartBuffSize);
			pRxBuffer->Space += (len_tmp - tail_len);
		}
		else {
			//os_printf("case 3 in rx deq\n\r");
			os_memcpy(pdata, pRxBuffer->pOutPos, len_tmp);
			pRxBuffer->pOutPos += len_tmp;
			pRxBuffer->pOutPos = (pRxBuffer->pUartBuff +
								  (pRxBuffer->pOutPos - pRxBuffer->pUartBuff) % pRxBuffer->UartBuffSize);
			pRxBuffer->Space += len_tmp;
		}
	}
	if (pRxBuffer->Space >= UART_FIFO_LEN) {
		uart_rx_intr_enable(UART0);
	}
	return len_tmp;
}

//move data from uart fifo to rx buffer
void UART_RxFifoDeq(void)
{
	uint8 fifo_len, buf_idx;
	uint8 fifo_data;
	fifo_len = (uint8) ((READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT);
	if (fifo_len >= pRxBuffer->Space) {
		os_printf("buf full!!!\n\r");
	}
	else {
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
		if (pRxBuffer->Space >= UART_FIFO_LEN) {
			//os_printf("after rx enq buf enough\n\r");
			uart_rx_intr_enable(UART0);
		}
	}
}

/**
 * Schedule data to be sent
 * @param pdata
 * @param data_len
 */
void ICACHE_FLASH_ATTR
UART_SendAsync(char *pdata, uint16 data_len)
{
	if (pTxBuffer == NULL) {
		info("\n\rnull, create buffer struct\n\r");
		pTxBuffer = UART_AsyncBufferInit(UART_TX_BUFFER_SIZE);
		if (pTxBuffer != NULL) {
			UART_WriteToAsyncBuffer(pTxBuffer, pdata, data_len);
		}
		else {
			error("uart tx MALLOC no buf \n\r");
		}
	}
	else {
		if (data_len <= pTxBuffer->Space) {
			UART_WriteToAsyncBuffer(pTxBuffer, pdata, data_len);
		}
		else {
			error("UART TX BUF FULL!!!!\n\r");
		}
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

	if (pTxBuffer) {
		data_len = (uint8) (pTxBuffer->UartBuffSize - pTxBuffer->Space);
		if (data_len > fifo_remain) {
			len_tmp = fifo_remain;
			UART_TxFifoEnq(pTxBuffer, len_tmp, uart_no);
			SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
		}
		else {
			len_tmp = (uint8) data_len;
			UART_TxFifoEnq(pTxBuffer, len_tmp, uart_no);
		}
	}
	else {
		error("pTxBuff null \n\r");
	}
}
