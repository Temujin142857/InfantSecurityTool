#ifndef LCD_H
#define LCD_H


#include <unistd.h>


void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_SendChar(char c);
void LCD_Print(char *str);

#endif