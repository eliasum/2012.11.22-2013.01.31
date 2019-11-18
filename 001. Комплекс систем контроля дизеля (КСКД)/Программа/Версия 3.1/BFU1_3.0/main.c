/*==========================================================================================================================*/
/*                                   Блок формирования уставок контролируемых параметров БФУ1.                              */
/*                                            Управляющая программа для ATmega8535.                                         */
/*==========================================================================================================================*/
#include <compat/ina90.h>
#include <avr/iom8535.h>
#include <avr/interrupt.h>  //_SEI(); _CLI();
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "main.h"
/*==========================================================================================================================*/
/*                                                  Главная функция программы                                               */
/*==========================================================================================================================*/
int main(void)
{
  DDRA  = 0;           //ALL inputs
  PORTA = 0;

  DDRB  = 0xe3;        //SAVE, TOGGLE и MODE - входы
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

  TIMSK = TIMSK|0x20;  //разрешение прерывания по событию «Захват» таймера/счетчика T1
  TCCR1B = 0xc1;       //ICNC1 = 1 схема подавления помех включена и захват осуществляется только в случае 4-х одинаковых выборок, соответствующих активному фронту сигнала
                       //ICES1 = 1 cохранение счетного регистра в регистре захвата осуществляется по нарастающему фронту сигнала
                       //CS10  = 1 нет предделения тактовой частоты контроллера для тактирования Т1, 0.0625мкс
  TIMSK = TIMSK|0x4;   //разрешение прерывания по переполнению таймера/счетчика T1

  Dt4_8 = Dt4_30 = Dt4_37 = Dt5_37 = Dt5_95 = Dt5_118 = Dt6_115 = Dd1_1 = Dd1_4 = Nd_500 = Nd_1450 = Nd_1750 = 0;
  array[9] = 0;
  from_ee[9] = 0;

  NDiz=0;
  CtTDiz=15;
  InvCounter=0;
  RegWait=30000;       //старое значение - 300 //Вернуть!!
  while (RegWait--)

  _WDR();

  mask_leds_c = 0x00;                    //PORT C.X
  mask_leds_d = 0x00;                    //PORT D.X
  
  if (PINB & (1<<MODE))   mode = 1;      //выбор режима работы
  else                    mode = 0;

  if(!mode)                              //работа в режиме калибровки
  {
    //инициализация переменных функции калибровки:
    Key_Inp = KEY_EMPTY;                 //кнопка не нажата
    timer_250 = RELOAD;                  //начало нового цикла индикации длительностью 250 мс
    count_channel = 0;                   //счёт начинается с 0-го канала
    const_write_ok = 0;                  //записи констант не было
    mask_leds_c = 0x00;                  //PORT C.X
    mask_leds_d = 0x20;                  //PORT D.5

    while (1)
    {
     _WDR();

	 AccountADC();

	 //Dt4=444; Dt5=555; Dt6=666; Key_Inp = KEY_BS;

     //алгоритм записи и проверки записи калибровочных констант в EEPROM
     if(Key_Inp == KEY_BS)                                                     //если нажата кнопка "сохранить константу"            
     {
	   eeprom_busy_wait();
       eeprom_update_word(ADDRESS_TO_EE, array[count_channel]);                //записать калибровочную константу, если значение другое
	   eeprom_busy_wait();
       from_ee[count_channel] = eeprom_read_word(ADDRESS_TO_EE);               //считать записанное значение для проверки

       //проверка успешной записи
	   if(from_ee[count_channel] == array[count_channel]) const_write_ok = 1;  //поднять флаг об успешной записи константы
       else                                               const_write_ok = 0;

       Key_Inp=KEY_EMPTY;                                          //установить флаг кнопка не нажата
     }

     //алгоритм переключения индикации и записи калибровочных констант датчиков в ОЗУ
     if((Key_Inp == KEY_BT) || const_write_ok)  //если нажата кнопка "переключения канала" или была записана константа
     {
       if(count_channel > 6) count_channel = 0; //если было переключение с 8-го канала, то переключить на 0-й канал
       switch (count_channel)                   //варианты индикации и записи калибровочных констант в зависимости от канала
       {
         case 0:                                    
           mask_leds_c = 0x00; //PORT C.X
           mask_leds_d = 0x20; //PORT D.5
		   array[count_channel] = Dt4;
           break;
         case 1:
		   mask_leds_c = 0x00; //PORT C.X
           mask_leds_d = 0x08; //PORT D.3
		   array[count_channel] = Dt4;
           break;
         case 2:
           mask_leds_c = 0x04; //PORT C.2
           mask_leds_d = 0x00; //PORT D.X
		   array[count_channel] = Dt4;
           break;
         case 3:
           mask_leds_c = 0x10; //PORT C.4
           mask_leds_d = 0x00; //PORT D.X
		   array[count_channel] = Dt5;
           break;
         case 4:
           mask_leds_c = 0x01; //PORT C.0
           mask_leds_d = 0x00; //PORT D.X
		   array[count_channel] = Dt5;
           break;
         case 5:
           mask_leds_c = 0x08; //PORT C.3
           mask_leds_d = 0x00; //PORT D.X
		   array[count_channel] = Dt6;
           break;
         case 6:
           mask_leds_c = 0x02; //PORT C.1
           mask_leds_d = 0x00; //PORT D.X
		   array[count_channel] = Dt5;
           break;
        }
        count_channel++;       //переключение на следующий канал
		const_write_ok = 0;    //сброс состояния записи
        Key_Inp=KEY_EMPTY;     //кнопка не нажата
      }
    }
  }
 else                                  //обычный режим работы     
  {
    unsigned char i;
/*    
	//алгоритм проверки наличия констант в EEPROM и записи констант в ОЗУ
    for (i=0; i<9; i++)                 
    {
      eeprom_busy_wait();
      array[i] = eeprom_read_word(ADDRESS_FROM_EE);        //считывание массива уставок из EEPROM в ОЗУ
      if((array[i] == 0xff)||(array[i] == 0x00))           //если ячейка пуста
	    {
		  eeprom_write_word(ADDRESS_FROM_EE, ustavki[i]);  //записать уставку в пустую ячейку EEPROM
          eeprom_busy_wait();
	      array[i] = eeprom_read_word(ADDRESS_FROM_EE);    //записать уставку из изменённой ячейки EEPROM в ОЗУ
        }
    }
*/
    for (i=0; i<9; i++) array[i] = ustavki[i];

    /*Work program*/
    while (1)
    {
      _WDR();
      AccountADC();
      AccountNDiz();
/*
      Задание флагов датчиков температуры. Логика для оцифрованных значений обратная.

      Условия задания флагов для температурного датчика 4 по порогу 8.
      Если значение датчика температуры стало больше или равно 8'С, учитывая гистерезис и флаг датчика 4 по порогу 8 был сброшен
      (означает, что значение датчика температуры было до этого меньше 8'С), то установить флаг датчика 4 по порогу 8 (означает
      превышение порога 8).
      Если значение датчика температуры стало меньше 8'С и флаг датчика 4 по порогу 8 был установлен, то флаг сбросить.
*/
      if ((Dt4<=array[0]-Hyst_t)&&(!Dt4_8))       Dt4_8=1;   //выше 9.5 градусов направление срабатывания - вверх
      if ((Dt4>array[0])&&(Dt4_8))                Dt4_8=0;   //ниже 8 градусов

      if ((Dt4<=(array[1]-Hyst_t))&&(!Dt4_30))    Dt4_30=1;  //выше 31.25 градусов
      if ((Dt4>array[1])&&(Dt4_30))               Dt4_30=0;  //ниже 30 градусов - гистерезис направление срабатывания - вверх

      if ((Dt4<=array[2])&&(!Dt4_37))             Dt4_37=1;  //выше 37.0 градусов
      if ((Dt4>(array[2]+Hyst_t))&&(Dt4_37))      Dt4_37=0;  //ниже 36,8 градусов направление срабатывания - вниз

      if ((Dt5<=array[3]-(Hyst_t+20))&&(!Dt5_37)) Dt5_37=1;  //выше 44.5 градусов
      if ((Dt5>array[3])&&(Dt5_37))               Dt5_37=0;  //ниже 37 градусов направление срабатывания - вверх

      if ((Dt5<=array[4])&&(!Dt5_95))             Dt5_95=1;  //выше 95 градусов
      if ((Dt5>(array[4]+Hyst_t))&&(Dt5_95))      Dt5_95=0;  //ниже 93.5 градусов направление срабатывания - вверх

      if ((Dt6<=array[5])&&(!Dt6_115))            Dt6_115=1; //выше 115 градусов
      if ((Dt6>(array[5]+Hyst_t))&&(Dt6_115))     Dt6_115=0; //ниже 113.5 градусов направление срабатывания - вверх

      if ((Dt5<=array[6])&&(!Dt5_118))            Dt5_118=1; //выше 118 градусов
      if ((Dt5>(array[6]+Hyst_t))&&(Dt5_118))     Dt5_118=0; //ниже 116.5 градусов направление срабатывания - вверх
/* 
      Задание флагов для датчика давления. Логика для оцифрованных значений прямая.

      Условия задания флагов для датчика давления по порогу 1.
      Если значение датчика давления стало больше или равно 1 кг/см^2 и флаг датчика давления по порогу 1 был сброшен (означает,
      что значение датчика давления было до этого меньше 1 кг/см^2), то установить флаг датчика давления по порогу 1 (означает 
      превышение порога 1).
      Если значение датчика давления стало меньше 1 кг/см^2 и флаг датчика давления по порогу 1 был установлен, то флаг сбросить.
*/
      if ((Dd1>=d1_1)&&(!Dd1_1))                  Dd1_1=1;   //выше 1 кг/см кв определить пределы направление срабатывания - вверх
      if ((Dd1<d1_1-Hyst_p)&&(Dd1_1))             Dd1_1=0;   //ниже 1 кг/см кв

      if ((Dd1>=d1_4+Hyst_p)&&(!Dd1_4))           Dd1_4=1;   //выше 4 кг/см кв определить пределы
      if ((Dd1<d1_4)&&(Dd1_4))                    Dd1_4=0;   //ниже 4 кг/см кв определить пределы 10 - гистерезис вниз направление срабатывания - вниз
/*
      Задание флагов для датчика частоты оборотов маховика коленвала дизеля. 

      Условия задания флагов по порогу 500.
      Если число оборотов маховика коленвала дизеля больше или равно 500 и флаг датчика частоты оборотов был сброшен (означает,
      что число оборотов было менше 500), то установить флаг датчика частоты оборотов (означает превышение порога 500).
      Если число оборотов меньше 400 и флаг датчика частоты оборотов был установлен, то сбросить флаг датчика частоты оборотов.
*/
      if ((NDiz>=500)&&(!Nd_500))                 Nd_500=1;  //100 оборотов гистерезис
      if ((NDiz<400)&&(Nd_500))                   Nd_500=0;

      if ((NDiz>=1450)&&(!Nd_1450))               Nd_1450=1;
      if ((NDiz<1350)&&(Nd_1450))                 Nd_1450=0;

      if ((NDiz>=1750)&&(!Nd_1750))               Nd_1750=1;
      if ((NDiz<1650)&&(Nd_1750))                 Nd_1750=0; 

//    Объявление портов и задание их логики (светодиодная индикация):
      if (!Dt4_8)   PORTD &= ~(1<<PORTD5); else PORTD |= (1<<PORTD5); //PD5 - выше 8 град
      if (Dt4_30)   PORTD &= ~(1<<PORTD3); else PORTD |= (1<<PORTD3); //PD3 - ниже 30 град
      if (!Dt4_37)  PORTC &= ~(1<<PORTC2); else PORTC |= (1<<PORTC2); //PС2 - ниже 37 град 37+красн

      if (Dt5_37)   PORTC &= ~(1<<PORTC4); else PORTC |= (1<<PORTC4); //PC4 - выше 45 град зеленый
      if (!Dt5_95)  PORTC &= ~(1<<PORTC0); else PORTC |= (1<<PORTC0); //PC0 - выше 95 град
	  if (Dt6_115)  PORTC &= ~(1<<PORTC3); else PORTC |= (1<<PORTC3); //PС3 - выше 115 град и 115 гр. инверсная
      if (Dt5_118)  PORTC &= ~(1<<PORTC1); else PORTC |= (1<<PORTC1); //PC1 - выше 118 град логика работы 118гр.

      if (!Dd1_1)   PORTD &= ~(1<<PORTD0); else PORTD |= (1<<PORTD0); //PD0 - выше 1 кг
      if (Dd1_4)    PORTD &= ~(1<<PORTD1); else PORTD |= (1<<PORTD1); //PD1 - ниже 4 кг
   
      if (!Nd_500)  PORTD &= ~(1<<PORTD2); else PORTD |= (1<<PORTD2); //PD2 - выше 500
      if (!Nd_1450) PORTD &= ~(1<<PORTD4); else PORTD |= (1<<PORTD4); //PD4 - выше 1450
      if (Nd_1750)  PORTC &= ~(1<<PORTC5); else PORTC |= (1<<PORTC5); //PC5 - выше 1750
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
  ADCSRA |=(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //1/128 - 125 kHz
  ADCSRA |=(1<<ADSC);                        //Start
}

void AccountNDiz(void)          //подпрограмма вычисления частоты вращения маховика коленвала дизеля
{
  unsigned long R1;             //переменная количества оборотов
  unsigned char R0;             //номер элемента массива выборок количества оборотов
  unsigned char NDiz_invalid=0; //флаг неправильной работы
  {
    R1=0;
    for (R0=0; R0<=15; ++R0)
    {
      if ((TDizTemp[R0]==0xffff)||(TDizTemp[R0]==0)) NDiz_invalid=1; //выставление флага если расчет кол-ва оборотов будет неверный
      R1+=TDizTemp[R0]; //суммирование всех 16 выборок
    }
    R1>>=4; //вычисление среднего значения
    if((R1>62330)||(NDiz_Overflow==1)) // если результат больше 62330 или установлен флаг переполнения, то обороты = 0
      NDiz=0;

    else
    {
      if (!NDiz_invalid)
      {
        R1=8421000/R1;  //R1 = 62330 - 140 оборотов в минуту или 2,33(3) в сек
        NDiz=R1;
        InvCounter=0;   //обнуление счетчика инвалидности
      }                 //R1 = 2909 - 3000оборотов в минуту или 50 в секунду
      if (NDiz_invalid)
      {
        InvCounter++;
        if (InvCounter>=10)
        {
          if (InvCounter>=250) InvCounter = 4; //колцевание
          NDiz = 0;
        }
      }
    }
  }
}
//В2Ч имеет 110 зубьев на маховике. и при 3000 об мин частота
//импульсов 5500Гц. при времени 1 такта таймер досчитывает до 2909
//для УД-45 частота импульсов на 1500оборотов=2850
//таймер досчитывает до 5614
//коэффициент деления=

void AccountADC(void)
{
  Dd1=AdResult[0];
  Dt4=AdResult[1];
  Dt5=AdResult[2];
  Dt6=AdResult[3];
}

SIGNAL(SIG_INPUT_CAPTURE1)
{
  ICR1_value=ICR1; //чтение значения как можно раньше

  if ((ICR1_value>ICR1Old)&&(CtOverLow==16))
  {
    TDizTemp[CtTDiz]=ICR1_value-ICR1Old; //разница между двумя измерениями
    if (TDizTemp[CtTDiz]<100)
      TDizTemp[CtTDiz]=0xffff; //FFFF - невозможное значение

    if (CtTDiz) --CtTDiz;
    else        CtTDiz=15;     //16 выборок
  }
  ICR1Old=ICR1_value;
  CtOverLow=16; //16 - переполнений до установки флага 0 оборотов.
  NDiz_Overflow=0;
}

SIGNAL(SIG_OVERFLOW1) //4.096 Mc = 65536*0.0625us - время переполнения таймера
{
  if (CtOverLow) CtOverLow--;  //16 переполнений до установки флага переполнения NDiz_Overflow)
  else           NDiz_Overflow=1;

  if(timer_250 == 0)           //если прошло 250 мс без изменения индикации 
  {
    timer_250 = RELOAD;        //начать новый цикл с периодом 250 мс
    PORTC ^= mask_leds_c;      //инвертировать сигнал, управляющий индикацией
    PORTD ^= mask_leds_d;
  }
  else timer_250--;

  //сканирование кнопок
  if(Key_Inp == KEY_EMPTY)
  {
    if((INKEY_PORT & KEY_MASK) != KEY_MASK)
        Key_new = INKEY_PORT & KEY_MASK;
    if (Key_new != 0xff)
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

  if (CtAd)
  {
    --CtAd; //64 измерения
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
  RegInt0 &=0xe0;     //обнуление MUX4..0
  RegInt0 |=NumberAd; //выставление нового адреса канала ацп
  ADMUX=RegInt0;

  ADCSRA |=(1<<ADSC); //start conversion
}
