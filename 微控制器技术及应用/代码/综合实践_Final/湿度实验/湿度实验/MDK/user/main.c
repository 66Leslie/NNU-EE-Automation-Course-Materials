#include "stm32f4xx.h"
#include "delay.h"
#include "ww_spi.h"
#include "SYS.h"
#include <stdio.h>
#include <string.h>
#include "spi_LCD.h"
#include "timer.h"
#include "gpio.h"
#include "DS18B20.h"

#define Key1 PCin(0) //设置下限
#define Key2 PCin(1)
#define Key3 PCin(2) //设置上限
#define Key4 PCin(3)
#define Key5 PCin(5) //确认按钮

#define LED1 PCout(4) //灯

#define EN1 PEout(0)
#define IN1 PEout(1)
#define IN2 PEout(2)

extern volatile double RH;
extern volatile double Freq;

int Alarm_h = 50;		 //上限阈值
int Alarm_l = 40;    //下限阈值

int main(void)
{ 
	char lcd_dat[16];//液晶显示变量
	int j=1;
	delay_init();//延时函数时钟初始化
	GPIO_Configuration_LCD();//gpio初始化
	SPI_Configuration();//spi初始化
	LCD_init();//lcd初始化    
	Tim2_Init();

double	temp_data=DS18B20_Get_Temp();
double	temp_data1=temp_data;
	
	GPIO_Configuration();	//GPIO初始化
	EN1=0;IN1=0;IN2=0;//关电机 
	while(1)
	{
		sprintf(lcd_dat,"H:%.1lf%%",RH);//显示频率和湿度
		LCD_prints(0,0,lcd_dat);
		sprintf(lcd_dat,"h:%d%% l:%d%%    ",Alarm_h,Alarm_l);//显示阈值
		LCD_prints(0,1,lcd_dat);
		//按键处理
		if(Key1==0)//按键1功能
		{
			delay_ms(10); //按键防抖
			if(Key1==0)
			{
					if(Alarm_h<100)
						Alarm_h+=1;
					while(Key1==0);
			}
		}
		if(Key2==0)//按键2功能
		{
			delay_ms(10);
			if(Key2==0)
			{
					if(Alarm_h>10)
						Alarm_h-=1;
					while(Key2==0);
			}
		}
			if(Key3==0)//按键3功能
		{
			delay_ms(10);
			if(Key3==0)
			{
					if(Alarm_l<100)
						Alarm_l+=1;
					while(Key3==0);
			}
		}
		if(Key4==0) //按键4功能
		{
			delay_ms(10);
			if(Key4==0)
			{
					if(Alarm_l>10)
						Alarm_l-=1;
					while(Key4==0);
			}
		}	
		if(Key5==0) //按键5功能
		{
			delay_ms(10);
			if(Key5==0)
			{
					j+=1;
					while(Key5==0);
			}
		}	
		//电机状态处理
		if(RH>Alarm_h || RH<Alarm_l) //湿度过高或过低
		{
			EN1=1;
			IN1=1;
			IN2=0;
			if(j%2==1)
			{
			 LED1 = ~LED1;
			 delay_ms(500);	
			}
      else
			{
			 LED1 = 0;	
			}
/*			if(Key5==0)
			{
			LED1 = 0;	
			j=~j;
			}
			else
			{
			do{
				LED1 = ~LED1;
			  delay_ms(500);	
			}while(j!=1);
		  }
*/      
		}
		
		else//关闭电机
		{
			EN1=0;IN1=0;IN2=0;
			LED1 = 1;
		}
		
		delay_ms(10);
		
	}	
	
}

