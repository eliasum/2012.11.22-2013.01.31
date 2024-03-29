/*==========================================================================================================================*/
/*                                   ���� ������������ ������� �������������� ���������� ���1.                              */
/*                                            ����������� ��������� ��� ATmega8535.                                         */
/*==========================================================================================================================*/
#include <compat/ina90.h>   // _WDR();
//#include <avr/iom8535.h>
#include <avr/interrupt.h>  //_SEI(); _CLI();

#include "main.h"
/*==========================================================================================================================*/
/*                                                  ������� ������� ���������                                               */
/*==========================================================================================================================*/
int main(void)
{
  unsigned char j;

  DDRA  = 0;           //ALL inputs
  PORTA = 0;

  DDRB  = 0xe3;        //SAVE, TOGGLE � MODE - �����
  PORTB = 0xff;

  DDRC  = 0xff;
  PORTC = 0xff;

  DDRD  = 0xbf;        //PD6 - Input
  PORTD = 0xff;        //transmit=off

  _WDR();

  InitAd();
  ADCSRA |= (1<<ADIE); //enable AD interrupt
  sei();               //enable global interrupt

  SPCR = 0;            //disable SPI
  SPSR = 0;

  TIMSK = TIMSK|0x20; //���������� ���������� �� ������� ������� �������/�������� T1
  TCCR1B = 0xc1;      //ICNC1 = 1 ����� ���������� ����� �������� � ������ �������������� ������ � ������ 4-� ����������
  // �������, ��������������� ��������� ������ �������
  //ICES1 = 1 c��������� �������� �������� � �������� ������� �������������� �� ������������ ������ �������
  //CS10  = 1 ��� ����������� �������� ������� ����������� ��� ������������ �1, 0.125���
  TIMSK = TIMSK|0x4;  //���������� ���������� �� ������������ �������/�������� T1

  Dt4_8 = Dt4_30 = Dt4_37 = Dt5_37 = Dt5_95 = Dt5_118 = Dt6_115 = Dd1_1 = Dd1_4 = Nd_500 = Nd_1450 = Nd_1750 = 0;
  for(j=0; j<7; j++) { array[j] = 0; from_ee[j] = 0;}

  NDiz=0;
  CtTDiz=15;
  InvCounter=0;
  RegWait=30000;       //������ �������� - 300 //�������!!
  while (RegWait--)

    _WDR();

  mask_leds_c = 0x00;                    //PORT C.X � ������� ������ ������ ���������� �� "�������"
  mask_leds_d = 0x00;                    //PORT D.X

  Key_new=0xff;
  Key_Inp = KEY_EMPTY;                   //������ �� ������

  if(PINB & (1<<MODE))    mode = 1;      //����� ������ ������
  else                    mode = 0;

  if(!mode)                              //������ � ������ ����������
  {
    //������������� ���������� ������� ����������:
    timer_250 = RELOAD;                  //������ ������ ����� ��������� ������������� 250 ��
    count_channel = 0;                   //���� ���������� � 0-�� ������
    const_write_ok = 0;                  //������ �������� �� ����
    mask_leds_c = 0x00;                  //PORT C.X ��� ����� � ����� ���������������� "�������" 0-� �����
    mask_leds_d = 0x20;                  //PORT D.5

    while (1)
    {
      _WDR();

      //AccountADC();

      Dt4=444; Dt5=555; Dt6=666; Key_Inp = KEY_BS;


      //�������� ������������ ��������� ��� ������ � EEPROM
      if(Key_Inp == KEY_BT)                                                        //���� ������ ������ "����������� �����"
      {
        switch_channel(count_channel);                                             //������������ ��������� � ������ ������������� �������� ������� � ���

        if(count_channel < 7)
          count_channel++;                                                         //������������ �� ��������� �����

        Key_Inp=KEY_EMPTY;                                                         //���������� ���� ������ �� ������
      }

      //�������� ������ � �������� ������ ������������� �������� � EEPROM
      if(Key_Inp == KEY_BS)                                                     //���� ������ ������ "��������� ���������"
      {
        switch_channel(count_channel);                                          //������������ ��������� � ������ ������������� �������� ������� � ���

        if(count_channel < 7)
        {
          EEPROM_Write_Word(EEP_ADDR+ 2*count_channel, array[count_channel]);   //�������� ������������� ���������, ���� �������� ������
          from_ee[count_channel] = EEPROM_Read_Word(EEP_ADDR+ 2*count_channel); //������� ���������� �������� ��� ��������

          //�������� �������� ������
          if((from_ee[count_channel] == array[count_channel])&&(from_ee[count_channel] != 0xFFFF)&&(from_ee[count_channel] != 0x0))
            count_channel++;                                                    //������������ �� ��������� �����
        }
        Key_Inp=KEY_EMPTY;                                                      //���������� ���� ������ �� ������
      }
    }
  }
  else     //������� ����� ������
  {
    unsigned char i;

    //���������� ������� ������� �� EEPROM � ���
    for(i=0; i<6; i++)
    {
      array[i] = EEPROM_Read_Word(EEP_ADDR+ 2*i);
    }
    /*Work program*/
    while (1)
    {
      _WDR();
      AccountADC();
      AccountNDiz();
      /*
            ������� ������ �������� �����������. ������ ��� ������������ �������� ��������.

            ������� ������� ������ ��� �������������� ������� 4 �� ������ 8.
            ���� �������� ������� ����������� ����� ������ ��� ����� 8'�, �������� ���������� � ���� ������� 4 �� ������ 8 ��� �������
            (��������, ��� �������� ������� ����������� ���� �� ����� ������ 8'�), �� ���������� ���� ������� 4 �� ������ 8 (��������
            ���������� ������ 8).
            ���� �������� ������� ����������� ����� ������ 8'� � ���� ������� 4 �� ������ 8 ��� ����������, �� ���� ��������.
      */
      if((Dt4<=array[0]-Hyst_t)&&(!Dt4_8))       Dt4_8=1;   //���� 9.5 �������� ����������� ������������ - �����
      if((Dt4>array[0])&&(Dt4_8))                Dt4_8=0;   //���� 8 ��������

      if((Dt4<=(array[1]-Hyst_t))&&(!Dt4_30))    Dt4_30=1;  //���� 31.25 ��������
      if((Dt4>array[1])&&(Dt4_30))               Dt4_30=0;  //���� 30 �������� - ���������� ����������� ������������ - �����

      if((Dt4<=array[2])&&(!Dt4_37))             Dt4_37=1;  //���� 37.0 ��������
      if((Dt4>(array[2]+Hyst_t))&&(Dt4_37))      Dt4_37=0;  //���� 36,8 �������� ����������� ������������ - ����

      if((Dt5<=array[3]-(Hyst_t+20))&&(!Dt5_37)) Dt5_37=1;  //���� 44.5 ��������
      if((Dt5>array[3])&&(Dt5_37))               Dt5_37=0;  //���� 37 �������� ����������� ������������ - �����

      if((Dt5<=array[4])&&(!Dt5_95))             Dt5_95=1;  //���� 95 ��������
      if((Dt5>(array[4]+Hyst_t))&&(Dt5_95))      Dt5_95=0;  //���� 93.5 �������� ����������� ������������ - �����

      if((Dt6<=array[5])&&(!Dt6_115))            Dt6_115=1; //���� 115 ��������
      if((Dt6>(array[5]+Hyst_t))&&(Dt6_115))     Dt6_115=0; //���� 113.5 �������� ����������� ������������ - �����

      if((Dt5<=array[6])&&(!Dt5_118))            Dt5_118=1; //���� 118 ��������
      if((Dt5>(array[6]+Hyst_t))&&(Dt5_118))     Dt5_118=0; //���� 116.5 �������� ����������� ������������ - �����
      /*
            ������� ������ ��� ������� ��������. ������ ��� ������������ �������� ������.

            ������� ������� ������ ��� ������� �������� �� ������ 1.
            ���� �������� ������� �������� ����� ������ ��� ����� 1 ��/��^2 � ���� ������� �������� �� ������ 1 ��� ������� (��������,
            ��� �������� ������� �������� ���� �� ����� ������ 1 ��/��^2), �� ���������� ���� ������� �������� �� ������ 1 (��������
            ���������� ������ 1).
            ���� �������� ������� �������� ����� ������ 1 ��/��^2 � ���� ������� �������� �� ������ 1 ��� ����������, �� ���� ��������.
      */
      if((Dd1>=d1_1)&&(!Dd1_1))                  Dd1_1=1;   //���� 1 ��/�� �� ���������� ������� ����������� ������������ - �����
      if((Dd1<d1_1-Hyst_p)&&(Dd1_1))             Dd1_1=0;   //���� 1 ��/�� ��

      if((Dd1>=d1_4+Hyst_p)&&(!Dd1_4))           Dd1_4=1;   //���� 4 ��/�� �� ���������� �������
      if((Dd1<d1_4)&&(Dd1_4))                    Dd1_4=0;   //���� 4 ��/�� �� ���������� ������� 10 - ���������� ���� ����������� ������������ - ����
      /*
            ������� ������ ��� ������� ������� �������� �������� ��������� ������.

            ������� ������� ������ �� ������ 500.
            ���� ����� �������� �������� ��������� ������ ������ ��� ����� 500 � ���� ������� ������� �������� ��� ������� (��������,
            ��� ����� �������� ���� ����� 500), �� ���������� ���� ������� ������� �������� (�������� ���������� ������ 500).
            ���� ����� �������� ������ 400 � ���� ������� ������� �������� ��� ����������, �� �������� ���� ������� ������� ��������.
      */
      if((NDiz>=500)&&(!Nd_500))                 Nd_500=1;  //100 �������� ����������
      if((NDiz<400)&&(Nd_500))                   Nd_500=0;

      if((NDiz>=1450)&&(!Nd_1450))               Nd_1450=1;
      if((NDiz<1350)&&(Nd_1450))                 Nd_1450=0;

      if((NDiz>=1750)&&(!Nd_1750))               Nd_1750=1;
      if((NDiz<1650)&&(Nd_1750))                 Nd_1750=0;

//    ���������� ������ � ������� �� ������ (������������ ���������):
      if(!Dt4_8)   PORTD &= ~(1<<PORTD5); else PORTD |= (1<<PORTD5); //PD5 - ���� 8 ����
      if(Dt4_30)   PORTD &= ~(1<<PORTD3); else PORTD |= (1<<PORTD3); //PD3 - ���� 30 ����
      if(!Dt4_37)  PORTC &= ~(1<<PORTC2); else PORTC |= (1<<PORTC2); //P�2 - ���� 37 ���� 37+�����

      if(Dt5_37)   PORTC &= ~(1<<PORTC4); else PORTC |= (1<<PORTC4); //PC4 - ���� 45 ���� �������
      if(!Dt5_95)  PORTC &= ~(1<<PORTC0); else PORTC |= (1<<PORTC0); //PC0 - ���� 95 ����
      if(Dt6_115)  PORTC &= ~(1<<PORTC3); else PORTC |= (1<<PORTC3); //P�3 - ���� 115 ���� � 115 ��. ���������
      if(Dt5_118)  PORTC &= ~(1<<PORTC1); else PORTC |= (1<<PORTC1); //PC1 - ���� 118 ���� ������ ������ 118��.

      if(!Dd1_1)   PORTD &= ~(1<<PORTD0); else PORTD |= (1<<PORTD0); //PD0 - ���� 1 ��
      if(Dd1_4)    PORTD &= ~(1<<PORTD1); else PORTD |= (1<<PORTD1); //PD1 - ���� 4 ��

      if(!Nd_500)  PORTD &= ~(1<<PORTD2); else PORTD |= (1<<PORTD2); //PD2 - ���� 500
      if(!Nd_1450) PORTD &= ~(1<<PORTD4); else PORTD |= (1<<PORTD4); //PD4 - ���� 1450
      if(Nd_1750)  PORTC &= ~(1<<PORTC5); else PORTC |= (1<<PORTC5); //PC5 - ���� 1750
    }
  }
}

void InitAd(void)
{
  ADMUX = (1<<REFS0);                        //internal Aref with capacitor 0100 0000 (ADC0, single-ended, gain=1)
  CtAd = CtAd0;                              //63
  NumberAd = 0;
  ADCSRA = 0;
  ADCSRA |=(1<<ADEN);                        //enable AD
  ADCSRA |=(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //1/128 - 125 kHz ???
  ADCSRA |=(1<<ADSC);                        //Start
}

void AccountNDiz(void)          //������������ ���������� ������� �������� �������� ��������� ������
{
  unsigned long R1;             //���������� ���������� ��������
  unsigned char R0;             //����� �������� ������� ������� ���������� ��������
  unsigned char NDiz_invalid=0; //���� ������������ ������
  {
    R1=0;
    for(R0=0; R0<=15; ++R0)
    {
      if((TDizTemp[R0]==0xffff)||(TDizTemp[R0]==0)) NDiz_invalid=1; //����������� ����� ���� ������ ���-�� �������� ����� ��������
      R1+=TDizTemp[R0]; //������������ ���� 16 �������
    }
    R1>>=4; //���������� �������� ��������
    if((R1>62330)||(NDiz_Overflow==1)) // ���� ��������� ������ 62330 ��� ���������� ���� ������������, �� ������� = 0
      NDiz=0;

    else
    {
      if(!NDiz_invalid)
      {
        R1=8421000/R1;  //R1 = 62330 - 140 �������� � ������ ��� 2,33(3) � ���
        NDiz=R1;
        InvCounter=0;   //��������� �������� ������������
      }                 //R1 = 2909 - 3000�������� � ������ ��� 50 � �������
      if(NDiz_invalid)
      {
        InvCounter++;
        if(InvCounter>=10)
        {
          if(InvCounter>=250) InvCounter = 4; //����������
          NDiz = 0;
        }
      }
    }
  }
}
//�2� ����� 110 ������ �� ��������. � ��� 3000 �� ��� �������
//��������� 5500��. ��� ������� 1 ����� ������ ����������� �� 2909
//��� ��-45 ������� ��������� �� 1500��������=2850
//������ ����������� �� 5614
//����������� �������=

void AccountADC(void)
{
  Dd1=AdResult[0];
  Dt4=AdResult[1];
  Dt5=AdResult[2];
  Dt6=AdResult[3];
}

unsigned int switch_channel(unsigned int channel)
{

  //�������� ������������ ��������� � ������ ������������ �������� �������� � ���
  switch (count_channel)           //�������� ��������� � ������ ������������ �������� �������� � ����������� �� ������
  {
  case 0:
    mask_leds_c = 0x00;           //PORT C.X ����� ���������� ������
    mask_leds_d = 0x08;           //PORT D.3  0x08
    array[count_channel] = Dt4;
    break;
  case 1:
    mask_leds_c = 0x04;           //PORT C.2  0x04
    mask_leds_d = 0x00;           //PORT D.X
    array[count_channel] = Dt4;
    break;
  case 2:
    mask_leds_c = 0x10;           //PORT C.4  0x10
    mask_leds_d = 0x00;           //PORT D.X
    array[count_channel] = Dt4;
    break;
  case 3:
    mask_leds_c = 0x01;           //PORT C.0  0x01
    mask_leds_d = 0x00;           //PORT D.X
    array[count_channel] = Dt5;
    break;
  case 4:
    mask_leds_c = 0x08;           //PORT C.3  0x08
    mask_leds_d = 0x00;           //PORT D.X
    array[count_channel] = Dt5;
    break;
  case 5:
    mask_leds_c = 0x02;           //PORT C.1  0x02
    mask_leds_d = 0x00;           //PORT D.X
    array[count_channel] = Dt6;
    break;
  case 6:
    mask_leds_c = 0x00;           //PORT C.X � PORT D.X- ��������� ������ �� ������ ����������
    mask_leds_d = 0x00;
    array[count_channel] = Dt5;
    break;
  }
  return(array[count_channel]);
}

SIGNAL(SIG_INPUT_CAPTURE1)
{
  ICR1_value=ICR1; //������ �������� ��� ����� ������

  if((ICR1_value>ICR1Old)&&(CtOverLow==16))
  {
    TDizTemp[CtTDiz]=ICR1_value-ICR1Old; //������� ����� ����� �����������
    if(TDizTemp[CtTDiz]<100)
      TDizTemp[CtTDiz]=0xffff;           //FFFF - ����������� ��������

    if(CtTDiz) --CtTDiz;
    else        CtTDiz=15;               //16 �������
  }
  ICR1Old=ICR1_value;
  CtOverLow=16; //16 - ������������ �� ��������� ����� 0 ��������.
  NDiz_Overflow=0;
}

SIGNAL(SIG_OVERFLOW1) //8.192 Mc = 65536*0.125us - ����� ������������ �������
{
  if(CtOverLow) CtOverLow--;   //16 ������������ �� ��������� ����� ������������ NDiz_Overflow)
  else           NDiz_Overflow=1;

  if(timer_250 == 0)           //���� ������ 250 �� ��� ��������� ���������
  {
    timer_250 = RELOAD;        //������ ����� ���� � �������� 250 ��
    PORTC ^= mask_leds_c;      //������������� ������, ����������� ����������
    PORTD ^= mask_leds_d;
  }
  else
  {
    timer_250--;
    PORTC &= 0xe0;             //�������� ��� ���� ����������� ���������� ��������
    PORTD &= 0xd7;
  }
// ----
  //������������ ������
  if(Key_Inp == KEY_EMPTY)
  {
    if((INKEY_PORT & KEY_MASK) != KEY_MASK)
      Key_new = INKEY_PORT & KEY_MASK;
    if(Key_new != 0xff)
    {
      if(Key_new == Key_old)
      {
        if(Count_on > KSTATE) Ff_key = 1;
        else Count_on++;
      }
      else
      {
        Key_old = Key_new;
        Count_on = Count_off = 0;
        Ff_key = 0;
      }
      Key_new = 0xff;
    }
    else
    {
      if(Ff_key == 1)
      {
        if(Count_off > KSTATE)
        {
          Ff_key = 0;
          Key_Inp = Key_old;
        }
        else Count_off++;
      }
      else Key_old = 0x55;
    }
  }
}

SIGNAL(SIG_ADC)
{
  unsigned int RegInt0;
  RegInt0=ADC;

  if(CtAd)
  {
    --CtAd; //64 ���������
    if(CtAd<=CtAd0)
    {
      AdTemp +=RegInt0;
    }
  }
  else
  {
    AdResult[NumberAd]=AdTemp;
    AdResult[NumberAd]>>=6;
    AdTemp=0;
    CtAd=CtAd0+2;
    //if(NumberAd==11)
    NumberAd=NumberAdNew[NumberAd];
  }

  RegInt0=ADMUX;
  RegInt0 &=0xe0;     //��������� MUX4..0
  RegInt0 |=NumberAd; //����������� ������ ������ ������ ���
  ADMUX=RegInt0;

  ADCSRA |=(1<<ADSC); //start conversion
}

unsigned int EEPROM_Read_Word(unsigned int uiAddress)
{
  unsigned int ival;
  ival  = EEPROM_read(uiAddress);
  ival += EEPROM_read(uiAddress+1);
  return ival;
}
void EEPROM_Write_Word(unsigned int uiAddress, unsigned int ucWord)
{
  EEPROM_write(ucWord,    (unsigned char)(ucWord&0x00FF));
  EEPROM_write(ucWord+1 , (unsigned char)(ucWord/256));
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
  unsigned char sreg;
  // Save Global In
  sreg = SREG;
  // Disable interrupt
  _CLI();
  // Wait for completion of previous write
  while (EECR & (1<<EEWE));
  // Set up Address Register
  EEAR = uiAddress;
  // Start eeprom read by writing EERE
  EECR |= (1<<EERE);
  // Restory interrupt
  SREG = sreg;
  // Return data from Data Register
  return EEDR;
}

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
  unsigned char sreg;
  // Save Global In
  sreg = SREG;
  // Disable interrupt
  _CLI();
  // Wait for completion of previous write
  while (EECR & (1<<EEWE));
  // Set up Address and Data Registers
  EEAR = uiAddress;
  EEDR = ucData;
  // Write logical one to EEMWE
  EECR |= (1<<EEMWE);
  // Start eeprom write by setting EEWE
  EECR |= (1<<EEWE);
  // Restory interrupt
  SREG = sreg;
}
