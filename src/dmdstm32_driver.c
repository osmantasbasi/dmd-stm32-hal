/*--------------------------------------------------------------------------------------

 dmdstm32_driver.c - Function and support library for the P10 DMD Led Panel with STM32 HAL Libaries.

 Copyright (C) 2025 Osman Taşbaşı (osman.tasbasi@inorobotics.com)

 Note that the DMD library uses the SPI port for the fastest, low overhead writing to the
 display. Keep an eye on conflicts if there are any other devices running from the same
 SPI port, and that the chip select on those devices is correctly set to be inactive
 when the DMD is being written to.

 ---

 This program is free software: you can redistribute it and/or modify it under the terms
 of the version 3 GNU General Public License as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program.
 If not, see <http://www.gnu.org/licenses/>.

--------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------
 Setup and instantiation of DMD library
 Note this currently uses the SPI port for the fastest performance to the DMD, be
 careful of possible conflicts with other SPI port devices
--------------------------------------------------------------------------------------*/


#include "dmdstm32_driver.h"

uint8_t DisplaysWide;
uint8_t DisplaysHigh;
uint8_t DisplaysTotal;

volatile uint8_t bDMDByte;
int row1, row2, row3;
uint8_t *bDMDScreenRAM = NULL;  
const uint8_t* Font;



//Marquee values
char marqueeText[256];
uint8_t marqueeLength;
int marqueeWidth;
int marqueeHeight;
int marqueeOffsetX;
int marqueeOffsetY;




void initPanel(uint8_t panelsWide, uint8_t panelsHigh)
{
    DisplaysWide=panelsWide;
    DisplaysHigh=panelsHigh;
    DisplaysTotal=DisplaysWide*DisplaysHigh;
    row1 = DisplaysTotal<<4;
    row2 = DisplaysTotal<<5;
    row3 = ((DisplaysTotal<<2)*3)<<2;
    

    free(bDMDScreenRAM);
    bDMDScreenRAM = (uint8_t *)malloc(DisplaysTotal * DMD_RAM_SIZE_BYTES);



}


/*--------------------------------------------------------------------------------------
 Scan the dot matrix LED panel display, from the RAM mirror out to the display hardware.
 Call 4 times to scan the whole display which is made up of 4 interleaved rows within the 16 total rows.
 Insert the calls to this function into the main loop for the highest call rate, or from a timer interrupt
--------------------------------------------------------------------------------------*/
void scanDisplayBySPI(void)
{
    
        //SPI transfer pixels to the display hardware shift registers
        int rowsize=DisplaysTotal<<2;
        int offset=rowsize * bDMDByte;


        for(int i = 0; i < rowsize; ++i){
            uint8_t byte1 = bDMDScreenRAM[offset + i + row3];
            uint8_t byte2 = bDMDScreenRAM[offset + i + row2];
            uint8_t byte3 = bDMDScreenRAM[offset + i + row1];
            uint8_t byte4 = bDMDScreenRAM[offset + i];

            HAL_SPI_Transmit(&PANEL_SPI, &byte1, 1, 100);
            HAL_SPI_Transmit(&PANEL_SPI, &byte2, 1, 100);
            HAL_SPI_Transmit(&PANEL_SPI, &byte3, 1, 100);
            HAL_SPI_Transmit(&PANEL_SPI, &byte4, 1, 100);
        }

        //OE_DMD_ROWS_OFF();
        LATCH_DMD_SHIFT_REG_TO_OUTPUT();
        switch (bDMDByte) {
        case 0:			// row 1, 5, 9, 13 were clocked out
            LIGHT_DMD_ROW_01_05_09_13();
            bDMDByte=1;
            break;
        case 1:			// row 2, 6, 10, 14 were clocked out
            LIGHT_DMD_ROW_02_06_10_14();
            bDMDByte=2;
            break;
        case 2:			// row 3, 7, 11, 15 were clocked out
            LIGHT_DMD_ROW_03_07_11_15();
            bDMDByte=3;
            break;
        case 3:			// row 4, 8, 12, 16 were clocked out
            LIGHT_DMD_ROW_04_08_12_16();
            bDMDByte=0;
            break;
        }
        //OE_DMD_ROWS_ON();
    
}



void drawMarquee(const char *bChars, uint8_t length, int left, int top) {
    marqueeWidth = 0;
    
    for (uint8_t i = 0; i < length; i++) {
        marqueeText[i] = bChars[i];
        marqueeWidth += charWidth(bChars[i]) + 1;
    }

    marqueeHeight = Font[FONT_HEIGHT];  // STM32'de doğrudan RAM erişimi
    marqueeText[length] = '\0';  // String sonlandırma
    marqueeOffsetY = top;
    marqueeOffsetX = left;
    marqueeLength = length;

    drawString(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength, GRAPHICS_NORMAL);
}

void drawString(int bX, int bY, const char *bChars, uint8_t length, uint8_t bGraphicsMode) {
    if (bX >= (DMD_PIXELS_ACROSS * DisplaysWide) || bY >= DMD_PIXELS_DOWN * DisplaysHigh) return;
    uint8_t height = Font[FONT_HEIGHT];
    if (bY + height < 0) return;

    int strWidth = 0;
    drawLine(bX - 1, bY, bX - 1, bY + height, GRAPHICS_INVERSE);

    for (uint8_t i = 0; i < length; i++) {
        int charWide = drawChar(bX + strWidth, bY, bChars[i], bGraphicsMode);
        if (charWide > 0) {
            strWidth += charWide;
            drawLine(bX + strWidth, bY, bX + strWidth, bY + height, GRAPHICS_INVERSE);
            strWidth++;
        } else if (charWide < 0) {
            return;
        }
        if ((bX + strWidth) >= DMD_PIXELS_ACROSS * DisplaysWide || bY >= DMD_PIXELS_DOWN * DisplaysHigh) return;
    }
}


void drawLine(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode) {
    int dy = y2 - y1;
    int dx = x2 - x1;
    int stepx = (dx < 0) ? -1 : 1;
    int stepy = (dy < 0) ? -1 : 1;

    dx = (dx < 0) ? -dx : dx;
    dy = (dy < 0) ? -dy : dy;
    
    dy <<= 1;
    dx <<= 1;

    writePixel(x1, y1, bGraphicsMode, 1);
    
    if (dx > dy) {
        int fraction = dy - (dx >> 1);
        while (x1 != x2) {
            if (fraction >= 0) {
                y1 += stepy;
                fraction -= dx;
            }
            x1 += stepx;
            fraction += dy;
            writePixel(x1, y1, bGraphicsMode, 1);
        }
    } else {
        int fraction = dx - (dy >> 1);
        while (y1 != y2) {
            if (fraction >= 0) {
                x1 += stepx;
                fraction -= dy;
            }
            y1 += stepy;
            fraction += dx;
            writePixel(x1, y1, bGraphicsMode, 1);
        }
    }
}


void writePixel(uint16_t bX, uint16_t bY, uint8_t bGraphicsMode, uint8_t bPixel) {
    if (bX >= (DMD_PIXELS_ACROSS * DisplaysWide) || bY >= (DMD_PIXELS_DOWN * DisplaysHigh)) {
        return;
    }

    uint8_t panel = (bX / DMD_PIXELS_ACROSS) + (DisplaysWide * (bY / DMD_PIXELS_DOWN));
    bX = (bX % DMD_PIXELS_ACROSS) + (panel << 5);
    bY = bY % DMD_PIXELS_DOWN;

    uint16_t uiDMDRAMPointer = (bX / 8) + (bY * (DisplaysTotal << 2));
    uint8_t lookup = bPixelLookupTable[bX & 0x07];

    switch (bGraphicsMode) {
        case GRAPHICS_NORMAL:
            if (bPixel)
                bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup;
            else
                bDMDScreenRAM[uiDMDRAMPointer] |= lookup;
            break;

        case GRAPHICS_INVERSE:
            if (!bPixel)
                bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup;
            else
                bDMDScreenRAM[uiDMDRAMPointer] |= lookup;
            break;

        case GRAPHICS_TOGGLE:
            if (bPixel) {
                if ((bDMDScreenRAM[uiDMDRAMPointer] & lookup) == 0)
                    bDMDScreenRAM[uiDMDRAMPointer] |= lookup;
                else
                    bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup;
            }
            break;

        case GRAPHICS_OR:
            if (bPixel)
                bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup;
            break;

        case GRAPHICS_NOR:
            if (bPixel && ((bDMDScreenRAM[uiDMDRAMPointer] & lookup) == 0))
                bDMDScreenRAM[uiDMDRAMPointer] |= lookup;
            break;
    }
}



int drawChar(int bX, int bY, unsigned char letter, uint8_t bGraphicsMode) {
    if (bX > (DMD_PIXELS_ACROSS * DisplaysWide) || bY > (DMD_PIXELS_DOWN * DisplaysHigh)) {
        return -1;
    }

    uint8_t height = Font[FONT_HEIGHT];
    if (letter == ' ') {
        int charWide = charWidth(' ');
        drawFilledBox(bX, bY, bX + charWide, bY + height, GRAPHICS_INVERSE);
        return charWide;
    }

    uint8_t width = 0;
    uint8_t bytes = (height + 7) / 8;
    uint8_t firstChar = Font[FONT_FIRST_CHAR];
    uint8_t charCount = Font[FONT_CHAR_COUNT];
    uint16_t index = 0;

    if (letter < firstChar || letter >= (firstChar + charCount)) {
        return 0;
    }

    letter -= firstChar;

    if (Font[FONT_LENGTH] == 0 && Font[FONT_LENGTH + 1] == 0) {
        width = Font[FONT_FIXED_WIDTH];
        index = letter * bytes * width + FONT_WIDTH_TABLE;
    } else {
        for (uint8_t i = 0; i < letter; i++) {
            index += Font[FONT_WIDTH_TABLE + i];
        }
        index = index * bytes + charCount + FONT_WIDTH_TABLE;
        width = Font[FONT_WIDTH_TABLE + letter];
    }

    if (bX < -width || bY < -height) {
        return width;
    }

    for (uint8_t j = 0; j < width; j++) {
        for (uint8_t i = bytes - 1; i < 254; i--) {
            uint8_t data = Font[index + j + (i * width)];
            int offset = (i * 8);
            if ((i == bytes - 1) && bytes > 1) {
                offset = height - 8;
            }
            for (uint8_t k = 0; k < 8; k++) {
                if ((offset + k >= i * 8) && (offset + k <= height)) {
                    if (data & (1 << k)) {
                        writePixel(bX + j, bY + offset + k, bGraphicsMode, true);
                    } else {
                        writePixel(bX + j, bY + offset + k, bGraphicsMode, false);
                    }
                }
            }
        }
    }

    return width;
}

/*--------------------------------------------------------------------------------------
 Draw or clear a filled box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/
void drawFilledBox(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode) {
    for (int b = x1; b <= x2; b++) {
        drawLine(b, y1, b, y2, bGraphicsMode);
    }
}

int charWidth(const unsigned char letter) {
    unsigned char c = letter;
    if (c == ' ') c = 'n';  

    uint8_t width = 0;
    uint8_t firstChar = Font[FONT_FIRST_CHAR];
    uint8_t charCount = Font[FONT_CHAR_COUNT];

    if (c < firstChar || c >= (firstChar + charCount)) {
        return 0;
    }
    c -= firstChar;

    if (Font[FONT_LENGTH] == 0 && Font[FONT_LENGTH + 1] == 0) {
        width = Font[FONT_FIXED_WIDTH];  
    } else {
        width = Font[FONT_WIDTH_TABLE + c];  
    }
    return width;
}

bool stepMarquee(int amountX, int amountY) {
    bool ret = false;
    marqueeOffsetX += amountX;
    marqueeOffsetY += amountY;

    if (marqueeOffsetX < -marqueeWidth) {
        marqueeOffsetX = DMD_PIXELS_ACROSS * DisplaysWide;
        clearScreen(true);
        ret = true;
    } else if (marqueeOffsetX > DMD_PIXELS_ACROSS * DisplaysWide) {
        marqueeOffsetX = -marqueeWidth;
        clearScreen(true);
        ret = true;
    }

    if (marqueeOffsetY < -marqueeHeight) {
        marqueeOffsetY = DMD_PIXELS_DOWN * DisplaysHigh;
        clearScreen(true);
        ret = true;
    } else if (marqueeOffsetY > DMD_PIXELS_DOWN * DisplaysHigh) {
        marqueeOffsetY = -marqueeHeight;
        clearScreen(true);
        ret = true;
    }

    // Özel durum: Yatay kaydırma
    if (amountY == 0 && amountX == -1) {
        // Tüm ekranı sola kaydır
        for (int i = 0; i < DMD_RAM_SIZE_BYTES * DisplaysTotal; i++) {
            if ((i % (DisplaysWide * 4)) == (DisplaysWide * 4) - 1) {
                bDMDScreenRAM[i] = (bDMDScreenRAM[i] << 1) + 1;
            } else {
                bDMDScreenRAM[i] = (bDMDScreenRAM[i] << 1) + ((bDMDScreenRAM[i + 1] & 0x80) >> 7);
            }
        }

        // Son karakteri tekrar çiz
        int strWidth = marqueeOffsetX;
        for (uint8_t i = 0; i < (uint8_t)marqueeLength; i++) {
            int wide = charWidth(marqueeText[i]);
            if (strWidth + wide >= DisplaysWide * DMD_PIXELS_ACROSS) {
                drawChar(strWidth, marqueeOffsetY, marqueeText[i], GRAPHICS_NORMAL);
                return ret;
            }
            strWidth += wide + 1;
        }
    } else if (amountY == 0 && amountX == 1) {
        // Tüm ekranı sağa kaydır
        for (int i = (DMD_RAM_SIZE_BYTES * DisplaysTotal) - 1; i >= 0; i--) {
            if ((i % (DisplaysWide * 4)) == 0) {
                bDMDScreenRAM[i] = (bDMDScreenRAM[i] >> 1) + 128;
            } else {
                bDMDScreenRAM[i] = (bDMDScreenRAM[i] >> 1) + ((bDMDScreenRAM[i - 1] & 1) << 7);
            }
        }

        // Son karakteri tekrar çiz
        int strWidth = marqueeOffsetX;
        for (uint8_t i = 0; i < (uint8_t)marqueeLength; i++) {
            int wide = charWidth(marqueeText[i]);
            if (strWidth + wide >= 0) {
                drawChar(strWidth, marqueeOffsetY, marqueeText[i], GRAPHICS_NORMAL);
                return ret;
            }
            strWidth += wide + 1;
        }
    } else {
        drawString(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength, GRAPHICS_NORMAL);
    }

    return ret;
}

/*--------------------------------------------------------------------------------------
 Draw the selected test pattern
--------------------------------------------------------------------------------------*/
void drawTestPattern(uint8_t bPattern) {
    uint32_t ui;
    int numPixels = DisplaysTotal * DMD_PIXELS_ACROSS * DMD_PIXELS_DOWN;
    int pixelsWide = DMD_PIXELS_ACROSS * DisplaysWide;

    for (ui = 0; ui < numPixels; ui++) {
        int x = ui & (pixelsWide - 1);
        int y = (ui & ~(pixelsWide - 1)) / pixelsWide;
        
        switch (bPattern) {
            case PATTERN_ALT_0: // Her iki pikselde bir, ilk piksel açık
                if ((ui & pixelsWide) == 0) {
                    // Çift satır
                    writePixel(x, y, GRAPHICS_NORMAL, ui & 1);
                } else {
                    // Tek satır
                    writePixel(x, y, GRAPHICS_NORMAL, !(ui & 1));
                }
                break;
            
            case PATTERN_ALT_1: // Her iki pikselde bir, ilk piksel kapalı
                if ((ui & pixelsWide) == 0) {
                    // Çift satır
                    writePixel(x, y, GRAPHICS_NORMAL, !(ui & 1));
                } else {
                    // Tek satır
                    writePixel(x, y, GRAPHICS_NORMAL, ui & 1);
                }
                break;

            case PATTERN_STRIPE_0: // Dikey çizgiler, ilk çizgi açık
                writePixel(x, y, GRAPHICS_NORMAL, ui & 1);
                break;

            case PATTERN_STRIPE_1: // Dikey çizgiler, ilk çizgi kapalı
                writePixel(x, y, GRAPHICS_NORMAL, !(ui & 1));
                break;
        }
    }
}

void drawCircle(int xCenter, int yCenter, int radius, uint8_t bGraphicsMode)
{
    int x = 0;
    int y = radius;
    int p = (5 - radius * 4) / 4;

    drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
    while (x < y) {
    x++;
    if (p < 0) {
    p += 2 * x + 1;
    } else {
    y--;
    p += 2 * (x - y) + 1;
    }
    drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
    }
}

void drawCircleSub(int cx, int cy, int x, int y, uint8_t bGraphicsMode)
{

    if (x == 0) {
	    writePixel(cx, cy + y, bGraphicsMode, true);
	    writePixel(cx, cy - y, bGraphicsMode, true);
	    writePixel(cx + y, cy, bGraphicsMode, true);
	    writePixel(cx - y, cy, bGraphicsMode, true);
    } else if (x == y) {
	    writePixel(cx + x, cy + y, bGraphicsMode, true);
	    writePixel(cx - x, cy + y, bGraphicsMode, true);
	    writePixel(cx + x, cy - y, bGraphicsMode, true);
	    writePixel(cx - x, cy - y, bGraphicsMode, true);
    } else if (x < y) {
	    writePixel(cx + x, cy + y, bGraphicsMode, true);
	    writePixel(cx - x, cy + y, bGraphicsMode, true);
	    writePixel(cx + x, cy - y, bGraphicsMode, true);
	    writePixel(cx - x, cy - y, bGraphicsMode, true);
	    writePixel(cx + y, cy + x, bGraphicsMode, true);
	    writePixel(cx - y, cy + x, bGraphicsMode, true);
	    writePixel(cx + y, cy - x, bGraphicsMode, true);
	    writePixel(cx - y, cy - x, bGraphicsMode, true);
    }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/
void drawBox(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode)
{
    drawLine(x1, y1, x2, y1, bGraphicsMode);
    drawLine(x2, y1, x2, y2, bGraphicsMode);
    drawLine(x2, y2, x1, y2, bGraphicsMode);
    drawLine(x1, y2, x1, y1, bGraphicsMode);
}


void selectFont(const uint8_t * font) {
    Font = font;
}

void clearScreen(uint8_t bNormal)
{
    if (bNormal) // clear all pixels
        memset(bDMDScreenRAM,0xFF,DMD_RAM_SIZE_BYTES*DisplaysTotal);
    else // set all pixels
        memset(bDMDScreenRAM,0x00,DMD_RAM_SIZE_BYTES*DisplaysTotal);
}

void setBrightness(uint8_t value)
{
	uint8_t brightness = min(max(value, 0), 100);
    __HAL_TIM_SET_COMPARE(&OE_HTIM, OE_TIM_CHANNEL, (OE_HTIM.Init.Period * brightness) / 100);
}
