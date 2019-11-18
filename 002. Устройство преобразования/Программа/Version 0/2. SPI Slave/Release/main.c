/*======================================================================================================================================*/
/*                                              ���������� �������������� ����.656111.059.                                              */
/*                                           ����������� ��������� ��� ATmega8535 SPI Slave.                                            */
/*======================================================================================================================================*/
#include <compat/ina90.h>   // _WDR();
#include <avr/iom8535.h>        
#include <avr/interrupt.h>  //_SEI(); _CLI();

#include "main.h"

/*======================================================================================================================================*/
/*                                                       ������� ������� ���������.                                                     */
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

  _SEI();                                   //���������� ����������
 
  SPI_SlaveInit();                          //������������� SPI � ������ Slave
  
  for(;;)
  {
    byte_from_modbus1 = SPI_SlaveReceive(); //���� ������ �� ��������, ���������� �� ���� ModBus 
    byte_from_modbus2 = SPI_SlaveReceive();
	byte_from_modbus3 = SPI_SlaveReceive();
	byte_from_modbus4 = SPI_SlaveReceive();

    PORTA |= byte_from_modbus1;             //4 ����� �� 27 ������� ��� �������� �� ���� ModBus
    PORTB |= byte_from_modbus2;
	PORTC |= byte_from_modbus3;
	PORTD |= byte_from_modbus4;
  } 
  return(0);
}

// ������������ ������������� SPI � ������ Slave
void SPI_SlaveInit(void)
{
  // ���������� SPI � ������ ��������
  SPCR = (1<<SPE);
}

// ������������ ������ ����� SPI
unsigned char SPI_SlaveReceive(void)
{
  // �������� ���������� �������� (���� ��� SPIF �� ����������)
  while(!(SPSR & (1<<SPIF)));
  // ������ �������� ������ � ����� �� ���������
  return SPDR;
}
