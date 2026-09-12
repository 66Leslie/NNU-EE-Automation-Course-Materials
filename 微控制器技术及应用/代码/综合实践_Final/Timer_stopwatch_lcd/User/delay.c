#include "delay.h"
#include "misc.h"

static u8  fac_us=0;							//us延时倍乘数			   
static u32 fac_ms=0;							//ms延时倍乘数
	
   
//初始化延迟函数
//时钟固定为AHB时钟的1/8
void delay_init(void)
{
 	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8); 
	fac_us=168/8;						
	fac_ms=(u16)fac_us*1000;									 
}								    

//延时us
//us为要延时的us数.
void delay_us(u32 us)
{
	u32 temp;		   
	SysTick->LOAD=(u32)us*fac_us;				//时间加载(SysTick->LOAD为24bit)
	SysTick->VAL =0x00;							//清空计数器
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;	//开始倒数  
	temp=SysTick->VAL;
	do
	{
		temp=SysTick->CTRL;

	}while((temp&0x01)&&!(temp&(1<<16)));		//等待时间到达   
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;	//关闭计数器
	SysTick->VAL =0X00;       					//清空计数器	  
}
////延时ms
////注意ms的范围
////SysTick->LOAD为24位寄存器,所以,最大延时为:
////ms<=0xffffff*8*1000/SYSCLK
////SYSCLK单位为Hz,ms单位为ms
////对168M条件下,ms<=798ms 
//void delay_xms(u16 ms)
//{	 		  	  
//	u32 temp;		   
//	SysTick->LOAD=(u32)ms*fac_ms;			//时间加载(SysTick->LOAD为24bit)
//	SysTick->VAL =0x00;           			//清空计数器
//	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //开始倒数 
//	do
//	{
//		temp=SysTick->CTRL;
//	}while((temp&0x01)&&!(temp&(1<<16)));	//等待时间到达   
//	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //关闭计数器
//	SysTick->VAL =0X00;     		  		//清空计数器	  	    
//} 
////延时ms 
////ms:0~65535
//void delay_ms(u16 ms)
//{	 	 
//	u8 repeat=ms/540;						//这里用540,是考虑到某些客户可能超频使用,
//											//比如超频到248M的时候,delay_xms最大只能延时541ms左右了
//	u16 remain=ms%540;
//	while(repeat)
//	{
//		delay_xms(540);
//		repeat--;
//	}
//	if(remain)delay_xms(remain);
//} 

			 
//延时ms
//注意ms的范围
//对168M条件下,SysTick->LOAD为24位寄存器,所以,最大延时为:ms<=0xffffff*1000/SYSCLK
//200ms为一个周期，大于200MS引入变量扩大定时时间。最大定时时间 200ms*65535
void delay_ms(u16 ms){
	u32 temp;	
	u8 t;
	u16 temp1;
	temp1=ms/200;
	for(t=0;t<temp1;t++)
	{
		SysTick->LOAD=(u32)200*fac_ms;				//时间加载(SysTick->LOAD为24bit)
		SysTick->VAL =0x00;							//清空计数器
		SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;	//开始倒数  
		temp=SysTick->VAL;
		do
		{
			temp=SysTick->CTRL;

		}while((temp&0x01)&&!(temp&(1<<16)));		//等待时间到达   
		SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;	//关闭计数器
		SysTick->VAL =0X00;       					//清空计数器	  
	}
	if((ms%200)!=0)
	{
		SysTick->LOAD=(u32)(ms%200)*fac_ms;				//时间加载(SysTick->LOAD为24bit)
		SysTick->VAL =0x00;							//清空计数器
		SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;	//开始倒数  
		temp=SysTick->VAL;
		do
		{
			temp=SysTick->CTRL;

		}while((temp&0x01)&&!(temp&(1<<16)));		//等待时间到达   
		SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;	//关闭计数器
		SysTick->VAL =0X00;       					//清空计数器	
	}		
}


































