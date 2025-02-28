/*--------------------------------------------------------------------------------------

 dmdstm32_driver.h   - Function and support library for the P10 DMD Led Panel with STM32 HAL Libaries.

 Copyright (C) 2025 Osman Taşbaşı (osman.tasbasi@inorobotics.com)

 Note that the DMD library uses the SPI port for the fastest, low overhead writing to the
 display. Keep an eye on conflicts if there are any other devices running from the same
 SPI port, and that the chip select on those devices is correctly set to be inactive
 when the DMD is being written to.


LED Panel Layout in RAM
                            32 pixels (4 bytes)
        top left  ----------------------------------------
                  |                                      |
         Screen 1 |        512 pixels (64 bytes)         | 16 pixels
                  |                                      |
                  ---------------------------------------- bottom right

 ---
 !NOTE: This software is written for P10 DMD Led Panel drive with STM32 using STM HAL Libraries.
 
 This program is free software: you can redistribute it and/or modify it under the terms
 of the version 3 GNU General Public License as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program.
 If not, see <http://www.gnu.org/licenses/>.

 ---
 CONFIGURATION:
Description: When using this library there is parameters that are must be configured. 

PIN_DMD_nOE: 
   Desc: Setting this low lights all the LEDs in the selected rows. Can pwm it at very high frequency for brightness control.
   Conf: Analog output. Set pwm frequency to 490Hz.
PIN_DMD_A:
   Conf: GPIO_OUPUT
PIN_DMD_B:
   Conf: GPIO_OUPUT
PIN_DMD_SCLK:
   Conf: GPIO_OUPUT

OE_TIM:
   Desc: Which TIM is using for pwm output
   Conf: Default TIM1
OE_HTIM:
   Desc: Which TIM pointer is using
   Conf: Default htim1
OE_TIM_CHANNEL:
   Desc: Which channel is using at TIM for pwm output.
   Conf: Default TIM_CHANNEL_1

PANEL_SPI:
   Desc: Which spi is using for communication for data transferring
   Conf: Default SPI1
         Mode: Master
         Direction: Full Duplex or Half Duplex
         DataSize: 8
         CLKPolarity: LOW
         CLKPhase: EDGE
         NSS: SOFTWARE
         BaudRatePrescaler: 256 (If it is not in your cubemx selecet the highest value and test it)
         FirstBit: MSB
         CRCCalculation: DISABLE


--------------------------------------------------------------------------------------*/


#ifndef DMD_STM32_DRIVER_H
#define DMD_STM32_DRIVER_H


#include "main.h"
#include "stdint.h"
#include "stdbool.h"




#define PIN_DMD_nOE PIN_DMD_nOE_Pin
#define PIN_DMD_A   PIN_DMD_A_Pin
#define PIN_DMD_B   PIN_DMD_B_Pin      
#define PIN_DMD_SCLK PIN_DMD_SCLK_Pin    


#define PIN_DMD_nOE_Port PIN_DMD_nOE_GPIO_Port
#define PIN_DMD_A_Port   PIN_DMD_A_GPIO_Port
#define PIN_DMD_B_Port   PIN_DMD_B_GPIO_Port      
#define PIN_DMD_SCLK_Port PIN_DMD_SCLK_GPIO_Port    

#define OE_TIM          TIM1
#define OE_HTIM         htim1
#define OE_TIM_CHANNEL  TIM_CHANNEL_1

#define PANEL_SPI       hspi1

extern TIM_HandleTypeDef OE_HTIM;
extern SPI_HandleTypeDef PANEL_SPI;


#define LIGHT_DMD_ROW_01_05_09_13()       { HAL_GPIO_WritePin(PIN_DMD_B_Port, PIN_DMD_B, GPIO_PIN_RESET); HAL_GPIO_WritePin(PIN_DMD_A_Port, PIN_DMD_A, GPIO_PIN_RESET); }
#define LIGHT_DMD_ROW_02_06_10_14()       { HAL_GPIO_WritePin(PIN_DMD_B_Port, PIN_DMD_B, GPIO_PIN_RESET); HAL_GPIO_WritePin(PIN_DMD_A_Port, PIN_DMD_A, GPIO_PIN_SET); }
#define LIGHT_DMD_ROW_03_07_11_15()       { HAL_GPIO_WritePin(PIN_DMD_B_Port, PIN_DMD_B, GPIO_PIN_SET);   HAL_GPIO_WritePin(PIN_DMD_A_Port, PIN_DMD_A, GPIO_PIN_RESET); }
#define LIGHT_DMD_ROW_04_08_12_16()       { HAL_GPIO_WritePin(PIN_DMD_B_Port, PIN_DMD_B, GPIO_PIN_SET);   HAL_GPIO_WritePin(PIN_DMD_A_Port, PIN_DMD_A, GPIO_PIN_SET); }
#define LATCH_DMD_SHIFT_REG_TO_OUTPUT()   { HAL_GPIO_WritePin(PIN_DMD_SCLK_Port, PIN_DMD_SCLK, GPIO_PIN_SET); HAL_GPIO_WritePin(PIN_DMD_SCLK_GPIO_Port, PIN_DMD_SCLK, GPIO_PIN_RESET); }
#define OE_DMD_ROWS_OFF()                 { __HAL_TIM_SET_COMPARE(&OE_HTIM, OE_TIM_CHANNEL, (OE_HTIM.Init.Period * 0) / 100); }
#define OE_DMD_ROWS_ON()                  { __HAL_TIM_SET_COMPARE(&OE_HTIM, OE_TIM_CHANNEL, (OE_HTIM.Init.Period * 100) / 100); }

// #define OE_DMD_ROWS_OFF()                 { HAL_GPIO_WritePin(PIN_DMD_nOE_Port, PIN_DMD_nOE_Pin, GPIO_PIN_RESET); }
// #define OE_DMD_ROWS_ON()                  { HAL_GPIO_WritePin(PIN_DMD_nOE_Port, PIN_DMD_nOE_Pin, GPIO_PIN_SET); }

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))


//Pixel/graphics writing modes (bGraphicsMode)
#define GRAPHICS_NORMAL    0
#define GRAPHICS_INVERSE   1
#define GRAPHICS_TOGGLE    2
#define GRAPHICS_OR        3
#define GRAPHICS_NOR       4

//drawTestPattern Patterns
#define PATTERN_ALT_0     0
#define PATTERN_ALT_1     1
#define PATTERN_STRIPE_0  2
#define PATTERN_STRIPE_1  3


#define DMD_PIXELS_ACROSS         32      //pixels across x axis (base 2 size expected)
#define DMD_PIXELS_DOWN           16      //pixels down y axis
#define DMD_BITSPERPIXEL           1      //1 bit per pixel, use more bits to allow for pwm screen brightness control
#define DMD_RAM_SIZE_BYTES        ((DMD_PIXELS_ACROSS*DMD_BITSPERPIXEL/8)*DMD_PIXELS_DOWN)



// Font Indices
#define FONT_LENGTH             0
#define FONT_FIXED_WIDTH        2
#define FONT_HEIGHT             3
#define FONT_FIRST_CHAR         4
#define FONT_CHAR_COUNT         5
#define FONT_WIDTH_TABLE        6


static uint8_t bPixelLookupTable[8] =
{
   0x80,   //0, bit 7
   0x40,   //1, bit 6
   0x20,   //2. bit 5
   0x10,   //3, bit 4
   0x08,   //4, bit 3
   0x04,   //5, bit 2
   0x02,   //6, bit 1
   0x01    //7, bit 0
};


//Draw or clear a filled box(rectangle) with a single pixel border
void drawFilledBox(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode);

//Draw a single character
int drawChar(int bX, int bY, unsigned char letter, uint8_t bGraphicsMode);

//Set or clear a pixel at the x and y location (0,0 is the top left corner)
void writePixel(uint16_t bX, uint16_t bY, uint8_t bGraphicsMode, uint8_t bPixel);

//Draw or clear a line from x1,y1 to x2,y2
void drawLine(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode);

//Draw a string
void drawString(int bX, int bY, const char *bChars, uint8_t length, uint8_t bGraphicsMode);

//Draw a scrolling string
void drawMarquee(const char *bChars, uint8_t length, int left, int top);

//Move the maquee accross by amount
bool stepMarquee(int amountX, int amountY);

//Draw or clear a box(rectangle) with a single pixel border
void drawBox(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode);

// Initialize panel type and parameters
void initPanel(uint8_t panelsWide, uint8_t panelsHigh);

//Find the width of a character
int charWidth(const unsigned char letter);

//Select a text font
void selectFont(const uint8_t * font);

//Clear the screen in DMD RAM
void clearScreen(uint8_t bNormal);

//Draw the selected test pattern
void drawTestPattern(uint8_t bPattern);

//Draw or clear a circle of radius r at x,y centre
void drawCircle(int xCenter, int yCenter, int radius, uint8_t bGraphicsMode);
void drawCircleSub(int cx, int cy, int x, int y, uint8_t bGraphicsMode);

// Set brightness 0-100
void setBrightness(uint8_t value);

//Scan the dot matrix LED panel display, from the RAM mirror out to the display hardware.
//Call 4 times to scan the whole display which is made up of 4 interleaved rows within the 16 total rows.
//Insert the calls to this function into the main loop for the highest call rate, or from a timer interrupt
void scanDisplayBySPI(void);
#endif // !DMD_STM32_DRIVER_H
