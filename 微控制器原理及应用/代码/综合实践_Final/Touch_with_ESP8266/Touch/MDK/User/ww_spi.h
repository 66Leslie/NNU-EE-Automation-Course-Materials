#ifndef __SPI_H
#define __SPI_H
#include "stm32f4xx.h"
//----------------------------------------------
#define SPI_FLAG_RXNE           SPI_I2S_FLAG_RXNE
#define SPI_FLAG_TXE            SPI_I2S_FLAG_TXE
#define SPI_FLAG_OVR            SPI_I2S_FLAG_OVR
#define SPI_FLAG_BSY            SPI_I2S_FLAG_BSY
//----------------------------------------------
#define SPI_GetFlagStatus       SPI_I2S_GetFlagStatus
#define SPI_SendData            SPI_I2S_SendData
#define SPI_ReceiveData         SPI_I2S_ReceiveData
void SPI_Configuration(void);
unsigned char SPI1_ReadWrite(unsigned char writedat);

#endif /*__SPI_H*/
