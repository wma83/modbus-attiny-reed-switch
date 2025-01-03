/*
 * FreeModbus Libary: ATMega168 Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *   - Initial version and ATmega168 support
 * Modfications Copyright (C) 2024 Marcin Wrzyciel:
 *   - ATTiny1624 support
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

#define UART_BAUD_CALC(UART_BAUD_RATE, F_OSC) \
    ( ( F_OSC ) / ( ( UART_BAUD_RATE ) * 16UL ) - 1 )

void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    MBUSART.CTRLB |= USART_TXEN_bm;

    if( xRxEnable )  
    {
        MBUSART.CTRLA |= USART_RXCIE_bm;
        MBUSART.CTRLB |= USART_RXEN_bm;
    }
    else
    {
        MBUSART.CTRLA &= ~USART_RXCIE_bm;
        MBUSART.CTRLB &= ~USART_RXEN_bm;
    }

    if( xTxEnable )
    {
        MBUSART.CTRLA |= USART_DREIE_bm;
    }
    else
    {
        MBUSART.CTRLA &= ~USART_DREIE_bm;
    }
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
    
    /* prevent compiler warning. */
    (void)ucPORT;
	
    MBUSART.BAUD = UART_BAUD_CALC( ulBaudRate, 64 * F_CPU / MAIN_CLOCK_PRESCALER );

    switch ( eParity )
    {
        case MB_PAR_EVEN:
            MBUSART.CTRLC |= USART_PMODE_EVEN_gc;
            break;
        case MB_PAR_ODD:
            MBUSART.CTRLC |= USART_PMODE_ODD_gc;
            break;
        case MB_PAR_NONE:
             MBUSART.CTRLC |= USART_PMODE_DISABLED_gc;
            break;
    }

    switch ( ucDataBits )
    {
        case 8:
            MBUSART.CTRLC |= USART_CHSIZE_8BIT_gc;
            break;
        case 7:
            MBUSART.CTRLC |= USART_CHSIZE_7BIT_gc;
            break;
    }

    vMBPortSerialEnable( FALSE, FALSE );
    return TRUE;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
    MBUSART.TXDATAL = ucByte;
    return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
    *pucByte  = MBUSART.RXDATAL;
    return TRUE;
}

ISR(MBUSART_DRE_vect)
{
    pxMBFrameCBTransmitterEmpty(  );
}

ISR(MBUSART_RXC_vect)
{
    pxMBFrameCBByteReceived(  );
}