# Demo project for STM32F407ZET6 and TFT LCD 240x320.

Working with the display using FSMC 16 bit. CS_MAIN - FSMC_NE4. RS - FSMC_A6.
FSMC Memory Bank1 NOR/PSRAM. Data address 0x6C000080, Command address 0x6C000000.

The backlight of the display is controlled by pin PB 15. Can use PWM.

Drivers for working with ili9341 are located in the ili9341 folder.
Display initialization taken from https://narodstream.ru/stm-urok-37-displej-tft-240x320-8bit-chast-3/. 

Files lcd_work.h and lcd_work.c contain a custom library for drawing geometric shapes, symbols, numbers and strings on the display.

## The STemWin library is connected. 
When building a project with the help of gcc (for example using CUBEMX_IDE), you must include the library file from ST in the project, for example STemWin_CM4_wc32.a. Before its name it is necessary to put :

An example of setting up a project to include a library file:
![Image alt](https://github.com/NKP144/stm32_tft_lcd/blob/master/ScreenShots/Adding%20STemWin%20lib.png)

When building with the STemWin library, the lcd_work.c file must be excluded from the build, otherwise there will be a conflict of function names.

Window Screenshot:
![Image alt](https://github.com/NKP144/stm32_tft_lcd/blob/master/ScreenShots/GUI%20Window.png)

The window contains text, a radio buttons, checkboxes, progbar and buttons.
When an element is clicked, the text at the top of the window displays the name of the clicked element. Checkboxes control LEDs on the board.
When you click on the buttons, the value of the progress bar increases or decreases.
An arrow cursor is also displayed.

## USB HOST HID mouse is connected.
It is possible to connect a USB mouse.
For the mouse to work properly, you may need to change the HID_MOUSE_Info_TypeDef structure in the usbh_hid_mouse.h file. And also change the processing of data after the parser report descriptor in the usb_hid_mouse.c file.
PID cursor control driver added for STemWin library. Used with API mouse driver.

### Video of GUI work:
https://user-images.githubusercontent.com/25184619/186236789-c0cad1a9-6cff-4943-9dfa-bbaf0ca0bf04.mp4

## Adding RTOS (FreeRTOS)
### USB HOST setup
In order to use RTOS, USBH_PROCESS_STACK_SIZE must be configured. I have it equal to (uint16_t) 2048.
### STemWin setup
1. It is necessary to connect another library, the file name must contain OS. I used STemWin_CM4_OS_wc32.a.
2. STemWin uses CMSIS RTOS API v1.
3. Instead of the GUI _X.c file, you must use GUI_X_OS.c
4. OS_SUPPORT must be declared in GUIConf.h.

## Added saving display image to USB FLASH
The code is on the drawing branch. 
The GUI_BMP_SerializeExBpp() function is used for saving. The BitsPerPixel parameter requires a per-color depth. I used GUICC_565 = RGB565, 2 bpp encoding.

![man](https://user-images.githubusercontent.com/25184619/186511480-dac04318-2d03-4f05-b5f7-205897575a7d.png)
![0](https://user-images.githubusercontent.com/25184619/186513298-cc71f3e3-f219-4247-badc-de6b85976f0e.png)



