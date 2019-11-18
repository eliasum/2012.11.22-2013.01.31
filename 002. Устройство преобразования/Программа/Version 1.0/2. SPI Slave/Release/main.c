/*======================================================================================================================================*/
/*                                              Устройство преобразования ДИАФ.656111.059.                                              */
/*                                           Управляющая программа для ATmega8535 SPI Slave.                                            */
/*======================================================================================================================================*/
#include <compat/ina90.h>   // _WDR();
#include <avr/iom8535.h>        
#include <avr/interrupt.h>  //_SEI(); _CLI();

#include "main.h"

/*======================================================================================================================================*/
/*                                                       Главная функция программы.                                                     */
/*======================================================================================================================================*/
int main( void )
{
  DDRA  = 0xff;
  PORTA = 0x0;

  DDRB  = 0x4f;
  PORTB = (1<<SCK)|(1<<MOSI)|(1<<MISO);

  DDRC  = 0xff;
  PORTC = 0x0;

  DDRD  = 0x7f;
  PORTD = 0x0;

  _WDR();

  _SEI();                                   //разрешение прерываний
 
  SPI_SlaveInit();                          //инициализация SPI в режиме Slave
  
  for(;;)
  {
    byte_from_modbus1 = SPI_SlaveReceive(); //приём данных от ведущего, полученных по шине ModBus 
    byte_from_modbus2 = SPI_SlaveReceive();
	byte_from_modbus3 = SPI_SlaveReceive();
	byte_from_modbus4 = SPI_SlaveReceive();

    PORTA |= byte_from_modbus1;             //4 байта на 27 выходов для передачи по шине ModBus
    PORTB |= byte_from_modbus2;
	PORTC |= byte_from_modbus3;
	PORTD |= byte_from_modbus4;
  } 
  return(0);
}

// Подпрограмма инициализации SPI в режиме Slave
void SPI_SlaveInit(void)
{
  // Разрешение SPI в режиме ведомого
  SPCR = (1<<SPE);
}

// Подпрограмма чтения через SPI
unsigned char SPI_SlaveReceive(void)
{
  // Ожидание завершения передачи (пока бит SPIF не установлен)
  while(!(SPSR & (1<<SPIF)));
  // Чтение принятых данных и выход из процедуры
  return SPDR;
}
