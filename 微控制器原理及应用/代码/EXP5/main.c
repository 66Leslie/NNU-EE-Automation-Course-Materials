#include <reg51.h>
#include <absacc.h>

/* ¶¨ÒåµØÖ· */
#define PC_8255 XBYTE[0xFF7E]
#define COM_8255 XBYTE[0xFF7F]

#define CTRL_WORD 0x88

void main(void)
{
    unsigned char temp_val;
    COM_8255 = CTRL_WORD;
    while(1)
    {
        temp_val = PC_8255;
        temp_val = temp_val >> 4;
        PC_8255 = temp_val;
    }
}