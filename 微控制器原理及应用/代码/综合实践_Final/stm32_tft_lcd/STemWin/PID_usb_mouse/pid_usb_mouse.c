/*
 * pid_usb_mouse.c
 *
 *  Created on: Aug 23, 2022
 *      Author: Konstantin
 */

#include "pid_usb_mouse.h"


GUI_PID_STATE mouse_pid_state;

void modify_mouse_pid_state (HID_MOUSE_Info_TypeDef *mouse_info)
{
	mouse_pid_state.x += (int8_t)mouse_info->x;
	if (mouse_pid_state.x > LCD_GetXSize())
	{
		mouse_pid_state.x = LCD_GetXSize();
	}
	if (mouse_pid_state.x < 0)
	{
		mouse_pid_state.x = 0;
	}

	mouse_pid_state.y += (int8_t)mouse_info->y;
	if (mouse_pid_state.y > LCD_GetYSize())
	{
		mouse_pid_state.y = LCD_GetYSize();
	}
	if (mouse_pid_state.y < 0)
	{
		mouse_pid_state.y = 0;
	}

	mouse_pid_state.Pressed = 0;
	if (mouse_info->buttons[0] != 0) // Left Button Pressed
	{
		mouse_pid_state.Pressed |= 1; // bit 0 - left button
	}

	if (mouse_info->buttons[1] != 0) // Rigth button pressed
	{

		mouse_pid_state.Pressed |= 2;  // bit 1 - right button
	}

	//TODO mouse_pid_state.Layer

	GUI_MOUSE_StoreState(&mouse_pid_state);
}
