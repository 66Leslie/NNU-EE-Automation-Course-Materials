/**
  ******************************************************************************
  * @file    ili9341.c
  * @author  MCD Application Team
  * @version V1.0.2
  * @date    02-December-2014
  * @brief   This file includes the LCD driver for ILI9341 LCD.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "ili9341.h"

/** @addtogroup BSP
  * @{
  */ 

/** @addtogroup Components
  * @{
  */ 
  
/** @addtogroup ILI9341
  * @brief This file provides a set of functions needed to drive the 
  *        ILI9341 LCD.
  * @{
  */

/** @defgroup ILI9341_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup ILI9341_Private_Defines
  * @{
  */
/**
  * @}
  */ 
  
/** @defgroup ILI9341_Private_Macros
  * @{
  */
/**
  * @}
  */  

/** @defgroup ILI9341_Private_Variables
  * @{
  */ 

LCD_DrvTypeDef   ili9341_drv = 
{
  ili9341_Init,
  ili9341_ReadID,
  ili9341_DisplayOn,
  ili9341_DisplayOff,
  ili9341_SetCursor,
  ili9341_Write_pixel,
  ili9341_Read_pixel,
  0,
  0,
  0,
  ili9341_GetLcdPixelWidth,
  ili9341_GetLcdPixelHeight,
  0,
  0,    
  ili9341_SetLcdRotation,
  ili9341_SwLcdReset,
  ili9341_Lcd_clear,
  ili9341_WriteData,
  ili9341_WriteReg
};

static uint16_t x_size = ILI9341_LCD_PIXEL_WIDTH;;
static uint16_t y_size = ILI9341_LCD_PIXEL_HEIGHT;
static uint8_t __rotation_id = 0;

static uint16_t POINT_COLOR = 0x0000;

/**
  * @}
  */ 
  
/** @defgroup ILI9341_Private_FunctionPrototypes
  * @{
  */

/**
  * @}
  */ 
  
/** @defgroup ILI9341_Private_Functions
  * @{
  */   

/**
  * @brief  Power on the LCD.
  * @param  None
  * @retval None
  */
void ili9341_Init(void)
{
  /* Initialize ILI9341 low level bus layer ----------------------------------*/
  //LCD_IO_Init();

  ili9341_SwLcdReset();
  lcd_delay(1);

//  /* Configure LCD */
//  ili9341_WriteReg(0xCA);
//  ili9341_WriteData(0xC3);
//  ili9341_WriteData(0x08);
//  ili9341_WriteData(0x50);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_POWERB);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0xC1);
//  ili9341_WriteData(0x30);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_POWER_SEQ);
//  ili9341_WriteData(0x64);
//  ili9341_WriteData(0x03);
//  ili9341_WriteData(0x12);
//  ili9341_WriteData(0x81);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_DTCA);
//  ili9341_WriteData(0x85);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0x78);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_POWERA);
//  ili9341_WriteData(0x39);
//  ili9341_WriteData(0x2C);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0x34);
//  ili9341_WriteData(0x02);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_PRC);
//  ili9341_WriteData(0x20);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_DTCB);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0x00);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_FRMCTR1);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0x1B);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_DFC);
//  ili9341_WriteData(0x0A);
//  ili9341_WriteData(0xA2);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_POWER1);
//  ili9341_WriteData(0x10);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_POWER2);
//  ili9341_WriteData(0x10);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_VCOM1);
//  ili9341_WriteData(0x45);
//  ili9341_WriteData(0x15);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_VCOM2);
//  ili9341_WriteData(0x90);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_MAC);
//  ili9341_WriteData(0xC8);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_3GAMMA_EN);
//  ili9341_WriteData(0x00);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_RGB_INTERFACE);
//  ili9341_WriteData(0xC2);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_DFC);
//  ili9341_WriteData(0x0A);
//  ili9341_WriteData(0xA7);
//  ili9341_WriteData(0x27);
//  ili9341_WriteData(0x04);
//  lcd_delay_us(1);
//  /* Colomn address set */
//  ili9341_WriteReg(LCD_COLUMN_ADDR);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0xEF);
//  lcd_delay_us(1);
//  /* Page address set */
//  ili9341_WriteReg(LCD_PAGE_ADDR);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0x01);
//  ili9341_WriteData(0x3F);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_INTERFACE);
//  ili9341_WriteData(0x01);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0x06);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_GRAM);
//  lcd_delay(200);
//
//  ili9341_WriteReg(LCD_GAMMA);
//  ili9341_WriteData(0x01);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_PGAMMA);
//  ili9341_WriteData(0x0F);
//  ili9341_WriteData(0x29);
//  ili9341_WriteData(0x24);
//  ili9341_WriteData(0x0C);
//  ili9341_WriteData(0x0E);
//  ili9341_WriteData(0x09);
//  ili9341_WriteData(0x4E);
//  ili9341_WriteData(0x78);
//  ili9341_WriteData(0x3C);
//  ili9341_WriteData(0x09);
//  ili9341_WriteData(0x13);
//  ili9341_WriteData(0x05);
//  ili9341_WriteData(0x17);
//  ili9341_WriteData(0x11);
//  ili9341_WriteData(0x00);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_NGAMMA);
//  ili9341_WriteData(0x00);
//  ili9341_WriteData(0x16);
//  ili9341_WriteData(0x1B);
//  ili9341_WriteData(0x04);
//  ili9341_WriteData(0x11);
//  ili9341_WriteData(0x07);
//  ili9341_WriteData(0x31);
//  ili9341_WriteData(0x33);
//  ili9341_WriteData(0x42);
//  ili9341_WriteData(0x05);
//  ili9341_WriteData(0x0C);
//  ili9341_WriteData(0x0A);
//  ili9341_WriteData(0x28);
//  ili9341_WriteData(0x2F);
//  ili9341_WriteData(0x0F);
//  lcd_delay_us(1);
//  ili9341_WriteReg(LCD_SLEEP_OUT);
//  lcd_delay(200);
//  ili9341_WriteReg(LCD_DISPLAY_ON);
//  lcd_delay_us(1);
//  /* GRAM start writing */
//  ili9341_WriteReg(LCD_GRAM);
//  lcd_delay(150);

  ili9341_WriteReg(0xCB);//Power Control A
  ili9341_WriteData(0x39);
  ili9341_WriteData(0x2C);
  ili9341_WriteData(0x00);
  ili9341_WriteData(0x34);
  ili9341_WriteData(0x02);
  lcd_delay_us(1);

  ili9341_WriteReg(0xCF);//Power Control B
  ili9341_WriteData(0x00);
  ili9341_WriteData(0xC1);
  ili9341_WriteData(0x30);
  lcd_delay_us(1);

  ili9341_WriteReg(0xE8);//Driver timing control A
  ili9341_WriteData(0x85);
  ili9341_WriteData(0x00);
  ili9341_WriteData(0x78);
  lcd_delay_us(1);

  ili9341_WriteReg(0xEA);//Driver timing control B
  ili9341_WriteData(0x00);
  ili9341_WriteData(0x00);
  lcd_delay_us(1);

  ili9341_WriteReg(0xED);//Power on Sequence control
  ili9341_WriteData(0x64);
  ili9341_WriteData(0x03);
  ili9341_WriteData(0x12);
  ili9341_WriteData(0x81);
  lcd_delay_us(1);

  ili9341_WriteReg(0xF7);//Pump ratio control
  ili9341_WriteData(0x20);
  lcd_delay_us(1);
  ili9341_WriteReg(0xC0);//Power Control 1
  ili9341_WriteData(0x10);
  lcd_delay_us(1);
  ili9341_WriteReg(0xC1);//Power Control 2
  ili9341_WriteData(0x10);
  lcd_delay_us(1);
  ili9341_WriteReg(0xC5);//VCOM Control 1
  ili9341_WriteData(0x3E);
  ili9341_WriteData(0x28);
  lcd_delay_us(1);
  ili9341_WriteReg(0xC7);//VCOM Control 2
  ili9341_WriteData(0x86);
  lcd_delay_us(1);
  ili9341_SetLcdRotation(0);
  lcd_delay_us(1);
  ili9341_WriteReg(0x3A);//Pixel Format Set
  ili9341_WriteData(0x55);//16bit
  lcd_delay_us(1);
  ili9341_WriteReg(0xB1);
  ili9341_WriteData(0x00);
  ili9341_WriteData(0x18);// Частота кадров 79 Гц
  lcd_delay_us(1);
  ili9341_WriteReg(0xB6);//Display Function Control
  ili9341_WriteData(0x08);
  ili9341_WriteData(0x82);
  ili9341_WriteData(0x27);//320 строк
  lcd_delay_us(1);
  ili9341_WriteReg(0xF2);//Enable 3G (пока не знаю что это за режим)
  ili9341_WriteData(0x00);//не включаем
  lcd_delay_us(1);
  ili9341_WriteReg(0x26);//Gamma set
  ili9341_WriteData(0x01);//Gamma Curve (G2.2) (Кривая цветовой гаммы)
  lcd_delay_us(1);
  ili9341_WriteReg(0xE0);//Positive Gamma  Correction
  ili9341_WriteData(0x0F);
  ili9341_WriteData(0x31);
  ili9341_WriteData(0x2B);
  ili9341_WriteData(0x0C);
  ili9341_WriteData(0x0E);
  ili9341_WriteData(0x08);
  ili9341_WriteData(0x4E);
  ili9341_WriteData(0xF1);
  ili9341_WriteData(0x37);
  ili9341_WriteData(0x07);
  ili9341_WriteData(0x10);
  ili9341_WriteData(0x03);
  ili9341_WriteData(0x0E);
  ili9341_WriteData(0x09);
  ili9341_WriteData(0x00);
  lcd_delay_us(1);

  ili9341_WriteReg(0xE1);//Negative Gamma  Correction
  ili9341_WriteData(0x00);
  ili9341_WriteData(0x0E);
  ili9341_WriteData(0x14);
  ili9341_WriteData(0x03);
  ili9341_WriteData(0x11);
  ili9341_WriteData(0x07);
  ili9341_WriteData(0x31);
  ili9341_WriteData(0xC1);
  ili9341_WriteData(0x48);
  ili9341_WriteData(0x08);
  ili9341_WriteData(0x0F);
  ili9341_WriteData(0x0C);
  ili9341_WriteData(0x31);
  ili9341_WriteData(0x36);
  ili9341_WriteData(0x0F);
  lcd_delay_us(1);

  ili9341_WriteReg(0x11);//Выйдем из спящего режим
  lcd_delay(150);
  ili9341_WriteReg(0x29);//Включение дисплея
  ili9341_WriteData(0x2C);
  lcd_delay(150);
}

/**
  * @brief  Enables the Display.
  * @param  None
  * @retval None
  */
void ili9341_DisplayOn(void)
{
  /* Display On */
  ili9341_WriteReg(LCD_DISPLAY_ON);
}

/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
void ili9341_DisplayOff(void)
{
  /* Display Off */
  ili9341_WriteReg(LCD_DISPLAY_OFF);
}

/**
  * @brief  Writes  to the selected LCD register.
  * @param  LCD_Reg: address of the selected register.
  * @retval None
  */
void ili9341_WriteReg(uint8_t LCD_Reg)
{
  LCD_IO_WriteReg(LCD_Reg);
}

/**
  * @brief  Writes data to the selected LCD register.
  * @param  LCD_Reg: address of the selected register.
  * @retval None
  */
void ili9341_WriteData(uint16_t RegValue)
{
  LCD_IO_WriteData(RegValue);
  lcd_delay_us(1);
}

/**
  * @brief  Reads the selected LCD Register.
  * @retval LCD Register Value.
  */
uint16_t ili9341_ReadData(void)
{
  return LCD_IO_ReadData();
}

/**
  * @brief  Disables the Display.
  * @param  None
  * @retval LCD Register Value.
  */
uint16_t ili9341_ReadID(void)
{
  uint16_t data = 0;
  ili9341_WriteReg(LCD_READ_ID4);
  ili9341_ReadData(); //dummy read
  ili9341_ReadData(); //0x00
  data = ili9341_ReadData(); //0x93
  data <<= 8;
  data |= ili9341_ReadData(); //0x93

  return data;
}

/**
  * @brief  Get LCD PIXEL WIDTH.
  * @param  None
  * @retval LCD PIXEL WIDTH.
  */
uint16_t ili9341_GetLcdPixelWidth(void)
{
  /* Return LCD PIXEL WIDTH */
  return x_size;
}

/**
  * @brief  Get LCD PIXEL HEIGHT.
  * @param  None
  * @retval LCD PIXEL HEIGHT.
  */
uint16_t ili9341_GetLcdPixelHeight(void)
{
  /* Return LCD PIXEL HEIGHT */
  return y_size;
}

/**
  * @brief  Get Ser Rotation.
  * @param  Rotation ID
  * @retval none.
  */
void ili9341_SetLcdRotation(uint8_t rotation_id)
{
	ili9341_WriteReg(LCD_MAC);
	__rotation_id = rotation_id;
	switch(rotation_id)
	{
	case 0: ili9341_WriteData(0x48);
			x_size = 240;
			y_size = 320;
			break;

	case 1: ili9341_WriteData(0x28);
			x_size = 320;
			y_size = 240;
			break;

	case 2: ili9341_WriteData(0x88);
			x_size = 240;
			y_size = 320;
			break;

	case 3: ili9341_WriteData(0xE8);
			x_size = 320;
			y_size = 240;
			break;
	}
}

uint8_t ili9341_GetLcdRotation(void)
{
	return __rotation_id;
}

void ili9341_SwLcdReset(void)
{
	ili9341_WriteReg(LCD_SWRESET);
	for (int i = 0; i < 3; i++)
	{
		ili9341_WriteData(0xFF);
	}
}

void ili9341_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
	ili9341_WriteReg(LCD_COLUMN_ADDR);
	ili9341_WriteData(Xpos >> 8);
	ili9341_WriteData(Xpos & 0xFF);
	lcd_delay_us(1);
	ili9341_WriteReg(LCD_PAGE_ADDR);
	ili9341_WriteData(Ypos >> 8);
	ili9341_WriteData(Ypos & 0xFF);
	lcd_delay_us(1);
}

void ili9341_Lcd_clear(uint16_t Color)
{
	uint32_t index=0;
	uint32_t totalpoint=ili9341_GetLcdPixelWidth();
	totalpoint*=ili9341_GetLcdPixelHeight();
	ili9341_SetCursor(0x0000,0x0000);
	ili9341_WriteReg(LCD_GRAM);
	for(index=0; index<totalpoint; index++)
	{
		ili9341_WriteData(Color);
	}
}

void ili9341_draw_point(uint16_t Xpos, uint16_t Ypos)
{
	ili9341_SetCursor(Xpos, Ypos);
	ili9341_WriteReg(LCD_GRAM);
	ili9341_WriteData(POINT_COLOR);
}

void ili9341_Write_pixel(uint16_t Xpos, uint16_t Ypos, uint16_t Color)
{
	ili9341_SetCursor(Xpos, Ypos);
	ili9341_WriteReg(LCD_GRAM);
	ili9341_WriteData(Color);
}

uint16_t ili9341_Read_pixel(uint16_t Xpos, uint16_t Ypos)
{
	uint16_t r=0,g=0, b=0;
	if(Xpos >= ili9341_GetLcdPixelWidth() || Ypos >= ili9341_GetLcdPixelHeight()) return 0;
	ili9341_SetCursor(Xpos, Ypos);
	ili9341_WriteReg(LCD_RAMRD);
	ili9341_ReadData(); //dummy read
	r = ili9341_ReadData();
	lcd_delay_us(2);
	b = ili9341_ReadData();
	g = r&0xFF;
	g<<=8;
	return (((r>>11)<<11)|((g>>10)<<5)|(b>>11));
}



/**
  * @}
  */ 

/**
  * @}
  */ 
  
/**
  * @}
  */ 

/**
  * @}
  */
  
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
