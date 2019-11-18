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
	volatile	unsigned    char	CtIntT1;
    unsigned    long            TDiz;
    unsigned    int             TDizTemp[16];
	const unsigned    char		NumberAdNew[4]={1,2,3,0};
	unsigned    char	NumberAd;
    unsigned    char	CtAd;
	const unsigned    char	    CtAd0=63;
	unsigned    int	AdTemp;
 	unsigned    int	AdResult[12];
    unsigned    char            CtTDiz;
    unsigned    char            CtOverLow;
    unsigned    int             NDiz, ICR1_value;
	unsigned int Dd1;
	unsigned int Dt4;
	unsigned int Dt5;
	unsigned int Dt6;
 	unsigned    int         	ICR1Old;
	unsigned    int   			RegWait;
	unsigned    char	RegS,RegSTemp,RegSOld, InvCounter, NDiz_Overflow=0;
	unsigned char Dt4_8, Dt4_30, Dt4_37, Dt5_37, Dt5_95, Dt5_118;
	unsigned char Dt6_115, Dd1_1, Dd1_4, Nd_500, Nd_1450, Nd_1750;  
	const char Hyst_t=10;
	const char Hyst_p=10;

	/*
	const unsigned int t4_8=705; 
	const unsigned int t4_30=617;
	const unsigned int t4_37=591;
	const unsigned int t5_37=595;
	const unsigned int t5_95=394; //Рассчетные значения
	const unsigned int t6_115=341;
	const unsigned int t5_118=327;
	const unsigned int d1_1=114;
	const unsigned int d1_4=213;
	*/
	
	const unsigned int t4_8=705; 
	const unsigned int t4_30=617;
	const unsigned int t4_37=591;
	const unsigned int t5_37=595;
	const unsigned int t5_95=403; //БФУ ТП23612105-2
	const unsigned int t6_115=341;
	const unsigned int t5_118=336;
	const unsigned int d1_1=114;
	const unsigned int d1_4=213;
	
       
    /*
	const unsigned int t4_8=703; 
	const unsigned int t4_30=619;
	const unsigned int t4_37=592;
	const unsigned int t5_37=590;
	const unsigned int t5_95=388; //БФУ №2 фактические 
	const unsigned int t6_115=341;
	const unsigned int t5_118=322;
	const unsigned int d1_1=114;
	const unsigned int d1_4=213;
    */
    /*============================================*/
void InitAd   (void)
    {
    ADMUX=0x40;//internal Aref with capacitor 0100 0 000 (ADC0, single-ended, gain=1)
 	CtAd=CtAd0; //63
	NumberAd=0;
	ADCSRA=0;
	ADCSRA |=(1<<ADEN);/*enable AD*/
	ADCSRA |=(1<<ADPS2);
    ADCSRA |=(1<<ADPS1);
	ADCSRA |=(1<<ADPS0); //1/128* - 31250 Hz/	 		
    ADCSRA |=(1<<ADSC);/* Start*/
    }



void AccountNDiz(void)
    {

    unsigned long R1;
    unsigned char R0;
	unsigned char NDiz_invalid=0;

		{
	R1=0;
	for(R0=0;R0<=15;++R0)
			{
	if ((TDizTemp[R0]==0xffff)||(TDizTemp[R0]==0)) NDiz_invalid=1; //выставление флага если расчет кол-ва оборотов будет неверный
    R1+=TDizTemp[R0]; //суммирование всех 16 выборок
			}
	R1>>=4; //вычисление среднего значения
	if ((R1>62330)||(NDiz_Overflow==1)) // если результат больше 62330, или установлен флаг переполнения то обороты = 0
	NDiz=0;

	else
			{
	if (!NDiz_invalid)
	{
		R1=8421000/R1;  //R1 = 62330 - 140 оборотов в минуту или 2,33(3) в сек
	 	NDiz=R1; 
		InvCounter=0; //обнуление счетчика инвалидности
	}		//R1 = 2909 - 3000оборотов в минуту или 50 в секунду
	if (NDiz_invalid)
	{
		InvCounter++;
		if (InvCounter>=10) 
		{
		if (InvCounter>=250) InvCounter = 4; //колцевание
		NDiz = 0;
		}	//В2Ч имеет 110 зубьев на маховике. и при 3000 об мин частота
	}				//импульсов 5500Гц. при времени 1 такта таймер досчитывает до 2909
					//для УД-45 частота импульсов на 1500оборотов=2850
					//таймер досчитывает до 5614
					//коэффициент деления=

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
/*++++++++++++++++++++++++++++++++++++++++++*/


    int main(void)
    {

    DDRB=0xFF;
	PORTB |= 0xFF;
    DDRC=0xff;
    PORTC |=0xFF;

    DDRD=0xbf; //PD6 - Input
    PORTD=0xFF;//transmit=off

	DDRA=0; // ALL inputs
	PORTA=0; 
	_WDR();
	 
	InitAd(); 
	ADCSRA |=(1<<ADIE);//enable AD interrupt 
	sei(); //enable global interrupt

    SPCR=0x0;// disable SPI
    SPSR=0;  //
		

    TIMSK=TIMSK | 0x20;//enable Int capture1
    TCCR1B=0xc1;//0.0625mkc 
	// noise canceller enable input capture select=1 
	// prescaler source=001
    TIMSK=TIMSK | 0x4;//enable Int overlowT1

	TCCR2 =1; //2 timer Clock select=001
	TCNT2=5;//4000mks значение таймера 2
	TIFR |=0x40; //T2 overflow flag=1 output compare=0

Dt4_8=0; 
Dt4_30=0; Dt4_37=0; Dt5_37=0; Dt5_95=0; Dt5_118=0;
Dt6_115=0;
Dd1_1=0;
Dd1_4=0; Nd_500=0; Nd_1450=0; Nd_1750=0;  

	NDiz=0;
	CtTDiz=15;
	InvCounter=0;
    RegWait=30000; //старое значение - 300 //Вернуть!!
	while(RegWait--)
    _WDR();

						       
 /*Work program*/     	 
    while(1)
    {
    _WDR();
	AccountDd1();
	AccountDt4();
	AccountDt5();
	AccountDt6();
	AccountNDiz();


	if((NDiz>=500)&&(!Nd_500)) Nd_500=1; 
	if((NDiz<=400)&&(Nd_500)) Nd_500=0;

	if((NDiz>=1450)&&(!Nd_1450)) Nd_1450=1; 
	if((NDiz<=1350)&&(Nd_1450)) Nd_1450=0;

	if((NDiz>=1750)&&(!Nd_1750)) Nd_1750=1; 
	if((NDiz<=1650)&&(Nd_1750)) Nd_1750=0; //100 оборотов гистерезис

	
	if((Dd1>=d1_1)&&(!Dd1_1)) Dd1_1=1; //выше 1 кг/см кв определить пределы направление срабатывания - вверх
	if((Dd1<=d1_1-Hyst_p)&&(Dd1_1)) Dd1_1=0; // ниже 1 кг/см кв

	if((Dd1>=d1_4+Hyst_p)&&(!Dd1_4)) Dd1_4=1; //выше 4 кг/см кв определить пределы
	if((Dd1<=d1_4)&&(Dd1_4)) Dd1_4=0;  //ниже 4 кг/см кв определить пределы 10 - гистерезис вниз направление срабатывания - вниз
	
	if((Dt4<=t4_8-Hyst_t)&&(!Dt4_8)) Dt4_8=1; //выше 9.5 градусов направление срабатывания - вверх
	if((Dt4>=(t4_8))&&(Dt4_8)) Dt4_8=0; //ниже 8 градусов

	if((Dt4<=(t4_30-Hyst_t))&&(!Dt4_30)) Dt4_30=1; //выше 31.25 градусов
	if((Dt4>=(t4_30))&&(Dt4_30)) Dt4_30=0; //ниже 30 градусов - гистерезис направление срабатывания - вверх

	if((Dt4<=t4_37)&&(!Dt4_37)) Dt4_37=1; //выше 37.0 градусов
	if((Dt4>=(t4_37+Hyst_t))&&(Dt4_37)) Dt4_37=0; //ниже 36,8 градусов направление срабатывания - вниз

	if((Dt5<=t5_37-(Hyst_t+20))&&(!Dt5_37)) Dt5_37=1; //выше 44.5 градусов
	if((Dt5>=(t5_37))&&(Dt5_37)) Dt5_37=0; //ниже 37 градусов направление срабатывания - вверх

	if((Dt5<=t5_95)&&(!Dt5_95)) Dt5_95=1; //выше 95 градусов
	if((Dt5>=(t5_95+Hyst_t))&&(Dt5_95)) Dt5_95=0; //ниже 93.5 градусов направление срабатывания - вверх

	if((Dt5<=t5_118)&&(!Dt5_118)) Dt5_118=1; //выше 118 градусов
	if((Dt5>=(t5_118+Hyst_t))&&(Dt5_118)) Dt5_118=0; //ниже 116.5 градусов направление срабатывания - вверх


	if((Dt6<=t6_115)&&(!Dt6_115)) Dt6_115=1; //выше 115 градусов
	if((Dt6>=(t6_115+Hyst_t))&&(Dt6_115)) Dt6_115=0; //ниже 113.5 градусов направление срабатывания - вверх


	if(!Nd_500)  
	 PORTD &=  0xFB;
	 else   PORTD |= 0x04; //PD5-выше 500

	if(!Nd_1450) 
	 PORTD &=  0xEF;
	  else PORTD |= 0x10;  //PD4-выше 1450
	if(Nd_1750) 
	 PORTC &=  0xDF; 
	 else PORTC |= 0x20;  //PC5-выше 1750            


	if(!Dd1_1)   PORTD &=  0xFE; else PORTD |= 0x01;  //PD0 - выше 1 кг 
	if(Dd1_4)    PORTD &=  0xFD; else PORTD |= 0x02;  //PD1 - ниже 4 кг

	if(!Dt4_8)   PORTD &=  0xDF; else PORTD |= 0x20; //PD5 - выше 8 град
	if(Dt4_30)   PORTD &=  0xF7; else PORTD |= 0x08;  //PD3 - ниже 30 град
	if(!Dt4_37)   PORTC &=  0xFB; else PORTC |= 0x04;  //PС2 - ниже 37 град 37+красн

	if(Dt5_37)  PORTC &=  0xEF; else PORTC |= 0x10;  //PC4 - выше 45 град зеленый
	if(!Dt5_95)  PORTC &=  0xFE; else PORTC |= 0x01;  //PC0 - выше 95 град

	if(Dt5_118)  PORTC &=  0xFD; else PORTC |= 0x02;  //PC1 - выше 118 град логика работы 118гр. 
	if(Dt6_115)  PORTC &=  0xF7; else PORTC |= 0x08;  //PС3 - выше 115 град и 115 гр. инверсная
	}
}

 SIGNAL(SIG_INPUT_CAPTURE1)
{

	ICR1_value=ICR1; //чтение значения как можно раньше
		
	if ((ICR1_value>ICR1Old)&&(CtOverLow==16))
    	{
    TDizTemp[CtTDiz]=ICR1_value-ICR1Old; //разница между двумя измерениями
		if(TDizTemp[CtTDiz]<100)
    		TDizTemp[CtTDiz]=0xffff; //FFFF - невозможное значение
    	
		if(CtTDiz)
			--CtTDiz;
		else
	CtTDiz=15; //16 выборок
		}
	ICR1Old=ICR1_value;
	CtOverLow=16; //16 - переполнений до установки флага 0 оборотов.
	NDiz_Overflow=0;
		
}

 SIGNAL(SIG_OVERFLOW1)/*4.096 Mc = 65536*0.0625us*/ 
    {
	if (CtOverLow) //16 переполнений до установки флага переполнения NDiz_Overflow)
	 	CtOverLow--;
	else NDiz_Overflow=1;
    }

	 
SIGNAL(SIG_ADC)
{
    unsigned int RegInt0;
    RegInt0=ADC;
	
	if(CtAd) 
	{
	--CtAd; //64 измерения 
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
	RegInt0 &=0xe0; // обнуление MUX4..0 
	RegInt0 |=NumberAd; //выставление нового адреса канала ацп
	ADMUX=RegInt0;

    ADCSRA |=(1<<ADSC); //start conversion
  	
}
