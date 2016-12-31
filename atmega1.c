/*
 * atmega1.c
 */

//period: 20000 micro -> ICR1
//timer 1 (A) -> PD5 (PIN 19 of ATMEGA32A) -> channel 1 (aileron)
//timer 1 (B) -> PD4 (PIN 18 of ATMEGA32A) -> channel 3 (throttle)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#define before_throttle 0
#define throttle_up 1
#define throttle_up_complete 2
#define throttle_down 3
#define throttle_down_complete 4
#define no_throttle 5

#define delay_throttle_up 22
#define delay_throttle_up_complete 29
#define delay_throttle_landing 64

#define aileron_width OCR1A
#define throttle_width OCR1B

uint8_t timer0_overflows, timer2_overflows;
volatile uint8_t cur_state, timer2_init_flag;

void TIMER1_INIT()
{
    ///clear OC1X on compare match (1010)
    ///no force on output compare (00)
    ///fast pwm WGM13:10 -> (1110)
    ///internal clock, no prescaler (001)

    TCCR1A = 0b10100010;
    TCCR1B = 0b00011001;

    ICR1 = 19999; ///period = ICR1+1 (as no prescaler)
    aileron_width = 1467; ///width = OCR1A + 1 (^^)
    throttle_width = 1138; /// (^^)
}

void TIMER2_INIT()
{
    timer2_overflows = 0;
    TIMSK |= 0b01000000;    ///enable timer2 overflow interrupt
    TCCR2 = 0b00000110; ///internal clock, prescaler (/256)
    TCNT2 = 0;
}

ISR(TIMER2_OVF_vect)
{
    switch(cur_state)
    {
    case throttle_up:
        switch(timer2_overflows)
        {
        case delay_throttle_up: ///increment throttle for 23 overflows
            throttle_width = 1856;  /// 1863
            TIMSK &= 0b10111111;    /// disable timer2 overflow interrupt
            cur_state = throttle_up_complete;
            timer2_init_flag = 1;
            break;
        default:
            throttle_width += 31;
            timer2_overflows++;
        }
        break;

    case throttle_up_complete:
        switch(timer2_overflows)
        {
        case delay_throttle_up_complete: ///remain full throttle for 30 overflows
            TIMSK &= 0b10111111;    /// disable timer2 overflow interrupt
            cur_state = throttle_down;
            throttle_width = 1800;   /// 1814
            timer2_init_flag = 1;
            break;

        default:
            timer2_overflows++;
        }

        break;

    case throttle_down:
        switch(timer2_overflows)
        {
        case delay_throttle_landing: ///remain in down throttle for 40 overflows
            TIMSK &= 0b10111111;    /// disable timer2 overflow interrupt
            cur_state = no_throttle;
            throttle_width = 1138;   ///bring throttle to 0
            break;

        default:
            timer2_overflows++;
        }

        break;

    }
}

ISR(INT0_vect)
{
    cur_state = throttle_up;
    aileron_width = 1452; //adjust aileron for flight

    TIMER2_INIT(); //gradually increment throttle
}


int main()
{
    DDRD = 0b00110000;  /// PD4-> throttle, PD5-> aileron for output
    timer2_init_flag = 0;

    GICR |= (1<<INT0);  ///enable INT0
    MCUCR |= (1<<ISC00) | (1<<ISC01); ///configure for rising edge

    TIMER1_INIT();
    sei();

    while(1){
        if((cur_state == throttle_up_complete || cur_state == throttle_down) && timer2_init_flag){
            timer2_init_flag = 0;
            TIMER2_INIT();
        }
    }
    return 0;
}
