#ifndef _TOUCHPANEL_H_
#define _TOUCHPANEL_H_

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/* Private typedef -----------------------------------------------------------*/
typedef	struct POINT 
{
   uint16_t x;
   uint16_t y;
}Coordinate;


typedef struct Matrix 
{						
long double An,  
            Bn,     
            Cn,   
            Dn,    
            En,    
            Fn,     
            Divider ;
} Matrix ;

/* Private variables ---------------------------------------------------------*/
extern Coordinate ScreenSample[3];
extern Coordinate DisplaySample[3];
extern Matrix matrix ;
extern Coordinate  display ;

/* Private define ------------------------------------------------------------*/
/* AD通道选择命令字和工作寄存器 */
#define	CHX 	0x90 	/* 通道Y+的选择控制字 */	
#define	CHY 	0xd0	/* 通道X+的选择控制字 */


#define TP_CS(x)	x ? GPIO_SetBits(GPIOC,GPIO_Pin_4): GPIO_ResetBits(GPIOC,GPIO_Pin_4)

#define TP_INT_IN   GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_5)


/* Private function prototypes -----------------------------------------------*/				
void TP_Init(void);	
Coordinate *Read_Ads7846(void);
void TouchPanel_Calibrate(void);
void DrawCross(uint16_t Xpos,uint16_t Ypos);
void TP_DrawPoint(uint16_t Xpos,uint16_t Ypos);
FunctionalState setCalibrationMatrix( Coordinate * displayPtr,Coordinate * screenPtr,Matrix * matrixPtr);
FunctionalState getDisplayPoint(Coordinate * displayPtr,Coordinate * screenPtr,Matrix * matrixPtr );

#endif

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/


