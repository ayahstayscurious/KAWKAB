/*
 * KAWKAB — Planet Encyclopedia Embedded Card Reader
 * ATmega8 Firmware
 *
 * ECCE4227: Embedded Systems, Spring 2026
 * Sultan Qaboos University — College of Engineering
 *
 * Authors: Ayah Alshanfari, Shahad Al Rubkhi, Hawa Bahashwan
 * Instructor: Dr. Muhammad Rizwan Mughal
 *
 * MCU   : ATmega8 @ 8 MHz (internal RC oscillator)
 * Inputs : ADC0 (PC0) — resistor-coded planet card
 *          ADC1 (PC1) — LDR card insertion detector
 * Outputs: LCD 16x2 (RS=PC2, EN=PC3, D4-D7=PD4-PD7)
 *          RGB LED   (R=PB0, G=PB1, B=PB2)
 *          Buzzer    (PB3, Timer2 CTC/OC2)
 *          UART TX   (PD1, 9600 baud 8N1)
 */

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

/* ── Thresholds ─────────────────────────────────────────────────────────── */
#define LDR_BLOCKED 300        /* ADC1 threshold: card present  */
#define LDR_OPEN    700        /* ADC1 threshold: card absent   */

/* ── UART ────────────────────────────────────────────────────────────────── */
#define BAUD     9600
#define UBRR_VAL (F_CPU / 16 / BAUD - 1)   /* = 51 for 8 MHz */

void uart_init(void) {
    UBRRH = (uint8_t)(UBRR_VAL >> 8);
    UBRRL = (uint8_t)(UBRR_VAL);
    UCSRB = (1 << TXEN) | (1 << RXEN);
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
}

void uart_putchar(char c) {
    while (!(UCSRA & (1 << UDRE)));
    UDR = c;
}

void uart_print(const char *s)   { while (*s) uart_putchar(*s++); }

void uart_println(const char *s) {
    uart_print(s);
    uart_putchar('\r');
    uart_putchar('\n');
}

void uart_send(const char *id, uint16_t adc0, uint16_t adc1) {
    char buf[6];
    uart_println(id);
    uart_print("PLANET="); uart_print(id);
    uart_print(" ADC0="); itoa(adc0, buf, 10); uart_print(buf);
    uart_print(" ADC1="); itoa(adc1, buf, 10); uart_print(buf);
    uart_println(" OK");
}

/* ── ADC ─────────────────────────────────────────────────────────────────── */
void adc_init(void) {
    ADMUX  = (1 << REFS0);                               /* AVCC reference    */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); /* prescaler 64      */
}

uint16_t adc_read(uint8_t ch) {
    ADMUX = (ADMUX & 0xF0) | (ch & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

/* Average n samples with 3 ms inter-sample delay for noise rejection */
uint16_t adc_avg(uint8_t ch, uint8_t n) {
    uint32_t s = 0;
    uint8_t  i;
    for (i = 0; i < n; i++) { s += adc_read(ch); _delay_ms(3); }
    return (uint16_t)(s / n);
}

/* ── LCD  (RS=PC2, EN=PC3, data nibble=PD4-PD7) ─────────────────────────── */
#define LCD_RS PC2
#define LCD_EN PC3

void lcd_pulse(void) {
    PORTC |=  (1 << LCD_EN); _delay_us(1);
    PORTC &= ~(1 << LCD_EN); _delay_us(50);
}

void lcd_nib(uint8_t n) { PORTD = (PORTD & 0x0F) | (n << 4); lcd_pulse(); }

void lcd_cmd(uint8_t c) {
    PORTC &= ~(1 << LCD_RS);
    lcd_nib(c >> 4);
    lcd_nib(c & 0x0F);
    _delay_ms(2);
}

void lcd_char(char c) {
    PORTC |= (1 << LCD_RS);
    lcd_nib(c >> 4);
    lcd_nib(c & 0x0F);
    _delay_us(50);
}

void lcd_str(const char *s) { while (*s) lcd_char(*s++); }

void lcd_clear(void) { lcd_cmd(0x01); _delay_ms(2); }

void lcd_goto(uint8_t r, uint8_t c) { lcd_cmd((r ? 0xC0 : 0x80) + c); }

void lcd_init(void) {
    DDRC |= (1 << LCD_RS) | (1 << LCD_EN);
    DDRD |= 0xF0;
    _delay_ms(50);
    PORTC &= ~(1 << LCD_RS);
    lcd_nib(0x03); _delay_ms(5);
    lcd_nib(0x03); _delay_ms(1);
    lcd_nib(0x03); _delay_ms(1);
    lcd_nib(0x02); _delay_ms(1);
    lcd_cmd(0x28);   /* 4-bit, 2 lines, 5×8 font */
    lcd_cmd(0x0C);   /* display on, cursor off    */
    lcd_cmd(0x06);   /* entry mode: increment      */
    lcd_clear();
}

/* ── Buzzer — Timer2 CTC mode, OC2=PB3 ──────────────────────────────────── */
void tone(uint16_t freq, uint16_t ms) {
    if (!freq) {
        TCCR2 = 0;
        PORTB &= ~(1 << PB3);
        _delay_ms(ms);
        return;
    }
    OCR2  = (uint8_t)(F_CPU / (2UL * 64UL * freq) - 1);
    TCCR2 = (1 << WGM21) | (1 << COM20) | (1 << CS22);
    _delay_ms(ms);
    TCCR2 = 0;
    PORTB &= ~(1 << PB3);
}

/* ── RGB LED ─────────────────────────────────────────────────────────────── */
#define LED_R PB0
#define LED_G PB1
#define LED_B PB2
#define LEDS  ((1 << LED_R) | (1 << LED_G) | (1 << LED_B))

void leds_set(uint8_t m)  { PORTB &= ~LEDS; PORTB |= (m & LEDS); }

void leds_flash(uint8_t m, uint8_t n, uint16_t ms) {
    uint8_t i;
    for (i = 0; i < n; i++) {
        leds_set(m);  _delay_ms(ms);
        leds_set(0);  _delay_ms(ms);
    }
}

/* ── Planet data ─────────────────────────────────────────────────────────── */
typedef enum {
    P_NONE = 0,
    P_MERCURY, P_VENUS, P_EARTH,   P_MARS,
    P_JUPITER, P_SATURN, P_URANUS, P_NEPTUNE
} PlanetID;

typedef struct {
    const char *uart_id, *lcd1, *lcd2;
    uint8_t     leds;
    uint16_t    freq[5], dur[5];
} Planet;

const Planet PLANETS[] = {
    /* P_NONE */
    {"", "", "", 0, {0}, {0}},

    /* P_MERCURY — white, rising fanfare */
    {"MERCURY", " MERCURY ", " CLOSEST SUN ",
     (1<<LED_R)|(1<<LED_G)|(1<<LED_B),
     {523, 659, 784, 1047, 0}, {80, 80, 80, 200, 0}},

    /* P_VENUS — yellow, descending */
    {"VENUS", " VENUS ", " HOTTEST PLANET ",
     (1<<LED_R)|(1<<LED_G),
     {784, 659, 523, 494, 0}, {120, 120, 120, 300, 0}},

    /* P_EARTH — cyan, hopeful */
    {"EARTH", " EARTH ", " OUR HOME :) ",
     (1<<LED_G)|(1<<LED_B),
     {523, 659, 784, 1047, 0}, {100, 100, 100, 300, 0}},

    /* P_MARS — red, weighty march */
    {"MARS", " MARS ", " JEZERO CRATER ",
     (1<<LED_R),
     {392, 392, 392, 311, 0}, {150, 150, 150, 400, 0}},

    /* P_JUPITER — yellow-orange, bold ascent */
    {"JUPITER", " JUPITER ", " GREAT RED SPOT ",
     (1<<LED_R)|(1<<LED_G),
     {262, 392, 523, 784, 0}, {100, 100, 100, 300, 0}},

    /* P_SATURN — magenta, falling */
    {"SATURN", " SATURN ", " RINGED GIANT ",
     (1<<LED_R)|(1<<LED_B),
     {440, 392, 349, 330, 0}, {150, 150, 150, 350, 0}},

    /* P_URANUS — cyan, curious rise */
    {"URANUS", " URANUS ", " TILTED 97.8 DEG",
     (1<<LED_G)|(1<<LED_B),
     {330, 349, 392, 440, 0}, {100, 100, 100, 250, 0}},

    /* P_NEPTUNE — blue, slow descent */
    {"NEPTUNE", " NEPTUNE ", "WINDS 2100 KM/H ",
     (1<<LED_B),
     {262, 247, 220, 196, 0}, {180, 180, 180, 400, 0}}
};

void play_tones(const Planet *p) {
    uint8_t i;
    for (i = 0; i < 5 && p->freq[i]; i++) {
        tone(p->freq[i], p->dur[i]);
        _delay_ms(25);
    }
}

/* ── Card identification ─────────────────────────────────────────────────── */
PlanetID identify(uint16_t adc) {
    if (adc <  138) return P_MERCURY;
    if (adc <  256) return P_VENUS;
    if (adc <  420) return P_EARTH;
    if (adc <  608) return P_MARS;
    if (adc <  765) return P_JUPITER;
    if (adc <  877) return P_SATURN;
    if (adc <  953) return P_URANUS;
    if (adc <  995) return P_NEPTUNE;
    return P_NONE;
}

/* ── Display helpers ─────────────────────────────────────────────────────── */
void show_planet(PlanetID id, uint16_t adc0, uint16_t adc1) {
    const Planet *p = &PLANETS[id];
    leds_set(p->leds);
    lcd_clear();
    lcd_goto(0, 0); lcd_str(p->lcd1);
    lcd_goto(1, 0); lcd_str(p->lcd2);
    uart_send(p->uart_id, adc0, adc1);
    play_tones(p);
    leds_set(p->leds);   /* keep LED on after tone completes */
}

void show_idle(void) {
    lcd_clear();
    lcd_goto(0, 0); lcd_str(" INSERT CARD  ");
    lcd_goto(1, 0); lcd_str(" KAWKAB v1.0  ");
    leds_set(0);
    uart_println("CARD_REMOVED");
    leds_set(1 << LED_B); _delay_ms(400); leds_set(0);
}

void show_unknown(void) {
    lcd_clear();
    lcd_goto(0, 0); lcd_str(" UNKNOWN CARD ");
    lcd_goto(1, 0); lcd_str(" CHECK CONTACTS ");
    leds_flash((1 << LED_R), 6, 80);
    leds_set(1 << LED_R);
    uart_println("CARD_UNKNOWN");
}

/* ── Main ────────────────────────────────────────────────────────────────── */
int main(void) {
    typedef enum { WAITING, CARD_IN } State;
    State   state  = WAITING;
    uint8_t stable = 0;
    const uint8_t NEED = 5;   /* consecutive readings required for state change */

    /* Port directions */
    DDRB |= LEDS | (1 << PB3);          /* LED + buzzer outputs */
    DDRC &= ~((1 << PC0) | (1 << PC1)); /* ADC pins as inputs   */

    uart_init();
    adc_init();
    lcd_init();

    /* Splash screen */
    lcd_goto(0, 0); lcd_str(" KAWKAB v1.0  ");
    lcd_goto(1, 0); lcd_str(" SQU ECCE4227 ");
    uart_println("=== KAWKAB PLANET READER ===");

    /* Power-on LED sweep */
    leds_set(1 << LED_R); _delay_ms(150);
    leds_set(1 << LED_G); _delay_ms(150);
    leds_set(1 << LED_B); _delay_ms(150);
    leds_set(LEDS);       _delay_ms(150);
    leds_set(0);

    /* Startup chime */
    tone(523, 60);  _delay_ms(30);
    tone(659, 60);  _delay_ms(30);
    tone(784, 60);  _delay_ms(30);
    tone(1047, 180);
    _delay_ms(600);

    show_idle();

    /* ── Main loop ── */
    while (1) {
        uint16_t ldr  = adc_avg(1, 4);
        uint16_t card = adc_avg(0, 4);
        PlanetID id;

        if (state == WAITING) {
            if (ldr < LDR_BLOCKED) {
                if (++stable >= NEED) {
                    stable = 0;
                    state  = CARD_IN;
                    id = identify(card);
                    if (id != P_NONE) show_planet(id, card, ldr);
                    else              show_unknown();
                }
            } else {
                stable = 0;
            }
        } else {   /* CARD_IN */
            if (ldr > LDR_OPEN) {
                if (++stable >= NEED) {
                    stable = 0;
                    state  = WAITING;
                    show_idle();
                }
            } else {
                stable = 0;
            }
        }

        _delay_ms(10);
    }

    return 0;
}
