/*
 * lcd_work.c
 *
 *  Created on: Aug 20, 2022
 *      Author: Konstantin
 */

#include "lcd_work.h"
#include "font.h"

static LCD_DrvTypeDef  *lcd_drv;

static void LCD_WriteRAM_Prepare(void);
static void LCD_WriteRAM(uint16_t data);
static uint32_t LCD_Pow(uint8_t m,uint8_t n);

uint8_t  LCD_Init(void)
{
	uint8_t ret = LCD_ERROR;

	lcd_drv = &ili9341_drv;

	/* LCD Init */
	lcd_drv->Init();

    /* Clear the LCD screen */
    LCD_Clear(WHITE);

    /* Initialize the font */
//    BSP_LCD_SetFont(&LCD_DEFAULT_FONT);

	return ret = LCD_OK;
}

uint32_t LCD_GetXSize(void)
{
	return (lcd_drv->GetLcdPixelWidth());
}
uint32_t LCD_GetYSize(void)
{
	return (lcd_drv->GetLcdPixelHeight());
}

void LCD_set_rotation(uint8_t rotation_id)
{
	lcd_drv->SetLcdRotation(rotation_id);
}

void LCD_SW_reset(void)
{
	lcd_drv->SwLcdReset();
}

uint16_t LCD_GetID(void)
{
	return lcd_drv->ReadID();
}

void LCD_DisplayOn(void)
{
	lcd_drv->DisplayOn();
}

void LCD_DisplayOff(void)
{
	lcd_drv->DisplayOff();
}

void LCD_SetCursor (uint16_t Xpos, uint16_t Ypos)
{
	lcd_drv->SetCursor(Xpos, Ypos);
}

void LCD_Clear(uint16_t Color)
{
	lcd_drv->LcdClear(Color);
}

void LCD_DrawPoint(uint16_t Xpos, uint16_t Ypos)
{

}

void LCD_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t Color)
{
	lcd_drv->WritePixel(Xpos, Ypos, Color);
}

uint16_t LCD_ReadPixel(uint16_t Xpos, uint16_t Ypos)
{
	return lcd_drv->ReadPixel(Xpos, Ypos);
}

void LCD_Draw_Circle(uint16_t Xpos,uint16_t Ypos, uint8_t r, uint16_t Color)
{
	int a, b;
	int di;
	a=0; b=r;
	di=3-(r<<1);

	while(a<=b)
	{
		LCD_WritePixel(Xpos+a,Ypos-b, Color);             //5
 		LCD_WritePixel(Xpos+b,Ypos-a, Color);             //0
		LCD_WritePixel(Xpos+b,Ypos+a, Color);             //4
		LCD_WritePixel(Xpos+a,Ypos+b, Color);             //6
		LCD_WritePixel(Xpos-a,Ypos+b, Color);             //1
 		LCD_WritePixel(Xpos-b,Ypos+a, Color);
		LCD_WritePixel(Xpos-a,Ypos-b, Color);             //2
  		LCD_WritePixel(Xpos-b,Ypos-a, Color);             //7
		a++;
		//Bresenham
		if(di < 0) di += 4*a+6;
		else
		{
			di += 10+4*(a-b);
			b--;
		}
	}
}

void LCD_DrawLine(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color)
{
	uint16_t t;
	int xerr=0, yerr=0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = X2-X1;
	delta_y = Y2-Y1;
	uRow = X1;
	uCol = Y1;
	if(delta_x > 0) incx=1;
	else if(delta_x == 0) incx=0;
	else {incx=-1; delta_x = -delta_x;}
	if(delta_y > 0) incy = 1;
	else if(delta_y == 0)incy = 0;
	else{incy = -1; delta_y = -delta_y;}
	if( delta_x > delta_y) distance = delta_x;
	else distance = delta_y;
	for(t=0; t<=distance+1; t++)
	{
		LCD_WritePixel(uRow,uCol, Color);
		xerr += delta_x ;
		yerr += delta_y ;
		if(xerr > distance)
		{
			xerr -= distance;
			uRow += incx;
		}
		if(yerr > distance)
		{
			yerr -= distance;
			uCol += incy;
		}
	}
}

void LCD_DrawRectangle(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color)
{
	LCD_DrawLine(X1,Y1,X2,Y1,Color);
	LCD_DrawLine(X1,Y1,X1,Y2,Color);
	LCD_DrawLine(X1,Y2,X2,Y2,Color);
	LCD_DrawLine(X2,Y1,X2,Y2,Color);
}

void LCD_Fill(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t Color)
{
	uint16_t i,j;
	uint16_t xlen=0;

	xlen = X2 - X1 + 1;
	for(i = Y1; i <= Y2; i++)
	{
		LCD_SetCursor(X1, i);
		LCD_WriteRAM_Prepare();
		for(j=0; j<xlen; j++) LCD_WriteRAM(Color);
	}
}

static void LCD_WriteRAM_Prepare(void)
{
	lcd_drv->LcdWriteReg(LCD_GRAM);
}

static void LCD_WriteRAM(uint16_t data)
{
	lcd_drv->LcdWriteData(data);
}

void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint8_t mode, uint16_t point_color, uint16_t back_color)
{
    uint8_t temp, t1, t;
    uint16_t y0 = y;
    uint8_t csize=(size/8+((size%8)?1:0))*(size/2);

	num=num-' ';
	for(t=0; t<csize; t++)
	{
		if(size == 12) temp = asc2_1206[num][t];
		else if(size == 16) temp = asc2_1608[num][t];
		else if(size == 24) temp = asc2_2412[num][t];
		else return;
		for(t1 = 0; t1 < 8; t1++)
		{
			if(temp&0x80) LCD_WritePixel(x, y, point_color);
			else if(mode == 0) LCD_WritePixel(x, y, back_color);
			temp <<= 1;
			y++;
			if(y >= LCD_GetYSize()) return;
			if((y - y0) == size)
			{
				y = y0;
				x++;
				if(x >= LCD_GetXSize()) return;
				break;
			}
		}
	}
}

static uint32_t LCD_Pow(uint8_t m,uint8_t n)
{
	uint32_t result=1;
	while(n--)result*=m;
	return result;
}

void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t point_color, uint16_t back_color)
{
	uint8_t t, temp;
	uint8_t enshow=0;
	for(t = 0; t < len; t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_ShowChar(x+(size/2)*t,y,' ',size,0, point_color, back_color);
				continue;
			}else enshow=1;

		}
	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,0, point_color, back_color);
	}
}

void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t *string, uint16_t point_color, uint16_t back_color)
{
	uint8_t x0=x;
	width+=x;
	height+=y;
    while((*string<='~')&&(*string>=' '))
    {
        if(x>=width){x=x0;y+=size;}
        if(y>=height)break;
        LCD_ShowChar(x, y, *string, size, 0, point_color, back_color);
        x+=size/2;
        string++;
    }
}
