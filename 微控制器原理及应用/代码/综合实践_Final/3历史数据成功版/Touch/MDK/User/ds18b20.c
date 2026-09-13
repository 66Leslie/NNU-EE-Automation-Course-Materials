#include "ds18b20.h"
#include "delay.h"	

void DS18B20_IO_out(void);
void DS18B20_IO_in(void);

void DS18B20_Rst(void)	   
{                 
		DS18B20_IO_out(); 	//SET PG11 OUTPUT
		GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET); 	//pull down DQ
		delay_us(750);    	//delay 750us
		GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET); 	//DQ=1 
		delay_us(15);     	//15US
}


u8 DS18B20_Check(void) 	   
{   
	u8 retry=0;
	DS18B20_IO_in();	//SET PG11 INPUT	 
    while (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8)&&retry<200)
	{
		retry++;
		delay_us(1);
	};	 
	if(retry>=200)return 1;
	else retry=0;
    while (!GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8)&&retry<240)
	{
		retry++;
		delay_us(1);
	};
	if(retry>=240)return 1;	    
	return 0;
}


u8 DS18B20_Read_Bit(void) 	 
{
    u8 data;
	DS18B20_IO_out();	//SET PG11 OUTPUT
    GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET); 
	delay_us(2);
    GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET); 
	DS18B20_IO_in();	//SET PG11 INPUT
	delay_us(12);
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8))data=1;
    else data=0;	 
    delay_us(50);           
    return data;
}


u8 DS18B20_Read_Byte(void)     
{        
    u8 i,j,dat;
    dat=0;
	for (i=1;i<=8;i++) 
	{
        j=DS18B20_Read_Bit();
        dat=(j<<7)|(dat>>1);
    }						    
    return dat;
}


void DS18B20_Write_Byte(u8 dat)     
 {             
    u8 j;
    u8 testb;
	DS18B20_IO_out();	//SET PG11 OUTPUT;
    for (j=1;j<=8;j++) 
	{
        testb=dat&0x01;
        dat=dat>>1;
        if (testb) 
        {
            GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);	// Write 1
            delay_us(2);                            
            GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
            delay_us(60);             
        }
        else 
        {
            GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);	// Write 0
            delay_us(60);             
            GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
            delay_us(2);                          
        }
    }
}

void DS18B20_Start(void) 
{   						               
    DS18B20_Rst();	   
	DS18B20_Check();	 
    DS18B20_Write_Byte(0xcc);	// skip rom
    DS18B20_Write_Byte(0x44);	// convert
} 


u8 DS18B20_Init(void)
{
 	GPIO_InitTypeDef  GPIO_InitStructure;
 	
 	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);	 
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8;//GPIO引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);									//初始化GPIO

 	GPIO_SetBits(GPIOA,GPIO_Pin_8);    

	DS18B20_Rst();

	return DS18B20_Check();
}  

short DS18B20_Get_Temp(void)
{
    u8 temp;
    u8 TL,TH;
	short tem;
    DS18B20_Start ();  			// ds1820 start convert
    DS18B20_Rst();
    DS18B20_Check();	 
    DS18B20_Write_Byte(0xcc);	// skip rom
    DS18B20_Write_Byte(0xbe);	// convert	    
    TL=DS18B20_Read_Byte(); 	// LSB   
    TH=DS18B20_Read_Byte(); 	// MSB  
	    	  
    if(TH>7)
    {
        TH=~TH;
        TL=~TL; 
        temp=0;					 
    }else temp=1;		  	  
    tem=TH; 				
    tem<<=8;    
    tem+=TL;					
    tem=(float)tem*0.625;		 
	if(temp)return tem; 		
	else return -tem;    
}

void DS18B20_IO_out(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE); 	//使能GPIO时钟					 

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8;//GPIO引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void DS18B20_IO_in(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE); 	//使能GPIO时钟					 
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8;//GPIO引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//GPIO速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 					//GPIO模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//GPIO推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//GPIO上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}
