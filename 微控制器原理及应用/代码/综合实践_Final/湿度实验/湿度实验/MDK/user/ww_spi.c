#include "ww_spi.h"

//SPI模块寄存器初始化
void SPI_Configuration(void)
{
	SPI_InitTypeDef SPI_InitStruct;
	GPIO_InitTypeDef GPIO_InitStructure;
	//SPI模块使用的时钟使能  1端口时钟  2SPI时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1,ENABLE);	

	//SPI模块使用的端口初始化 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出功能
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
	
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_SPI1); //PA5复用为 SPI1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_SPI1); //PA6复用为 SPI1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_SPI1); //PA7复用为 SPI1
 
	//这里只针对SPI口初始化
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,ENABLE);//复位SPI1
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,DISABLE);//停止复位SPI1
	
  SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;//定义波特率预分频的值:波特率预分频值为4
	SPI_InitStruct.SPI_Direction= SPI_Direction_2Lines_FullDuplex;//设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;//设置主模式
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;//设置SPI的数据大小:SPI发送接收8位帧结构
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;//串行同步时钟的空闲状态为低电平
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;//串行同步时钟的第1个跳变沿（上升或下降）数据被采样
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;//NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
	SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;//指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
	SPI_InitStruct.SPI_CRCPolynomial = 7;//CRC值计算的多项式
	SPI_Init(SPI1, &SPI_InitStruct);
	
	

	SPI_Cmd(SPI1, ENABLE);
}


//SPI1读写一字节数据
unsigned char SPI1_ReadWrite(unsigned char writedat)
{
   //检查指定的SPI标志位设置与否:发送缓存空标志位
   while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);

   //通过外设SPIx发送一个数据
   SPI_SendData(SPI1, writedat);

   //检查指定的SPI标志位设置与否:接受缓存非空标志位
   while (SPI_GetFlagStatus(SPI1, SPI_FLAG_RXNE) == RESET);

   //返回通过SPIx最近接收的数据	
   return SPI_ReceiveData(SPI1);
}



