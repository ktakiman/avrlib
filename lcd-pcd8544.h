#ifndef __LCD_PCD8544_H
#define __LCD_PCD8544_H

#include <avr/io.h>
#include "font5x7.h"

// ----------------------------------------------------------------------------
// SCE   = enable data input, active low
// RST   = reset, active low
// D/C   = Mode select
// DN    = Data
// SCLK  = Clock
// ----------------------------------------------------------------------------

#define PCD8544_ROW_MAX 6
#define PCD8544_COL_MAX 14

#define PCD8544_VOP_DEFAULT   0x3b  // this worked best for the 1st LCD - this really depends on each LCD...

#define PCD8544_FONT_HEART __FONT_HEART + ' '

void _pcd8544_init(volatile uint8_t* pPort, uint8_t sce, uint8_t rst, uint8_t dc, uint8_t dn, uint8_t sclk, uint8_t vop);

// text mode
void _pcd8544_clearText(uint8_t render);
void _pcd8544_clearRow(uint8_t row, uint8_t render);
void _pcd8544_renderText();
void _pcd8544_setText(uint8_t row, uint8_t* pText, uint8_t render);   // pText must be '\0'-ending
void _pcd8544_appendText(uint8_t* pText, uint8_t render);  // pText must be '\0'-ending
void _pcd8544_appendChar(uint8_t ch, uint8_t render);
void _pcd8544_newLine(uint8_t render);

void _pcd8544_appendByteAsHex(uint8_t byte, uint8_t render);
void _pcd8544_appendWordAsHex(uint16_t wd, uint8_t render);
void _pcd8544_appendByteAsInt(uint8_t byte, uint8_t render);
void _pcd8544_appendWordAsInt(uint16_t wd, uint8_t render); 

uint8_t _pcd8544_curRow();
uint8_t _pcd8544_curCol();

// graphics mode (not supported yet, but should not be difficult to implement....)

#endif
