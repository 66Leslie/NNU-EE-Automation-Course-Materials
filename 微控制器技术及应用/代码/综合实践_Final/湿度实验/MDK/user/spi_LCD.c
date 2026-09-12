#include "stm32f4xx.h"
#include "delay.h"
#include "spi_LCD.h"


void GPIO_Configuration_LCD(void)
{
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出功能
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}


void LCD_init(void) //初始化,通常情况下此函数不用更改
{
		LCD_RS_0;
		LCD_RW_0; 
		LCD_EN_0; 

		delay_ms(50);
		LCD_cmd(0x38);//16*2显示，5*7点阵，8位数据
		delay_ms(10);
		LCD_cmd(0x38);//16*2显示，5*7点阵，8位数据
		delay_ms(10);
		LCD_cmd(0x38);//16*2显示，5*7点阵，8位数据
		delay_ms(10);
		LCD_cmd(0x08);//先关显示，后开显示
		delay_ms(10);
		LCD_cmd(0x01);//清屏
		delay_ms(10);
		LCD_cmd(0x06);//自动右移光标，0X04为左移光标
		delay_ms(10);   
		LCD_cmd(0x0c);//显示开，关光标
		delay_ms(10);
}

void LCD_clr(void)  //清屏
{
    LCD_cmd(0x01);
}

void LCD_cmd(u8 cmd)//写命令，注意！！！
{   
		delay_ms(10);
		LCD_RS_0;        //GPIOC->BRR = LCD_RS;
		delay_us(1);   
		LCD_RW_0;       // GPIOC->BRR = LCD_RW;
		delay_us(1);   
		LCD_EN_0;        //GPIOC->BRR = LCD_EN;
		delay_us(300);
		LCD_LDL();
		SPI1_ReadWrite(cmd);
		delay_ms(1);  
		LCD_LDH();
		LCD_EN_1;
		delay_us(300);    
		LCD_EN_0;     
}

void LCD_dat(u8 dat)//写数据
{
		delay_ms(10);   
		LCD_RS_1;
		delay_us(10);  		// GPIOC->BSRR = LCD_RS;    
		LCD_RW_0;       	//GPIOC->BRR = LCD_RW;
		delay_us(10);
		LCD_EN_0;        //GPIOC->BRR = LCD_EN;
		delay_us(300);
		LCD_LDL();
		SPI1_ReadWrite(dat);
		delay_ms(1); 
		LCD_LDH();
		LCD_EN_1;       	// GPIOC->BSRR = LCD_EN;
		delay_us(300);   
		LCD_EN_0;       	//GPIOC->BRR = LCD_EN;
}

void LCD_pos(u8 x,u8 y)//显示位置,不需要更改
{
     if(y)
     LCD_cmd(x | 0xc0);
   	 else
     LCD_cmd(x | 0x80);
}

void LCD_printc(u8 x,u8 y,char c)//显示字符，不需要更改
{
    LCD_pos(x,y);
    LCD_dat(c);
}

void LCD_prints(u8 x,u8 y,char *s)//显示字符串，不需要更改
{
    LCD_pos(x,y);
    while(*s!='\0')
    {
        LCD_dat(*s);
        s++;
        delay_ms(1);
    }
}
