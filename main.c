#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "OneWire.h"

#define F_CPU 8000000UL

#include <util/delay.h>

#define ONE_WIRE_PORT      PORTB
#define ONE_WIRE_DDR       DDRB
#define ONE_WIRE_PIN       PINB

uint8_t last_state_e EEMEM;
uint8_t last_state = 0xFF;
uint8_t kill_covid_temp = 50;

double getTemp(uint64_t ds18b20s) {
	uint8_t temperatureL;
	uint8_t temperatureH;
	double retd = 0;
	
	
	setDevice(ds18b20s);
	writeByte(CMD_CONVERTTEMP);
	
	_delay_ms(750);
	
	setDevice(ds18b20s);
	writeByte(CMD_RSCRATCHPAD);
	
	temperatureL = readByte();
	temperatureH = readByte();
	
	retd = ((temperatureH << 8) + temperatureL) * 0.0625;
	
	return retd;
}

void pwm_init(void)
{
	TCCR0A |= 0xA3;  //PWM mode
	TCCR0B |= 0x02; // FREQ CLK/8
	TCNT0=0;
	OCR0A=0;
	OCR0B=255;//For CLEAR counter.Read Attiny 13 datasheet at page 65-68.
}

int main(void)
{
    DDRB|=(1<<DDB0)|(1<<DDB1)|(0<<DDB2);
	pwm_init();
	last_state = eeprom_read_byte(&last_state_e);
	if(last_state  != 0 && last_state != 1)
	{
		last_state = 0;
		eeprom_write_byte(&last_state_e, last_state);
	}
	asm("nop");//Init delay, read Attiny 13 datasheet at page 45.
	oneWireInit(PINB3);
  
  double temperature = 0;
  uint8_t n = 2;
  uint64_t roms[n];
  searchRom(roms, n);
	while (1) 
    {
		if(!(PINB & (1<<PB2)))
		{
			switch(last_state)
			{
				case 0:
					last_state = 1;
					eeprom_write_byte(&last_state_e, last_state);
					OCR0A = 0;
					OCR0B = 255;
					while(temperature < kill_covid_temp)
					{
						temperature = getTemp(roms[0]);
						//_delay_ms(1);////Test delay, ONLY FOR PROTEUS DEBUG!!! Usually delay 750ms
					}
					OCR0A = 255;
				break;
				case 1:
					last_state = 0;
					eeprom_write_byte(&last_state_e, last_state);
					OCR0B = 0;
					OCR0A = 255;
					while(temperature < kill_covid_temp)
					{
						temperature = getTemp(roms[1]);
						//_delay_ms(1);////Test delay, ONLY FOR PROTEUS DEBUG!!! Usually delay 750ms
					}
					OCR0B = 255;		
				break;
			}
		}
		_delay_ms(5);//Test delay, ONLY FOR PROTEUS DEBUG!!! Usually delay 200-300ms
	}
}

