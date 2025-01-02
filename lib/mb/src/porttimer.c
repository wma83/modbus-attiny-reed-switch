/*
 * FreeModbus Libary: ATTiny1624 Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
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

/* ----------------------- AVR includes -------------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/

/*
When changing this value remember to update also vMBPortTimersEnable() function
*/
#define MB_TIMER_PRESCALER (16UL)

/*
ATTINY have internal prescaler 6 enabled by default, so we need to divide
the main clock by 6 to get the correct value, this prescaller value can be set
in the Main Clock Prescaler register (CLKCTRL.MCLKCTRLB) if needed.

Corected formula for the 50ms timer ticks is:
50us / (1 / (F_CPU / MB_TIMER_PRESCALER / MAIN_CLOCK_PRESCALER))

After calculating set the value below
*/
#define MB_50US_TICKS (8)

/* ----------------------- Static variables ---------------------------------*/
static USHORT usTimerOCRADelta;
static USHORT usTimerOCRBDelta;

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortTimersInit(USHORT usTim1Timerout50us)
{
    /* Calculate overflow counter an OCR values for Timer1. */
    usTimerOCRADelta = usTim1Timerout50us * MB_50US_TICKS;

    TCA0.SINGLE.CTRLA = 0x00;
    TCA0.SINGLE.CTRLB = 0x00;
    TCA0.SINGLE.CTRLC = 0x00;

    vMBPortTimersDisable();

    return TRUE;
}

inline void
vMBPortTimersEnable()
{
    // set the timer value
    if (usTimerOCRADelta > 0)
    {
        TCA0.SINGLE.PER = usTimerOCRADelta;
        
        // clear current counter value on each enable
        TCA0.SINGLE.CNT = 0x00;

        // enable overflow interrupt
        TCA0.SINGLE.INTCTRL |= 0x01;
    }

    // enable timer with prescaler 16
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm;
}

inline void
vMBPortTimersDisable()
{
    /* Disable the timer. */
    TCA0.SINGLE.CTRLA = 0x00;

    /* Clear interrupts and flags */
    TCA0.SINGLE.INTCTRL = 0x00;
    TCA0.SINGLE.INTFLAGS = 0xFF;
}

ISR(TCA0_OVF_vect)
{
    (void)pxMBPortCBTimerExpired();
}