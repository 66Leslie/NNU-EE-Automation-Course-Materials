#ifndef __DS18B20_H
#define __DS18B20_H 
#include "stm32f4xx.h"   

//IO
#define DS18B20_IO_IN()  {GPIOG->CRH&=0XFFFF0FFF;GPIOG->CRH|=8<<12;}
#define DS18B20_IO_OUT() {GPIOG->CRH&=0XFFFF0FFF;GPIOG->CRH|=3<<12;}
											   
#define	DS18B20_DQ_OUT PGout(11) 
#define	DS18B20_DQ_IN  PGin(11)  
   	
u8 DS18B20_Init(void);
short DS18B20_Get_Temp(void);
void DS18B20_Start(void);
void DS18B20_Write_Byte(u8 dat);
u8 DS18B20_Read_Byte(void);
u8 DS18B20_Read_Bit(void);
u8 DS18B20_Check(void);
void DS18B20_Rst(void);  
#endif

