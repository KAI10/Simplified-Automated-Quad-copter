/*
 * atmega2.c
 */

//period: 20000 micro -> ICR1
//timer 1 (A) -> PD5 (PIN 19 of ATMEGA32A) -> channel 4 (Rudder)
//timer 1 (B) -> PD4 (PIN 18 of ATMEGA32A) -> channel 2 (Elevator)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#define rudder_width OCR1A
#define elevator_width OCR1B

#define before_arm_state 0      // initial state
#define arm_state 1             // arming state
#define before_flight_state 2   // arm complete, wait for some time before flight
#define flight_state 3          // follow predefined route

#define delay_before_arm 200    // initial state time, approx 5s, increase it so that almost 12s delay is achieved
#define delay_armed 10          // arming time, approx 2.5s
#define delay_before_flight 29  // waiting time before flight approx 8s

uint16_t timer0_overflows, timer2_overflows;
volatile uint8_t cur_state, timer2_init_flag, flight_state_interrupt_flag;

void TIMER1_INIT()
{
    ///clear OC1X on compare match (1010)
    ///no force on output compare (00)
    ///fast pwm WGM13:10 -> (1110)
    ///internal clock, no prescaler (001)

    TCCR1A = 0b10100010;
    TCCR1B = 0b00011001;

    ICR1 = 19999; ///period = ICR1+1 (as no prescaler)
    rudder_width = 1491; ///width = OCR1A + 1 (^^) 1479
    elevator_width = 1479; /// (^^)
}

void TIMER2_INIT()
{
    timer2_overflows = 0;
    TIMSK |= 0b01000000; ///enable timer2 overflow interrupt
    TCCR2 = 0b00000111; ///internal clock,  prescaler (/1024)
    TCNT2 = 0;
}


ISR(TIMER2_OVF_vect)
{

    switch(cur_state)
    {

    case before_arm_state:

        switch(timer2_overflows)
        {
        case delay_before_arm:
            rudder_width = 1131;    /// required for arming
            TIMSK &= 0b10111111;    /// disable timer2 overflow interrupt
            cur_state = arm_state;
            timer2_init_flag = 1;   /// in main timer2 may now be enabled again for next state
            break;

        default:
            timer2_overflows++;
        }
        break;

    case arm_state:

        switch(timer2_overflows)
        {
        case delay_armed:
            rudder_width = 1491;    /// neutral for waiting mode before flight 1479
            TIMSK &= 0b10111111;    /// disable timer2 overflow interrupt
            cur_state = before_flight_state;
            timer2_init_flag = 1;
            break;

        default:
            timer2_overflows++;
        }

        break;

    case before_flight_state:

        switch(timer2_overflows)
        {
        case delay_before_flight:

            TIMSK &= 0b10111111;    /// disable timer2 overflow interrupt
            PORTA |= 0b00000001;    ///send interrupt to atmega1
            cur_state = flight_state;
            elevator_width = 1522;     ///set elevator value

            PORTA &= 0b11111110;
            break;

        default:
            timer2_overflows++;
        }
        break;
    }
}


int main()
{
    DDRD = 0b00110000;  /// PD4-> elevator, PD5-> rudder for output
    DDRA = 0b00000001;  ///configure PA0 for output, used to send interrupt to atmega1 for initiating flight state

    cur_state = before_arm_state;
    timer2_init_flag = 1;

    TIMER1_INIT();
    sei();

    while(1){
        if(cur_state < flight_state && timer2_init_flag){
            timer2_init_flag = 0;
            TIMER2_INIT();
        }
    }
    return 0;
}
