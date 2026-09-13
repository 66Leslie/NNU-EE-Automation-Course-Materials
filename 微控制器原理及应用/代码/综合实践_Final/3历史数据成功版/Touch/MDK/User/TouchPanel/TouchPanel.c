/* Includes ------------------------------------------------------------------*/
#include "TouchPanel.h"
#include "LCD.h"

/* 私有变量 ---------------------------------------------------------*/
Matrix matrix ;
Coordinate  display ;

/* DisplaySample LCD坐标上对应的ads7843采样AD值 如：LCD 坐标45,45 应该的X Y采样ADC分别为3388,920 */	
Coordinate ScreenSample[3];
/* LCD上的坐标 */
Coordinate DisplaySample[3] =   {
                                            { 30,  45},
											{ 290, 45},
                                            { 160,210}
	                            } ;

/* 私有 define ------------------------------------------------------------*/
#define THRESHOLD 2   /* 差值门限 */


/*******************************************************************************
* 函数名称：ADS7843_SPI_Init
* 说明：ADS7843 SPI 初始化
* 输入：无
* 输出：无
* 返回：无
* 注意：无
*******************************************************************************/
static void ADS7843_SPI_Init(void) 
{ 
  SPI_InitTypeDef  SPI_InitStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	
	SPI_I2S_DeInit(SPI2);
 
  /* SPI2 Config -------------------------------------------------------------*/ 
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; 
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master; 
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; 
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; 
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; 
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; 
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; 
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; 
  SPI_InitStructure.SPI_CRCPolynomial = 7; 
  SPI_Init(SPI2, &SPI_InitStructure); 
  /* Enable SPI2 */ 
  SPI_Cmd(SPI2, ENABLE); 
} 

/*******************************************************************************
* 函数名：TP_Init
* 说明：ADS7843端口初始化
* 输入：无
* 输出：无
* 返回：无
* 注意：无
*******************************************************************************/
void TP_Init(void) 
{ 
  	GPIO_InitTypeDef GPIO_InitStructure;
  	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC ,ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
  	/* Configure SPI2 pins: SCK, MISO and MOSI ---------------------------------*/ 
		GPIO_PinAFConfig(GPIOB,  GPIO_PinSource13,  GPIO_AF_SPI2);
		GPIO_PinAFConfig(GPIOB,  GPIO_PinSource14,  GPIO_AF_SPI2);
		GPIO_PinAFConfig(GPIOB,  GPIO_PinSource15,  GPIO_AF_SPI2);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_13 | GPIO_Pin_14;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;  
		GPIO_Init(GPIOB, &GPIO_InitStructure);
			/* TP_CS */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;  
		GPIO_Init(GPIOC,&GPIO_InitStructure);
  	/* TP_IRQ */
  	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;  
  	GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  	TP_CS(1); 
  	ADS7843_SPI_Init(); 
} 


/*******************************************************************************
* 函数名称：DelayUS
* 说明：延时1us
* 输入 : - cnt: 值
* 输出：无
* 返回：无
* 注意：无
*******************************************************************************/
static void DelayUS(vu32 cnt)
{
  uint16_t i;
  for(i = 0;i<cnt;i++)
  {
     uint8_t us = 12; /* 设置值为12，大约延1微秒 */    
     while (us--)     /* 延1微秒	*/
     {
       ;   
     }
  }
}


/*******************************************************************************
* 函数名：WR_CMD
* 描述 : 向 ADS7843 写数据
* 输入: - cmd: 传输的数据
* 输出：无
* 返回：无
* 注意：无
*******************************************************************************/
static void WR_CMD (uint8_t cmd)  
{ 
  /* Wait for SPI2 Tx buffer empty */ 
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET); 
  /* Send SPI2 data */ 
  SPI_I2S_SendData(SPI2,cmd); 
  /* Wait for SPI2 data reception */ 
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET); 
  /* Read SPI2 received data */ 
  SPI_I2S_ReceiveData(SPI2); 
} 



/*******************************************************************************
* 函数名：RD_AD
* 说明：读取ADC值
* 输入：无
* 输出：无
* 返回：ADS7843返回二字节数据
* 注意：无
*******************************************************************************/
static int RD_AD(void)  
{ 
  unsigned short buf,temp; 
  /* Wait for SPI2 Tx buffer empty */ 
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET); 
  /* Send SPI2 data */ 
  SPI_I2S_SendData(SPI2,0x0000); 
  /* Wait for SPI2 data reception */ 
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET); 
  /* Read SPI2 received data */ 
  temp=SPI_I2S_ReceiveData(SPI2); 
  buf=temp<<8; 
  delay_us(1); 
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET); 
  /* Send SPI2 data */ 
  SPI_I2S_SendData(SPI2,0x0000); 
  /* Wait for SPI2 data reception */ 
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET); 
  /* Read SPI2 received data */ 
  temp=SPI_I2S_ReceiveData(SPI2); 
  buf |= temp; 
  buf>>=3; 
  buf&=0xfff; 
  return buf; 
} 


/*******************************************************************************
* 函数名称：Read_X
* 说明：读取ADS7843通道X+的ADC值
* 输入：无
* 输出：无
* 返回：ADS7843返回通道X+的ADC值
* 注意：无 
*******************************************************************************/
int Read_X(void)  
{  
  int i; 
  TP_CS(0); 
  delay_us(1); 
  WR_CMD(CHX); 
  delay_us(1); 
  i=RD_AD(); 
  TP_CS(1); 
  return i;    
} 

/*******************************************************************************
* 函数名称：Read_Y
* 说明：读取ADS7843通道Y+的ADC值
* 输入：无
* 输出：无
* 返回：ADS7843返回通道Y+的ADC值
* 注意：无
*******************************************************************************/
int Read_Y(void)  
{  
  int i; 
  TP_CS(0); 
  delay_us(1); 
  WR_CMD(CHY); 
  delay_us(1); 
  i=RD_AD(); 
  TP_CS(1); 
  return i;     
} 


/*******************************************************************************
* 函数名称：TP_GetAdXY
* 说明：读取ADS7843通道X+通道Y+的ADC值
* 输入：无
* 输出：无
* 返回：ADS7843返回通道X+通道Y+的ADC值
* 注意：无
*******************************************************************************/
void TP_GetAdXY(int *x,int *y)  
{ 
  int adx,ady; 
  adx=Read_X(); 
  delay_us(1); 
  ady=Read_Y(); 
  *x=adx; 
  *y=ady; 
} 

/*******************************************************************************
* 函数名：TP_DrawPoint
* 说明：在指定座标画点
* 输入：- Xpos：行坐标
* - Ypos：线坐标
* 输出：无
* 返回：无
* 注意：无
*******************************************************************************/
void TP_DrawPoint(uint16_t Xpos,uint16_t Ypos)
{
  LCD_SetPoint(Xpos,Ypos,0xf800);     /* 中心点 */
  LCD_SetPoint(Xpos+1,Ypos,0xf800);
  LCD_SetPoint(Xpos,Ypos+1,0xf800);
  LCD_SetPoint(Xpos+1,Ypos+1,0xf800);	
}	

/*******************************************************************************
* 函数名称：DrawCross
* 说明：在指定座标画十字准星
* 输入：- Xpos：行坐标
* - Ypos：线坐标
* 输出：无
* 返回：无
* 注意：无
*******************************************************************************/
void DrawCross(uint16_t Xpos,uint16_t Ypos)
{
  LCD_DrawLine(Xpos-15,Ypos,Xpos-2,Ypos,0xffff);
  LCD_DrawLine(Xpos+2,Ypos,Xpos+15,Ypos,0xffff);
  LCD_DrawLine(Xpos,Ypos-15,Xpos,Ypos-2,0xffff);
  LCD_DrawLine(Xpos,Ypos+2,Xpos,Ypos+15,0xffff);
  
  LCD_DrawLine(Xpos-15,Ypos+15,Xpos-7,Ypos+15,RGB565CONVERT(184,158,131));
  LCD_DrawLine(Xpos-15,Ypos+7,Xpos-15,Ypos+15,RGB565CONVERT(184,158,131));

  LCD_DrawLine(Xpos-15,Ypos-15,Xpos-7,Ypos-15,RGB565CONVERT(184,158,131));
  LCD_DrawLine(Xpos-15,Ypos-7,Xpos-15,Ypos-15,RGB565CONVERT(184,158,131));

  LCD_DrawLine(Xpos+7,Ypos+15,Xpos+15,Ypos+15,RGB565CONVERT(184,158,131));
  LCD_DrawLine(Xpos+15,Ypos+7,Xpos+15,Ypos+15,RGB565CONVERT(184,158,131));

  LCD_DrawLine(Xpos+7,Ypos-15,Xpos+15,Ypos-15,RGB565CONVERT(184,158,131));
  LCD_DrawLine(Xpos+15,Ypos-15,Xpos+15,Ypos-7,RGB565CONVERT(184,158,131));	  	
}	
	
/*******************************************************************************
* 函数名称：Read_Ads7846
* 说明：得到PK之后的XY
* 输入：无
* 输出：无
* 返回：坐标结构体地址
* 注意：无
*******************************************************************************/
Coordinate *Read_Ads7846(void)
{
  static Coordinate  screen;
  int m0,m1,m2,TP_X[1],TP_Y[1],temp[3];
  uint8_t count=0;
  int buffer[2][9]={{0},{0}};  /* 坐标X和Y进行多次采样 */
  
  do					       /* 循环采样9次 */
  {		   
    TP_GetAdXY(TP_X,TP_Y);  
	buffer[0][count]=TP_X[0];  
	buffer[1][count]=TP_Y[0];
	count++;  
  }
  while(!TP_INT_IN&& count<9);  /* TP_INT_IN为触摸屏中断引脚,当用户点击触摸屏时TP_INT_IN会被置低 */
  if(count==9)   /* 成功采样9次,进行滤波 */ 
  {  
    /* 为减少运算量,分别分3组取平均值 */
    temp[0]=(buffer[0][0]+buffer[0][1]+buffer[0][2])/3;
	temp[1]=(buffer[0][3]+buffer[0][4]+buffer[0][5])/3;
	temp[2]=(buffer[0][6]+buffer[0][7]+buffer[0][8])/3;
	/* 计算3组数据的差值 */
	m0=temp[0]-temp[1];
	m1=temp[1]-temp[2];
	m2=temp[2]-temp[0];
	/* 对上述差值取绝对值 */
	m0=m0>0?m0:(-m0);
    m1=m1>0?m1:(-m1);
	m2=m2>0?m2:(-m2);
	/* 判断绝对差值是否都超过差值门限，如果这3个绝对差值都超过门限值，则判定这次采样点为野点,抛弃采样点，差值门限取为2 */
	if( m0>THRESHOLD  &&  m1>THRESHOLD  &&  m2>THRESHOLD ) return 0;
	/* 计算它们的平均值，同时赋值给screen */ 
	if(m0<m1)
	{
	  if(m2<m0) 
	    screen.x=(4096-(temp[0]+temp[2])/2);
	  else 
	    screen.x=(4096-(temp[0]+temp[1])/2);	
	}
	else if(m2<m1) 
	  screen.x=(4096-(temp[0]+temp[2])/2);
	else 
	  screen.x=(4096-(temp[1]+temp[2])/2);

	/* 同上 计算Y的平均值 */
    temp[0]=(buffer[1][0]+buffer[1][1]+buffer[1][2])/3;
	temp[1]=(buffer[1][3]+buffer[1][4]+buffer[1][5])/3;
	temp[2]=(buffer[1][6]+buffer[1][7]+buffer[1][8])/3;
	m0=temp[0]-temp[1];
	m1=temp[1]-temp[2];
	m2=temp[2]-temp[0];
	m0=m0>0?m0:(-m0);
	m1=m1>0?m1:(-m1);
	m2=m2>0?m2:(-m2);
	if(m0>THRESHOLD&&m1>THRESHOLD&&m2>THRESHOLD) return 0;

	if(m0<m1)
	{
	  if(m2<m0) 
	    screen.y=(temp[0]+temp[2])/2;
	  else 
	    screen.y=(temp[0]+temp[1])/2;	
    }
	else if(m2<m1) 
	   screen.y=(temp[0]+temp[2])/2;
	else
	   screen.y=(temp[1]+temp[2])/2;

	return &screen;
  }  
  return 0; 
}
	 
/* 下面是触摸屏到液晶屏坐标变换的转换函数 */
/* 只有在LCD和触摸屏间的误差角度非常小时,才能运用下面公式 */


/*******************************************************************************
* 函数名称：setCalibrationMatrix
* 说明 : 计算出 K A B C D E F
* 输入：无
* 输出：无
* 返回：返回1表示成功0失败
* 注意：无
*******************************************************************************/
FunctionalState setCalibrationMatrix( Coordinate * displayPtr,
                          Coordinate * screenPtr,
                          Matrix * matrixPtr)
{

  FunctionalState retTHRESHOLD = ENABLE ;
  /* K＝(X0－X2) (Y1－Y2)－(X1－X2) (Y0－Y2) */
  matrixPtr->Divider = ((screenPtr[0].x - screenPtr[2].x) * (screenPtr[1].y - screenPtr[2].y)) - 
                       ((screenPtr[1].x - screenPtr[2].x) * (screenPtr[0].y - screenPtr[2].y)) ;
  if( matrixPtr->Divider == 0 )
  {
    retTHRESHOLD = DISABLE;
  }
  else
  {
    /* A＝((XD0－XD2) (Y1－Y2)－(XD1－XD2) (Y0－Y2))／K	*/
    matrixPtr->An = ((displayPtr[0].x - displayPtr[2].x) * (screenPtr[1].y - screenPtr[2].y)) -
                    ((displayPtr[1].x - displayPtr[2].x) * (screenPtr[0].y - screenPtr[2].y)) ;
	/* B＝((X0－X2) (XD1－XD2)－(XD0－XD2) (X1－X2))／K	*/
    matrixPtr->Bn = ((screenPtr[0].x - screenPtr[2].x) * (displayPtr[1].x - displayPtr[2].x)) - 
                    ((displayPtr[0].x - displayPtr[2].x) * (screenPtr[1].x - screenPtr[2].x)) ;
    /* C＝(Y0(X2XD1－X1XD2)+Y1(X0XD2－X2XD0)+Y2(X1XD0－X0XD1))／K */
    matrixPtr->Cn = (screenPtr[2].x * displayPtr[1].x - screenPtr[1].x * displayPtr[2].x) * screenPtr[0].y +
                    (screenPtr[0].x * displayPtr[2].x - screenPtr[2].x * displayPtr[0].x) * screenPtr[1].y +
                    (screenPtr[1].x * displayPtr[0].x - screenPtr[0].x * displayPtr[1].x) * screenPtr[2].y ;
    /* D＝((YD0－YD2) (Y1－Y2)－(YD1－YD2) (Y0－Y2))／K	*/
    matrixPtr->Dn = ((displayPtr[0].y - displayPtr[2].y) * (screenPtr[1].y - screenPtr[2].y)) - 
                    ((displayPtr[1].y - displayPtr[2].y) * (screenPtr[0].y - screenPtr[2].y)) ;
    /* E＝((X0－X2) (YD1－YD2)－(YD0－YD2) (X1－X2))／K	*/
    matrixPtr->En = ((screenPtr[0].x - screenPtr[2].x) * (displayPtr[1].y - displayPtr[2].y)) - 
                    ((displayPtr[0].y - displayPtr[2].y) * (screenPtr[1].x - screenPtr[2].x)) ;
    /* F＝(Y0(X2YD1－X1YD2)+Y1(X0YD2－X2YD0)+Y2(X1YD0－X0YD1))／K */
    matrixPtr->Fn = (screenPtr[2].x * displayPtr[1].y - screenPtr[1].x * displayPtr[2].y) * screenPtr[0].y +
                    (screenPtr[0].x * displayPtr[2].y - screenPtr[2].x * displayPtr[0].y) * screenPtr[1].y +
                    (screenPtr[1].x * displayPtr[0].y - screenPtr[0].x * displayPtr[1].y) * screenPtr[2].y ;
  }
  return( retTHRESHOLD ) ;
}
u8 lcd_dat1[16];
/*******************************************************************************
* 函数名称：getDisplayPoint
*说明：通过K A B C D E F把通道X Y的值转换为图像屏坐标轴
* 输入：无
* 输出：无
* 返回：返回1表示成功0失败
* 注意：无
*******************************************************************************/
FunctionalState getDisplayPoint(Coordinate * displayPtr,
                     Coordinate * screenPtr,
                     Matrix * matrixPtr )
{
  FunctionalState retTHRESHOLD =ENABLE ;

  if( matrixPtr->Divider != 0 )
  {
    /* XD = AX+BY+C */        
    displayPtr->x = ( (matrixPtr->An * screenPtr->x) + 
                      (matrixPtr->Bn * screenPtr->y) + 
                       matrixPtr->Cn 
                    ) / matrixPtr->Divider ;
	/* YD = DX+EY+F */        
    displayPtr->y = ( (matrixPtr->Dn * screenPtr->x) + 
                      (matrixPtr->En * screenPtr->y) + 
                       matrixPtr->Fn 
                    ) / matrixPtr->Divider ;
  }
  else
  {
    retTHRESHOLD = DISABLE;
  }
  return(retTHRESHOLD);
} 

/*******************************************************************************
* 功能名称：TouchPanel_Calibrate
* 描述：校准原触摸屏
* 输入：无
* 输出：无
* 返回：无
* 注意：无
*******************************************************************************/


void TouchPanel_Calibrate(void)
{
	uint8_t i;
	Coordinate * Ptr;

	for(i=0;i<3;i++)
	{
		LCD_Clear(Black);
		GUI_Chinese(96,10,"三点校准屏幕算法",0xffff,Black);
		delay_ms(500);
		DrawCross(DisplaySample[i].x,DisplaySample[i].y);
		do
		{
			Ptr=Read_Ads7846();
		}
		while( Ptr == (void*)0 );
		ScreenSample[i].x= Ptr->x; ScreenSample[i].y= Ptr->y;
	}
	setCalibrationMatrix( &DisplaySample[0],&ScreenSample[0],&matrix ) ;  /* 送入值得到参数 */	  
	LCD_Clear(Black);
	GUI_Chinese(112,100,"屏幕校准完成",0xffff,Black);
	delay_ms(1500);
	LCD_Clear(Black);
} 

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
