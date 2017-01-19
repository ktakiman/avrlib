#include <util/delay.h>

#include "lcd-pcd8544.h"

// ----------------------------------------------------------------------------
#define PCD8544_FUNC        0x20
#define PCD8544_FUNC_PD     0x04    // power down
#define PCD8544_FUNC_V      0x02    // vertical addressing
#define PCD8544_FUNC_H      0x01    // switch to extended instruction set if set, switch to basic instuction set if not set

// Basic instruction set
#define PCD8544_DISPLAY_BLANK   0x08
#define PCD8544_DISPLAY_ALLON   0x09
#define PCD8544_DISPLAY_NORMAL  0x0c
#define PCD8544_DISPLAY_INVERSE 0x0d

#define PCD8544_SETYADDR    0x40
#define PCD8544_SETXADDR    0x80

// Extended instruction set
#define PCD8544_TEMP_COEF0  0x04
#define PCD8544_TEMP_COEF1  0x05
#define PCD8544_TEMP_COEF2  0x06
#define PCD8544_TEMP_COEF3  0x07

#define PCD8544_BIAS        0x10  // OR with value 0 ~ 0x7
#define PCD8544_VOP         0x80  // OR with value 0 ~ 0x7f

#define PCD8544_BIAS_DEFAULT  0x4   // from an Adafruit example code

uint8_t gHex[] = {'0','1','2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

// ----------------------------------------------------------------------------
volatile uint8_t* gpPort;
volatile uint8_t gSce;
volatile uint8_t gRst;
volatile uint8_t gDc;
volatile uint8_t gDn;
volatile uint8_t gSclk;

// ----------------------------------------------------------------------------
void send(uint8_t data)
{
    uint8_t i;
    
    for (i = 0; i < 8; i++)
    {
        *gpPort &= ~(1 << gSclk);  // CLK Low
    
        if (data & (1 << (7 - i)))
        {
            *gpPort |= (1 << gDn); // Data high
        }
        else
        {
            *gpPort &= ~(1 << gDn); // Data low
        }
        
        *gpPort |= (1 << gSclk); // CLK high
    }
}
// ----------------------------------------------------------------------------
void sendCmnd(uint8_t cmnd)
{
    *gpPort &= ~(1 << gDc);                 // D/C Low
    send(cmnd);
}
// ----------------------------------------------------------------------------
void sendData(uint8_t data)
{
    *gpPort |= (1 << gDc);                 // D/C High
    send(data);
}
// ----------------------------------------------------------------------------
void _pcd8544_init(volatile uint8_t* pPort, uint8_t sce, uint8_t rst, uint8_t dc, uint8_t dn, uint8_t sclk, uint8_t vop)
{
    gpPort = pPort;
    gSce = sce;
    gRst = rst;
    gDc = dc;
    gDn = dn;
    gSclk = sclk;
    
    // Setup
    *gpPort |= (1 << gSce);                // SCE High - Page 11: Serial interface is initialized while SCE is HIGH
    _delay_ms(500);
    *gpPort &= ~(1 << gSce);               // SCE Low  

    *gpPort &= ~(1 << gRst);               // RST LOW
    _delay_ms(500);
    *gpPort |= (1 << gRst);                // RST High- Page11: SCE needs to be LOW on positive edge of RES
    
    sendCmnd(PCD8544_FUNC | PCD8544_FUNC_H);  // switch to extended function mode
    
    sendCmnd(PCD8544_BIAS | PCD8544_BIAS_DEFAULT);
     
    sendCmnd(PCD8544_VOP | vop);
    
    sendCmnd(PCD8544_FUNC);        // switch to normal function mode
    
    sendCmnd(PCD8544_DISPLAY_NORMAL);    
    
    _pcd8544_clearText(1);
}

// ----------------------------------------------------------------------------
// Text mode 
// ----------------------------------------------------------------------------
volatile uint8_t gCharBuf[PCD8544_ROW_MAX * PCD8544_COL_MAX];

volatile uint8_t gCsr = 0;  // console row
volatile uint8_t gCsc = 0;  // console col

// ----------------------------------------------------------------------------
void _pcd8544_renderText()
{
    uint8_t c;
    uint8_t r;
    uint16_t j;
    uint16_t i = 0;

    // Data!
    for (r = 0; r < PCD8544_ROW_MAX; r++)
    {
        for (c = 0; c < PCD8544_COL_MAX; c++)
        {
            uint16_t pos = gCharBuf[i++] - ' ';
        
            for (j = 0; j < 6; j++)
            {
                if (j == 5)
                {
                    sendData(0x00);
                }
                else
                {
                    sendData(Fonts[pos * 5 + j]);
                }
            }
        }
    }
    sendCmnd(PCD8544_SETYADDR);     
}
// ----------------------------------------------------------------------------
void shiftLines()
{
    uint8_t r;
    uint8_t c;
    
    for (r = 0; r < PCD8544_ROW_MAX - 1; r++)
    {
        for (c = 0; c < PCD8544_COL_MAX; c++)
        {
            gCharBuf[r * PCD8544_COL_MAX + c] = gCharBuf[(r + 1) * PCD8544_COL_MAX + c];
        }
    }
    
    for (c = 0; c < PCD8544_COL_MAX; c++)
    {
        gCharBuf[(PCD8544_ROW_MAX - 1) * PCD8544_COL_MAX + c] = ' ';
    }
 }
 // ----------------------------------------------------------------------------
void appendChar(uint8_t ch)
{
    if (ch == '\r' || ch == '\n')
    {
        gCsc = 0;
        gCsr++;
        
        if (gCsr == PCD8544_ROW_MAX)
        {
            shiftLines();
            gCsr = PCD8544_ROW_MAX - 1;
        }            
    }
    else if (0x20 <= ch && ch <= 0x7f)  // ASCII only
    {
        if (gCsc == PCD8544_COL_MAX)
        {
            gCsc = 0;
            gCsr++;
        }
        
        if (gCsr == PCD8544_ROW_MAX)
        {
            shiftLines();
            gCsr = PCD8544_ROW_MAX - 1;
        }            
    
        gCharBuf[gCsr * PCD8544_COL_MAX + gCsc] = ch;
        gCsc++;
    }
}
// ----------------------------------------------------------------------------
void _pcd8544_clearText(uint8_t render)
{
    uint16_t i = 0;
    
    for (i = 0; i < PCD8544_ROW_MAX * PCD8544_COL_MAX; i++)
    {
        gCharBuf[i] = ' ';
    }
    
    gCsr = 0;
    gCsc = 0;
    
    if (render)
    {
        _pcd8544_renderText();
    }
}
// ----------------------------------------------------------------------------
void _pcd8544_clearRow(uint8_t row, uint8_t render)
{
    if (row < PCD8544_ROW_MAX)
    {
        uint16_t start = row * PCD8544_COL_MAX;
        uint16_t i = 0;
        for (i = 0; i < PCD8544_COL_MAX; i++)
        {
            gCharBuf[start + i] = ' ';
        }
    }
    
    if (render)
    {
        _pcd8544_renderText();
    }
}
// ----------------------------------------------------------------------------
void _pcd8544_setText(uint8_t r, uint8_t* pText, uint8_t render)
{
    uint16_t start = PCD8544_COL_MAX * r;
    uint16_t i;
    for (i = 0; i < PCD8544_COL_MAX && pText[i] != '\0'; i++)
    {
        gCharBuf[start + i] = pText[i]; 
    }
    
    if (render)
    {
        _pcd8544_renderText();
    }
}
// ----------------------------------------------------------------------------
void _pcd8544_appendText(uint8_t* pText, uint8_t render)
{
    uint8_t i = 0;
    
    while (pText[i] != '\0')
    {
        appendChar(pText[i++]);
    }
    
    if (render)
    {
        _pcd8544_renderText();
    }
}
// ----------------------------------------------------------------------------
void _pcd8544_appendChar(uint8_t ch, uint8_t render)
{
    appendChar(ch);
    
    if (render)
    {
        _pcd8544_renderText();
    }
}
// ----------------------------------------------------------------------------
void _pcd8544_newLine(uint8_t render)
{
    _pcd8544_appendChar('\r', render);
}
// ----------------------------------------------------------------------------
void _pcd8544_appendByteAsHex(uint8_t byte, uint8_t render)
{
    _pcd8544_appendChar(gHex[byte / 0x10], 0); byte %= 0x10;
    _pcd8544_appendChar(gHex[byte], render);
}
// ----------------------------------------------------------------------------
void _pcd8544_appendWordAsHex(uint16_t wd, uint8_t render)
{
    _pcd8544_appendChar(gHex[wd / 0x1000], 0); wd %= 0x1000;
    _pcd8544_appendChar(gHex[wd / 0x100], 0); wd %= 0x100;
    _pcd8544_appendChar(gHex[wd / 0x10], 0); wd %= 0x10;
    _pcd8544_appendChar(gHex[wd], render);
}
// ----------------------------------------------------------------------------
void _pcd8544_appendByteAsInt(uint8_t byte, uint8_t render)
{
    uint8_t showing = 0;
    if (byte / 100) { _pcd8544_appendChar(gHex[byte / 100], 0); showing = 1; byte %= 100; }
    if (byte / 10 || showing) { _pcd8544_appendChar(gHex[byte / 10], 0); byte %= 10; }
    _pcd8544_appendChar(gHex[byte], render);
}
// ----------------------------------------------------------------------------
void _pcd8544_appendWordAsInt(uint16_t wd, uint8_t render)
{
    uint8_t showing = 0;
    if (wd / 100000) { _pcd8544_appendChar(gHex[wd / 100000], 0); showing = 1; wd %= 100000; }
    if (wd / 10000 || showing) { _pcd8544_appendChar(gHex[wd / 10000], 0); showing = 1; wd %= 10000; }
    if (wd / 1000 || showing) { _pcd8544_appendChar(gHex[wd / 1000], 0); showing = 1; wd %= 1000; }
    if (wd / 100 || showing) { _pcd8544_appendChar(gHex[wd / 100], 0); showing = 1; wd %= 100; }
    if (wd / 10 || showing) { _pcd8544_appendChar(gHex[wd / 10], 0); showing = 1; wd %= 10; }
    _pcd8544_appendChar(gHex[wd], render);
}
// ----------------------------------------------------------------------------
uint8_t _pcd8544_curRow()
{
    return gCsr;
}
// ----------------------------------------------------------------------------
uint8_t _pcd8544_curCol()
{
    return gCsc;
}