/*
 * lcd.c
 *
 *  Created on: Aug 20, 2022
 *      Author: Konstantin
 */

// Data address
#define LCD_DATA     ((uint32_t)0x6C000080)
// Command address
#define LCD_REG      ((uint32_t)0x6C000000)

#include "fsmc.h"

/*
*/
void     LCD_IO_Init(void)
{
	 MX_FSMC_Init();
}

void     LCD_IO_WriteData(uint16_t RegValue)
{
	 *(uint16_t *) (LCD_DATA) = RegValue;
}

void     LCD_IO_WriteReg(uint8_t Reg)
{
	 *(uint16_t *) (LCD_REG) = Reg;
}

uint16_t LCD_IO_ReadData(void)
{
	return *(uint16_t *) (LCD_DATA);
}

void lcd_delay (uint32_t delay)
{
	//uint32_t i;
	//for (i = 0; i < delay; i++) {};
	HAL_Delay(delay);
}

void lcd_delay_us (uint32_t us)
{
	us *=(SystemCoreClock / 1000000) / 5;
    while (us--);
}



