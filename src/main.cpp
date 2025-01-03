/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

/* ----------------------- AVR includes -------------------------------------*/
#include "avr/io.h"
#include "avr/interrupt.h"
#include <util/delay.h>

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define UART_BAUD_RATE 19200

#define REG_INPUT_START 0x1000
#define REG_INPUT_NREGS 4

#define UPDATE_INTERVAL 15
#define USE_RS485 1

/* ----------------------- Static variables ---------------------------------*/
static USHORT usRegInputStart = REG_INPUT_START;
static USHORT usRegInputBuf[REG_INPUT_NREGS];

/* ----------------------- Start implementation -----------------------------*/
void configureRTC()
{
    while (RTC.STATUS > 0) { ; } // Wait for all register to be synchronized
    
    RTC.PER = UPDATE_INTERVAL;
    RTC.CTRLA = RTC_PRESCALER_DIV32768_gc | RTC_RTCEN_bm; // Enable RTC with 1 second clock
    RTC.INTCTRL = RTC_OVF_bm; // Enable overflow interrupt
}

void setup()
{
    eMBErrorCode    eStatus;
        
    PORTMUX.USARTROUTEA = PORTMUX_USART1_DEFAULT_gc | PORTMUX_USART0_NONE_gc;

#if USE_RS485 > 0
    // use RS485 mode
    MBUSART.CTRLA |= USART_RS485_bm;
    PORTA.DIR |= PIN4_bm; // set output pin for RS485 direction control
#endif
    
    // configure port A
    PORTA.DIR = 0;

    configureRTC();
    eStatus = eMBInit( MB_RTU, 0x01, 0, UART_BAUD_RATE, MB_PAR_NONE );
    sei();

    PORTA.DIR |= PIN1_bm; // set output pin for TXD
    PORTA.DIR |= PIN3_bm; // set output for Chanel select so we can chain two boards

    // configure port B
    PORTB.DIR = 0x0F; // multiplexer s0-s2, and enable as output
    PORTB.OUT = 0x08; // set the multiplexer enable to be disabled (active low)

    /* Enable the Modbus Protocol Stack. */
    eStatus = eMBEnable();
    
}

void updateInputRegisters(){

    cli();

    uint16_t output1 = 0, output2 = 0, output3 = 0;
    
    PORTB.OUT = 0x00; // set the multiplexer enable to be enabled (active low)
    for(uint8_t i = 0; i < 8; i++){
        PORTB.OUT = i & 0x07;
        _delay_us(5);

        uint8_t mp1 = (PORTA.IN & PIN5_bm) ? 1 : 0;
        uint8_t mp2 = (PORTA.IN & PIN6_bm) ? 1 : 0;
        uint8_t mp3 = (PORTA.IN & PIN7_bm) ? 1 : 0;

        output1 |= mp1 << i;
        output2 |= mp2 << i;
        output3 |= mp3 << i;
    }
    PORTB.OUT = 0x08; // set the multiplexer enable to be enabled (active low)

    usRegInputBuf[1] = output1;
    usRegInputBuf[2] = output2;
    usRegInputBuf[3] = output3;

    sei();
}

int main( void )
{
    eMBErrorCode    eStatus;

    setup();

    for( ;; )
    {
        eStatus = eMBPoll();

        // for the modbus state maching to work correctly we need to
        // have some delay between the calls to eMBPoll
        _delay_us(10);
    }
}

/* --------------- Freemodbus callback functions ------------------------ */
eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                 eMBRegisterMode eMode )
{
    return MB_ENOREG;
}


eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
    return MB_ENOREG;
}

eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    return MB_ENOREG;
}

/* ------------------------ Interupts handlers -----------------------------*/
ISR(RTC_CNT_vect)
{
    RTC.INTFLAGS |= RTC_OVF_bm;
    RTC.PER = UPDATE_INTERVAL;

    updateInputRegisters();
}