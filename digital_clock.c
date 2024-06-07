#include <mega16a.h>
#include <stdio.h>
#include <alcd.h>
#include <delay.h>

#define C1 PINC.4
#define C2 PINC.5
#define C3 PINC.6
#define C4 PINC.7
#define NCOL 4
#define NROW 4

#define ADC_VREF_TYPE ((0<<REFS1) | (0<<REFS0) | (0<<ADLAR))

int hour = 0, minute = 0, second = 0;

int is_seting_clock = 0;
int key_pressed = 0;
int count_30sec = 0;
int temperature_update = 1;
int temperature_1=0, temperature_2=0;   // temperature_1 -> dahgan  temperature_2 -> yekan

char str[10]="00:00:00";

//flash char Display[10] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f}; // for common cathod
flash char Display[10] = {0x03,0xF3,0x25,0x0D,0x99,0x49,0x41,0x1F,0x01,0x09};

flash char layout[NROW][NCOL] =
{
    '7', '8', '9', 'A',
    '4', '5', '6', 'B',
    '1', '2', '3', 'C',
    '*', '0', '#', 'D',
};
flash char shifter[NROW] = {0xFE, 0xFD, 0xFB, 0xF7};

unsigned int read_adc(unsigned char adc_input)
{
ADMUX=adc_input | ADC_VREF_TYPE;
// Delay needed for the stabilization of the ADC input voltage
delay_us(10);
// Start the AD conversion
ADCSRA|=(1<<ADSC);
// Wait for the AD conversion to complete
while ((ADCSRA & (1<<ADIF))==0);
ADCSRA|=(1<<ADIF);
return ADCW;
}

void show_clock(void)
{
    sprintf(str,"%02d:%02d:%02d", hour, minute, second);
    lcd_gotoxy(0,0);
    lcd_puts(str);
}

char read_keypad(void)
{
 int row, col=-1;
 
 for (row=0; row<NROW; row++)
 {
    PORTC = shifter[row];
    if (C1 == 0) col=0;
    else if(C2 == 0) col=1;
    else if(C3 == 0) col=2;
    else if(C4 == 0) col=3;
    if (col != -1)
    {
        PORTC = 0xF0;
        key_pressed = 0;
        return layout[row][col];
    }
 }
 return '\0';
}

void update_clock(void)
{
    char ch;
    int cnt;
    is_seting_clock = 1;
    for(cnt = 0; cnt<8 ; cnt++)
    {
        str[cnt] = '0';
    }
    for(cnt = 0;cnt < 8;cnt++)
    {
    if(cnt == 2 || cnt == 5) continue;

    lcd_gotoxy(cnt,1);
    lcd_putchar('^');
    while (!key_pressed)
    {
        lcd_gotoxy(cnt,0);
        lcd_putchar(' ');
        delay_ms(10);
        lcd_gotoxy(cnt,0);
        lcd_putchar('0');
        delay_ms(10);
    };
    ch = read_keypad();
    if(ch == '*')
    {
        lcd_gotoxy(cnt,1);
        lcd_putchar(' ');
        is_seting_clock = 0;
        return ;
    }else if(ch == '#')
    {
        lcd_gotoxy(cnt,1);
        lcd_putchar(' ');
        break;
    }
    
    lcd_gotoxy(cnt,0);
    lcd_putchar(ch);
    lcd_gotoxy(cnt,1);
    lcd_putchar(' ');
    str[cnt] = ch;
     
    }
    
    hour = (str[0] - 48)*10 + (str[1] - 48);
    minute = (str[3] - 48)*10 + (str[4] - 48);
    second = (str[6] - 48)*10 + (str[7] - 48);
    
    // time validate
    if(hour > 23) hour = 0;
    if(minute > 59) minute = 0;
    if(second > 59) second = 0;
     
    is_seting_clock = 0;
}



interrupt [EXT_INT0] void ext_int0_isr(void)
{
    key_pressed = 1;
}

// Timer1 overflow interrupt service routine
interrupt [TIM1_OVF] void timer1_ovf_isr(void)
{
// Reinitialize Timer1 value
TCNT1H=0xC2F7 >> 8;
TCNT1L=0xC2F7 & 0xff;

    count_30sec++;
    if(count_30sec >=30)
    {
        count_30sec = 0;
        temperature_update = 1;
    }
    second++;
    if(second >= 60)
    {
        minute++;
        second = 0;
        if(minute >= 60)
        {
            hour++;
            minute = 0;
            if(hour >= 24)
            {
                hour = 0;
            }
        }
    }
    
    if(!is_seting_clock)
        show_clock();    

}

void main(void)
{
char ch;
int temp;
int shifter[2] = {0b00010000,0b00100000};
int num[2];
int i;

DDRC = 0x0F;
PORTC = 0xF0;

DDRD = 0x30;
//PORTD = 0xF0;

DDRB = 0xFF;


TCCR1A=(0<<COM1A1) | (0<<COM1A0) | (0<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (0<<WGM10);
TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (0<<CS12) | (1<<CS11) | (1<<CS10);
TCNT1H=0xC2;
TCNT1L=0xF7;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=(0<<OCIE2) | (0<<TOIE2) | (0<<TICIE1) | (0<<OCIE1A) | (0<<OCIE1B) | (1<<TOIE1) | (0<<OCIE0) | (0<<TOIE0);

GICR|=(0<<INT1) | (1<<INT0) | (0<<INT2);
MCUCR=(0<<ISC11) | (0<<ISC10) | (1<<ISC01) | (0<<ISC00);
MCUCSR=(0<<ISC2);
GIFR=(0<<INTF1) | (1<<INTF0) | (0<<INTF2);


ADMUX=ADC_VREF_TYPE;
ADCSRA=(1<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (0<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
SFIOR=(0<<ADTS2) | (0<<ADTS1) | (0<<ADTS0);



lcd_init(16);

// Global enable interrupts
#asm("sei")


while (1)
{
    if (key_pressed)
    {   
        ch = read_keypad();
        if (ch == '*') 
            update_clock();    
    };
    
    if (temperature_update)
    {
        temp = read_adc(3);
        temp = (int)(temp * 0.488 + 0.5);
        temperature_2 = temp % 10;
        temp = temp / 10;
        temperature_1 = temp;
        
        temperature_update = 0;
    }
    
    num[0] = temperature_1;
    num[1] = temperature_2; 
    for(i = 0; i < 2; i++)
    {
        PORTD = shifter[i];
        PORTB = Display[num[i]];
        delay_ms(20);
    }
    /*PORTD.4 = 1;
    PORTD.5 = 0;
    PORTB = Display[temperature_1];
    delay_ms(20);
    PORTD.4 = 0;
    PORTD.5 = 1;
    PORTB = Display[temperature_2];
    delay_ms(20);*/
         
}
}
