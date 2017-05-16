#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

// Pull in the simavr preamble stuff to make it simulate-able.
#include "simavr/avr/avr_mcu_section.h"
AVR_MCU(F_CPU, "attiny85");

#define PEDAL_PORT DDB2
#define VAC_PORT DDB1
#define AIR_PORT DDB0
#define AIR_FILL_MS 150

#define ENABLE_PORT(port) PORTB |= (1 << port)
#define DISABLE_PORT(port) PORTB &= ~(1 << port)

volatile uint8_t apply_vacuum = 0;

ISR(INT0_vect)
{
    // If the vacuum was off, we now turn it on.  If it was on,
    // we turn it off and blip the pressure to clear the vacuum.
    apply_vacuum = apply_vacuum ? 0 : 1;

    if(apply_vacuum)
    {
        // All we want to do here is turn on the vacuum, and make
        // sure the air port is closed up.
        DISABLE_PORT(AIR_PORT);
        ENABLE_PORT(VAC_PORT);
    }
    else
    {
        // When turning off the vacuum, we want to blip the air port
        // to clear the vacuum as quick as possible.
        DISABLE_PORT(VAC_PORT);
        ENABLE_PORT(AIR_PORT);
        _delay_ms(AIR_FILL_MS);
        DISABLE_PORT(AIR_PORT);
    }
}

int main()
{
    // Set the pedal port as an input, and the vacuum/air ports as outputs.
    DDRB = 0 | (0 << PEDAL_PORT) | (1 << VAC_PORT) | (1 << AIR_PORT);

    // Make sure that the vacuum/air ports are off.
    DISABLE_PORT(AIR_PORT);
    DISABLE_PORT(VAC_PORT);

    // Enable the external interrupt, rising edge only, then enable interrupts globally.
    GIMSK |= (1 << INT0);
    MCUCR |= (1 << ISC00) | (1 << ISC01);
    sei();

    for (;;) sleep_mode();
}
