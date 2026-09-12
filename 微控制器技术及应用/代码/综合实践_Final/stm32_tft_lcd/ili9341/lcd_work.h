/*
 * lcd_work.h
 *
 *  Created on: Aug 20, 2022
 *      Author: Konstantin
 */

#ifndef LCD_WORK_H_
#define LCD_WORK_H_

#include "ili9341.h"

/**
  * @brief  LCD status structure definition
  */
#define LCD_OK         0x00
#define LCD_ERROR      0x01
#define LCD_TIMEOUT    0x02

#define WHITE         	 0xFFFF
#define BLACK         	 0x0000
#define BLUE         	 0x001F
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40
#define BRRED 			 0XFC07
#define GRAY  			 0X8430
#define DARKBLUE      	 0X01CF
#define LIGHTBLUE      	 0X7D7C
#define GRAYBLUE       	 0X5458
#define LIGHTGREEN     	 0X841F
#define LGRAY 			 0XC618
#define LGRAYBLUE        0XA651
#define LBBLUE           0X2B12

uint8_t LCD_Init(void);
uint32_t LCD_GetXSize(void);
uint32_t LCD_GetYSize(void);
void LCD_SW_reset(void);
uint16_t LCD_GetID(void);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_Clear(uint16_t Color);
void LCD_SetCursor (uint16_t Xpos, uint16_t Ypos);
void LCD_DrawPoint(uint16_t Xpos, uint16_t Ypos);
void LCD_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t Color);
uint16_t LCD_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void LCD_Draw_Circle(uint16_t Xpos, uint16_t Ypos, uint8_t r, uint16_t Color);
void LCD_DrawLine(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color);
void LCD_DrawRectangle(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color);
void LCD_Fill(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color);
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint8_t mode, uint16_t point_color, uint16_t back_color);
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t point_color, uint16_t back_color);
void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t *string, uint16_t point_color, uint16_t back_color);


#endif /* LCD_WORK_H_ */
