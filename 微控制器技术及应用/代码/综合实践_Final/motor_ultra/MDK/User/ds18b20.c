#include "ds18b20.h"
#include "stm32f4xx.h"

//配置DS18B20用到的I/O口
static void DS18B20_GPIO_Config(void)
{		
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(DS18B20_CLK, ENABLE); 
															   
  GPIO_InitStructure.GPIO_Pin = DS18B20_PIN;	
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;   
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	 //推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; 
  GPIO_Init(DS18B20_PORT, &GPIO_InitStructure);
	
	GPIO_SetBits(DS18B20_PORT, DS18B20_PIN);	 //DS18B20数据引脚初始化配置为高电平输出
}

//配置使DS18B20-DATA引脚变为输入模式
static void DS18B20_Mode_IPU(void)
{
 	  GPIO_InitTypeDef GPIO_InitStructure;
	  GPIO_InitStructure.GPIO_Pin = DS18B20_PIN; 
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	//上拉输入
	  GPIO_Init(DS18B20_PORT, &GPIO_InitStructure);
}

//配置使DS18B20-DATA引脚变为输出模式
static void DS18B20_Mode_Out_PP(void)
{
 	GPIO_InitTypeDef GPIO_InitStructure;
															   
  GPIO_InitStructure.GPIO_Pin = DS18B20_PIN;	
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	//推挽输出   
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_Init(DS18B20_PORT, &GPIO_InitStructure);
}

 //主机给从机发送复位脉冲
static void DS18B20_Rst(void)
{
	DS18B20_Mode_Out_PP();     //主机设置为推挽输出 
	
	GPIO_ResetBits(DS18B20_PORT,DS18B20_PIN);     //主机至少产生480us的低电平复位信号
	delay_us(750);
	GPIO_SetBits(DS18B20_PORT,DS18B20_PIN);   //主机在产生复位信号后，需将总线拉高
	delay_us(15);   //从机接收到主机的复位信号后，会在15~60us后给主机发一个存在脉冲
}

 //检测从机给主机返回的存在脉冲，成功返回0，失败返回1
static u8 DS18B20_Presence(void)
{
	u8 pulse_time = 0;
	
	DS18B20_Mode_IPU();    //主机设置为上拉输入
	
	/* 等待存在脉冲的到来，存在脉冲为一个60~240us的低电平信号 
	 * 如果存在脉冲没有来则做超时处理，从机接收到主机的复位信号后，会在15~60us后给主机发一个存在脉冲
	 */
	while( GPIO_ReadInputDataBit(DS18B20_PORT,DS18B20_PIN) && pulse_time<100 )
	{
		pulse_time++;
		delay_us(1);
	}	

	if( pulse_time >=100 )
		return 1;
	else
		pulse_time = 0;
	
	while( !GPIO_ReadInputDataBit(DS18B20_PORT,DS18B20_PIN) && pulse_time<240 )
	{
		pulse_time++;
		delay_us(1);
	}	
	if( pulse_time >=240 )
		return 1;
	else
		return 0;
}

//从DS18B20读取一个bit
static u8 DS18B20_Read_Bit(void)
{
	u8 dat;
	
	DS18B20_Mode_Out_PP();
	GPIO_ResetBits(DS18B20_PORT,DS18B20_PIN);
	delay_us(10);
	
	DS18B20_Mode_IPU();
	
	if( GPIO_ReadInputDataBit(DS18B20_PORT,DS18B20_PIN) == SET )
		dat = 1;
	else
		dat = 0;
	delay_us(45);
	
	return dat;
}

//从DS18B20读一个字节，低位先行
u8 DS18B20_Read_Byte(void)
{
	u8 i, j, dat = 0;	
	
	for(i=0; i<8; i++) 
	{
		j = DS18B20_Read_Bit();		//从DS18B20读取一个bit
		dat = (dat) | (j<<i);
	}
	
	return dat;																																																																																
}

//写一个字节到DS18B20，低位先行
void DS18B20_Write_Byte(u8 dat)
{
	u8 i, b;
	DS18B20_Mode_Out_PP();
	
	for( i=0; i<8; i++ )
	{
		b = dat&0x01;
		dat = dat>>1;		
		/* 写0和写1的时间至少要大于60us */
		if (b)
		{			
			GPIO_ResetBits(DS18B20_PORT,DS18B20_PIN);
			delay_us(8);   //1us < 这个延时 < 15us
			
			GPIO_SetBits(DS18B20_PORT,DS18B20_PIN);
			delay_us(58);    //58us+8us>60us
		}		
		else
		{			
			GPIO_ResetBits(DS18B20_PORT,DS18B20_PIN);  
			/* 60us < Tx 0 < 120us */
			delay_us(70);
			
			GPIO_SetBits(DS18B20_PORT,DS18B20_PIN);			
			/* 1us < Trec(恢复时间) < 无穷大*/
			delay_us(2);
		}
	}
}

 //起始DS18B20
void DS18B20_Start(void)
{
	DS18B20_Rst();	           //主机给从机发送复位脉冲
	DS18B20_Presence();	       //检测从机给主机返回的存在脉冲
	DS18B20_Write_Byte(0XCC);		 // 跳过 ROM 
	DS18B20_Write_Byte(0X44);		 // 开始转换 
}

//DS18B20初始化函数
u8 DS18B20_Init(void)
{
	DS18B20_GPIO_Config();   
	DS18B20_Rst();
	
	return DS18B20_Presence();
}

 //从DS18B20读取温度值
float DS18B20_Get_Temp(void)
{
	u8 msb, lsb;
	short s_tem;
	float f_tem;
	
	DS18B20_Rst();	   
	DS18B20_Presence();	 
	DS18B20_Write_Byte(0XCC);				/* 跳过 ROM */
	DS18B20_Write_Byte(0X44);				/* 开始转换 */
	
	DS18B20_Rst();
  DS18B20_Presence();
	DS18B20_Write_Byte(0XCC);				/* 跳过 ROM */
  DS18B20_Write_Byte(0XBE);				/* 读温度值 */
	
	lsb = DS18B20_Read_Byte();		 
	msb = DS18B20_Read_Byte(); 
	
	s_tem = msb<<8;
	s_tem = s_tem | lsb;
	
	if( s_tem < 0 )		/* 负温度 */
		f_tem = (~s_tem+1) * 0.0625;	
	else
		f_tem = (s_tem * 0.0625);
	  
																	//这样做的目的将小数点后第一位也转换为可显示数字
																	//同时进行一个四舍五入操作。
	  
	return f_tem; 	
}



