/*
 * pid_usb_mouse.h
 *
 *  Created on: Aug 23, 2022
 *      Author: Konstantin
 */

#ifndef _PID_USB_MOUSE_H_
#define _PID_USB_MOUSE_H_

#include "main.h"
#include "usbh_hid_mouse.h"
#include "GUI.h"

extern GUI_PID_STATE mouse_pid_state;
void modify_mouse_pid_state (HID_MOUSE_Info_TypeDef *mouse_info);

#endif /* _PID_USB_MOUSE_H_ */
