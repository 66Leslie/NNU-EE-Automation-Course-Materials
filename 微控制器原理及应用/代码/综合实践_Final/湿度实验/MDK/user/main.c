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

#define Key1 PCin(0) //��������
#define Key2 PCin(1)
#define Key3 PCin(2) //��������
#define Key4 PCin(3)
#define Key5 PCin(5) //ȷ�ϰ�ť

#define LED1 PCout(4) //��

#define EN1 PEout(0)
#define IN1 PEout(1)
#define IN2 PEout(2)

extern volatile float RH;
extern volatile u16 HS1101_freq_hz;

int Alarm_h = 50;		 //������ֵ
int Alarm_l = 40;    //������ֵ

int main(void)
{ 
	char lcd_dat[16];//Һ����ʾ����
	int j=1;
	delay_init();//��ʱ����ʱ�ӳ�ʼ��
	GPIO_Configuration_LCD();//gpio��ʼ��
	SPI_Configuration();//spi��ʼ��
	LCD_init();//lcd��ʼ��    
	Tim2_Init();

	
	GPIO_Configuration();	//GPIO��ʼ��
	EN1=0;IN1=0;IN2=0;//�ص��? 
	while(1)
	{
		//Һ����ʾ
		sprintf(lcd_dat,"21230933 H:%lf%%    ",RH);//��ʾʪ��
		LCD_prints(0,0,lcd_dat);
//		sprintf(lcd_dat,"%d",j);
//		sprintf(lcd_dat,"T:%lf ",temp_data1);//��ʾ�¶�
		sprintf(lcd_dat,"F:%uHz",HS1101_freq_hz);//��ʾ��ֵ
		LCD_prints(0,1,lcd_dat);
		//��������
		if(Key1==0)//����1����
		{
			delay_ms(10); //��������
			if(Key1==0)
			{
					if(Alarm_h<100)
						Alarm_h+=1;
					while(Key1==0);
			}
		}
		if(Key2==0)//����2����
		{
			delay_ms(10);
			if(Key2==0)
			{
					if(Alarm_h>10)
						Alarm_h-=1;
					while(Key2==0);
			}
		}
			if(Key3==0)//����3����
		{
			delay_ms(10);
			if(Key3==0)
			{
					if(Alarm_l<100)
						Alarm_l+=1;
					while(Key3==0);
			}
		}
		if(Key4==0) //����4����
		{
			delay_ms(10);
			if(Key4==0)
			{
					if(Alarm_l>10)
						Alarm_l-=1;
					while(Key4==0);
			}
		}	
		if(Key5==0) //����5����
		{
			delay_ms(10);
			if(Key5==0)
			{
					j+=1;
					while(Key5==0);
			}
		}	
		//���״�?����
		if(RH>Alarm_h || RH<Alarm_l) //ʪ�ȹ��߻����?
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
		
		else//�رյ��?
		{
			EN1=0;IN1=0;IN2=0;
			LED1 = 1;
		}
		
		delay_ms(10);
		
	}	
	
}

