/*
 * KSKD.c
 *
 * Created: 10.01.2012 9:52:34
 *  Author: derishev.a
 */
/*ArgusBA*/
/* WDTON CKOPT
BOODLEVEL BODEN SUT1 CKSEL2*/


#include <compat/ina90.h>
#include <avr/iom8535.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <avr/pgmspace.h>


unsigned long TDiz;
unsigned int TDizTemp[16];
unsigned int AdTemp;
unsigned int AdResult[12];
unsigned int ICR1_value;
unsigned int NDiz;                                                      //����� �������� �������� ��������� ������
unsigned int Dd1;                                                       //������������ �������� ������� ��������
unsigned int Dt4, Dt5, Dt6;                                             //������������ �������� �������� �����������
unsigned int ICR1Old;
unsigned int RegWait;
unsigned char NumberAd;                                                 //����� �������� ������ ���
unsigned char CtAd;                                                     //����� �������� ��������� ���
unsigned char CtTDiz;
unsigned char CtOverLow;
unsigned char RegS,RegSTemp,RegSOld, InvCounter, NDiz_Overflow=0;
unsigned char Dt4_8, Dt4_30, Dt4_37, Dt5_37, Dt5_95, Dt5_118, Dt6_115;  //���� ������� ����������� Dtn �� ������, ��� n - ����� �������
unsigned char Dd1_1, Dd1_4;                                             //���� ������� �������� Ddn �� ������, ��� n - ����� �������
unsigned char Nd_500, Nd_1450, Nd_1750;                                 //���� ������� ������� �������� �������� ��������� ������ Nd �� ������
const char Hyst_t=10, Hyst_p=10;                                        //������������ �������� ����������� �������� ����������� � ���������                                            
const unsigned char NumberAdNew[4]={1,2,3,0};                           //������ ������� ������� ���
const unsigned char	CtAd0=63;                                           //���������� ��������� ��� 0...63
volatile unsigned char CtIntT1;

/*
const unsigned int t4_8=705;   //���������� ��������
const unsigned int t4_30=617;
const unsigned int t4_37=591;
const unsigned int t5_37=595;
const unsigned int t5_95=394; 
const unsigned int t6_115=341;
const unsigned int t5_118=327;
const unsigned int d1_1=114;
const unsigned int d1_4=213;
*/

const unsigned int t4_8=705;   //������������ ������� ��� ������� �������� ����������� � ��������
const unsigned int t4_30=617;
const unsigned int t4_37=591;
const unsigned int t5_37=595;
const unsigned int t5_95=403;  //��� ��23612105-2
const unsigned int t6_115=341;
const unsigned int t5_118=336;
const unsigned int d1_1=114;
const unsigned int d1_4=213;

/*
const unsigned int t4_8=703;   //��� �2 �����������
const unsigned int t4_30=619;
const unsigned int t4_37=592;
const unsigned int t5_37=590;
const unsigned int t5_95=388;  
const unsigned int t6_115=341;
const unsigned int t5_118=322;
const unsigned int d1_1=114;
const unsigned int d1_4=213;
*/
/*===========================================================================================================================*/
void InitAd   (void)
{
  ADMUX = (1<<REFS0);                        //internal Aref with capacitor 0100 0000 (ADC0, single-ended, gain=1)
  CtAd = CtAd0;                              //63
  NumberAd = 0;
  ADCSRA = 0; 
  ADCSRA |=(1<<ADEN);                        //enable AD
  ADCSRA |=(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //1/128 - 125 kHz
  ADCSRA |=(1<<ADSC);                        //Start
}

void AccountNDiz(void)          //������������ ���������� ������� �������� �������� ��������� ������
{

  unsigned long R1;             //���������� ���������� ��������
  unsigned char R0;             //����� �������� ������� ������� ���������� ��������
  unsigned char NDiz_invalid=0; //���� ������������ ������

  {
    R1=0;
    for (R0=0; R0<=15; ++R0)
    {
      if ((TDizTemp[R0]==0xffff)||(TDizTemp[R0]==0)) NDiz_invalid=1; //����������� ����� ���� ������ ���-�� �������� ����� ��������
      R1+=TDizTemp[R0]; //������������ ���� 16 �������
    }
    R1>>=4; //���������� �������� ��������
    if ((R1>62330)||(NDiz_Overflow==1)) // ���� ��������� ������ 62330 ��� ���������� ���� ������������, �� ������� = 0
      NDiz=0;

    else
    {
      if (!NDiz_invalid)
      {
        R1=8421000/R1;  //R1 = 62330 - 140 �������� � ������ ��� 2,33(3) � ���
        NDiz=R1;
        InvCounter=0; //��������� �������� ������������
      }		//R1 = 2909 - 3000�������� � ������ ��� 50 � �������
      if (NDiz_invalid)
      {
        InvCounter++;
        if (InvCounter>=10)
        {
          if (InvCounter>=250) InvCounter = 4; //����������
          NDiz = 0;
        }	//�2� ����� 110 ������ �� ��������. � ��� 3000 �� ��� �������
      }		//��������� 5500��. ��� ������� 1 ����� ������ ����������� �� 2909
            //��� ��-45 ������� ��������� �� 1500��������=2850
            //������ ����������� �� 5614
            //����������� �������=

    }
  }
}

void AccountDd1   (void)
{
  Dd1=AdResult[0];
}

void AccountDt4   (void)
{
  Dt4=AdResult[1];
}

void AccountDt5   (void)
{
  Dt5=AdResult[2];
}
void AccountDt6   (void)
{
  Dt6=AdResult[3];
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

int main(void)
{
  DDRA = 0;            //ALL inputs
  PORTA = 0;

  DDRB = 0xFF;
  PORTB |= 0xFF;

  DDRC = 0xFF;
  PORTC |= 0xFF;

  DDRD = 0xBF;         //PD6 - Input
  PORTD = 0xFF;        //transmit=off

  _WDR();

  InitAd();
  ADCSRA |= (1<<ADIE); //enable AD interrupt
  sei();               //enable global interrupt
 
  SPCR = 0;            // disable SPI
  SPSR = 0;            


  TIMSK = TIMSK|0x20;  //enable Int capture1
  TCCR1B = 0xc1;       //0.0625mkc
  // noise canceller enable input capture select=1
  // prescaler source=001
  TIMSK = TIMSK|0x4;   //enable Int overlowT1

  TCCR2 = 1;           //2 timer Clock select=001
  TCNT2 = 5;           //4000mks �������� ������� 2
  TIFR |= 0x40;        //T2 overflow flag=1 output compare=0

  Dt4_8=0, Dt4_30=0; Dt4_37=0; Dt5_37=0; Dt5_95=0; Dt5_118=0, Dt6_115=0;
  Dd1_1=0, Dd1_4=0; 
  Nd_500=0; Nd_1450=0; Nd_1750=0;

  NDiz=0;
  CtTDiz=15;
  InvCounter=0;
  RegWait=30000;       //������ �������� - 300 //�������!!
  while (RegWait--)
    _WDR();


  /*Work program*/
  while (1)
  {
    _WDR();
    AccountDd1();
    AccountDt4();
    AccountDt5();
    AccountDt6();
    AccountNDiz();
/*
    ������� ������ ��� ������� ������� �������� �������� ��������� ������. 

	������� ������� ������ �� ������ 500.
	���� ����� �������� �������� ��������� ������ ������ ��� ����� 500 � ���� ������� ������� �������� ��� ������� (��������,
	��� ����� �������� ���� ����� 500), �� ���������� ���� ������� ������� �������� (�������� ���������� ������ 500).
	���� ����� �������� ������ 400 � ���� ������� ������� �������� ��� ����������, �� �������� ���� ������� ������� ��������.
*/
    if ((NDiz>=500)&&(!Nd_500))              Nd_500=1;  //100 �������� ����������
    if ((NDiz<400)&&(Nd_500))                Nd_500=0;

    if ((NDiz>=1450)&&(!Nd_1450))            Nd_1450=1;
    if ((NDiz<1350)&&(Nd_1450))              Nd_1450=0;

    if ((NDiz>=1750)&&(!Nd_1750))            Nd_1750=1;
    if ((NDiz<1650)&&(Nd_1750))              Nd_1750=0; 
/* 
    ������� ������ ��� ������� ��������. ������ ��� ������������ �������� ������.

	������� ������� ������ ��� ������� �������� �� ������ 1.
	���� �������� ������� �������� ����� ������ ��� ����� 1 ��/��^2 � ���� ������� �������� �� ������ 1 ��� ������� (��������,
	��� �������� ������� �������� ���� �� ����� ������ 1 ��/��^2), �� ���������� ���� ������� �������� �� ������ 1 (�������� 
	���������� ������ 1).
	���� �������� ������� �������� ����� ������ 1 ��/��^2 � ���� ������� �������� �� ������ 1 ��� ����������, �� ���� ��������.
*/
    if ((Dd1>=d1_1)&&(!Dd1_1))               Dd1_1=1;   //���� 1 ��/�� �� ���������� ������� ����������� ������������ - �����
    if ((Dd1<d1_1-Hyst_p)&&(Dd1_1))          Dd1_1=0;   //���� 1 ��/�� ��

    if ((Dd1>=d1_4+Hyst_p)&&(!Dd1_4))        Dd1_4=1;   //���� 4 ��/�� �� ���������� �������
    if ((Dd1<d1_4)&&(Dd1_4))                 Dd1_4=0;   //���� 4 ��/�� �� ���������� ������� 10 - ���������� ���� ����������� ������������ - ����
/*
    ������� ������ �������� �����������. ������ ��� ������������ �������� ��������.

	������� ������� ������ ��� �������������� ������� 4 �� ������ 8.
	���� �������� ������� ����������� ����� ������ ��� ����� 8'�, �������� ���������� � ���� ������� 4 �� ������ 8 ��� �������
	(��������, ��� �������� ������� ����������� ���� �� ����� ������ 8'�), �� ���������� ���� ������� 4 �� ������ 8 (��������
	���������� ������ 8).
	���� �������� ������� ����������� ����� ������ 8'� � ���� ������� 4 �� ������ 8 ��� ����������, �� ���� ��������.
*/
    if ((Dt4<=t4_8-Hyst_t)&&(!Dt4_8))        Dt4_8=1;   //���� 9.5 �������� ����������� ������������ - �����
    if ((Dt4>(t4_8))&&(Dt4_8))               Dt4_8=0;   //���� 8 ��������

    if ((Dt4<=(t4_30-Hyst_t))&&(!Dt4_30))    Dt4_30=1;  //���� 31.25 ��������
    if ((Dt4>(t4_30))&&(Dt4_30))             Dt4_30=0;  //���� 30 �������� - ���������� ����������� ������������ - �����

    if ((Dt4<=t4_37)&&(!Dt4_37))             Dt4_37=1;  //���� 37.0 ��������
    if ((Dt4>(t4_37+Hyst_t))&&(Dt4_37))      Dt4_37=0;  //���� 36,8 �������� ����������� ������������ - ����

    if ((Dt5<=t5_37-(Hyst_t+20))&&(!Dt5_37)) Dt5_37=1;  //���� 44.5 ��������
    if ((Dt5>(t5_37))&&(Dt5_37))             Dt5_37=0;  //���� 37 �������� ����������� ������������ - �����

    if ((Dt5<=t5_95)&&(!Dt5_95))             Dt5_95=1;  //���� 95 ��������
    if ((Dt5>(t5_95+Hyst_t))&&(Dt5_95))      Dt5_95=0;  //���� 93.5 �������� ����������� ������������ - �����

    if ((Dt5<=t5_118)&&(!Dt5_118))           Dt5_118=1; //���� 118 ��������
    if ((Dt5>(t5_118+Hyst_t))&&(Dt5_118))    Dt5_118=0; //���� 116.5 �������� ����������� ������������ - �����

    if ((Dt6<=t6_115)&&(!Dt6_115))           Dt6_115=1; //���� 115 ��������
    if ((Dt6>(t6_115+Hyst_t))&&(Dt6_115))    Dt6_115=0; //���� 113.5 �������� ����������� ������������ - �����

//  ���������� ������ � ������� �� ������ (������������ ���������):
    if (!Nd_500)  PORTD &= ~(1<<PORTD2); else PORTD |= (1<<PORTD2); //PD2 - ���� 500
    if (!Nd_1450) PORTD &= ~(1<<PORTD4); else PORTD |= (1<<PORTD4); //PD4 - ���� 1450
    if (Nd_1750)  PORTC &= ~(1<<PORTC5); else PORTC |= (1<<PORTC5); //PC5 - ���� 1750

    if (!Dd1_1)   PORTD &= ~(1<<PORTD0); else PORTD |= (1<<PORTD0); //PD0 - ���� 1 ��
    if (Dd1_4)    PORTD &= ~(1<<PORTD1); else PORTD |= (1<<PORTD1); //PD1 - ���� 4 ��

    if (!Dt4_8)   PORTD &= ~(1<<PORTD5); else PORTD |= (1<<PORTD5); //PD5 - ���� 8 ����
    if (Dt4_30)   PORTD &= ~(1<<PORTD3); else PORTD |= (1<<PORTD3); //PD3 - ���� 30 ����
    if (!Dt4_37)  PORTC &= ~(1<<PORTC2); else PORTC |= (1<<PORTC2); //P�2 - ���� 37 ���� 37+�����

    if (Dt5_37)   PORTC &= ~(1<<PORTC4); else PORTC |= (1<<PORTC4); //PC4 - ���� 45 ���� �������
    if (!Dt5_95)  PORTC &= ~(1<<PORTC0); else PORTC |= (1<<PORTC0); //PC0 - ���� 95 ����

    if (Dt5_118)  PORTC &= ~(1<<PORTC1); else PORTC |= (1<<PORTC1); //PC1 - ���� 118 ���� ������ ������ 118��.
    if (Dt6_115)  PORTC &= ~(1<<PORTC3); else PORTC |= (1<<PORTC3); //P�3 - ���� 115 ���� � 115 ��. ���������
  }
}

SIGNAL(SIG_INPUT_CAPTURE1)
{

  ICR1_value=ICR1; //������ �������� ��� ����� ������

  if ((ICR1_value>ICR1Old)&&(CtOverLow==16))
  {
    TDizTemp[CtTDiz]=ICR1_value-ICR1Old; //������� ����� ����� �����������
    if (TDizTemp[CtTDiz]<100)
      TDizTemp[CtTDiz]=0xffff; //FFFF - ����������� ��������

    if (CtTDiz)
      --CtTDiz;
    else
      CtTDiz=15; //16 �������
  }
  ICR1Old=ICR1_value;
  CtOverLow=16; //16 - ������������ �� ��������� ����� 0 ��������.
  NDiz_Overflow=0;

}

SIGNAL(SIG_OVERFLOW1)/*4.096 Mc = 65536*0.0625us*/
{
  if (CtOverLow) //16 ������������ �� ��������� ����� ������������ NDiz_Overflow)
    CtOverLow--;
  else NDiz_Overflow=1;
}


SIGNAL(SIG_ADC)
{
  unsigned int RegInt0;
  RegInt0=ADC;

  if (CtAd)
  {
    --CtAd; //64 ���������
    if (CtAd<=CtAd0)
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
  RegInt0 &=0xe0; // ��������� MUX4..0
  RegInt0 |=NumberAd; //����������� ������ ������ ������ ���
  ADMUX=RegInt0;

  ADCSRA |=(1<<ADSC); //start conversion
}

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
  while(EECR & (1<<EEWE)); //����� ���������� ���������� ������
  EEAR = uiAddress;        //������������������� ��������
  EEDR = ucData;
  EECR |= (1<<EEMWE);      //���������� ���� EEMWE
  EECR |= (1<<EEWE);       //������ ������ � EEPROM
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
  while(EECR & (1<<EEWE)); //����� ���������� ���������� ������
  EEAR = uiAddress;        //������������������� ������� ������
  EECR |= (1<<EERE);       //��������� ������
  return EEDR;
}
