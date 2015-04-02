#include "Arduino.h"
//#include "nokia.h"

#define PIN_SCE   7
#define PIN_RESET 6
#define PIN_DC    5
#define PIN_SDIN  11
#define PIN_SCLK  13

#define LCD_C     LOW
#define LCD_D     HIGH

#define LCD_X     84
#define LCD_Y     48


void LCDchar(char character);
void LCDclear(void);
void LCDinit(void);
void LCDprint(char *characters);
void LCDwrite(byte dc, byte data);
void LCDgotoXY(int x, int y);
