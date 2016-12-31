/*
 * atmega3.c
 */

//period: 20000 micro -> ICR1
//timer 1 (A) -> PD5 (PIN 19 of ATMEGA32A) -> channel 5 (auxiliary)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#define aux_width OCR1A

void TIMER1_INIT()
{
    ///clear OC1X on compare match (1010)
    ///no force on output compare (00)
    ///fast pwm WGM13:10 -> (1110)
    ///internal clock, no prescaler (001)

    TCCR1A = 0b10100010;
    TCCR1B = 0b00011001;

    ///period = ICR1+1 (as no prescaler)
    ICR1 = 19999;

    ///width = OCR1A + 1 (^^) 1972 -> on
    aux_width = 1972;
}

int main()
{
    /// PD5 -> auxiliary (safe mode)
    DDRD = 0b00100000;

    TIMER1_INIT();
    sei();

    while(1);
    return 0;
}
