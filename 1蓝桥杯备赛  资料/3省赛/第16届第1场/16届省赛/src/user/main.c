#include <STC15F2K60S2.H>
#include "key.h"
#include "led.h"
#include "seg.h"
#include "init.h"
#include "onewire.h"
#include "init.h"
#include "ultrasound.h"
#include "iic.h"
#include "math.h"

idata unsigned char Key_Val, Key_Old, Key_Down, Key_Up;
idata unsigned char Key_Slow_Down;
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
idata unsigned char Seg_Pos = 0;
idata unsigned char Seg_Slow_Down;
idata unsigned char Temperature_Value;
idata unsigned int Temperature_Slow_Down;
idata unsigned char Light_Level = 1;
idata unsigned char AD_Slow_Down;

idata unsigned char Motion_State;
idata bit State_Change_Flag;
idata unsigned int Change_Time_3S;

idata unsigned char Distance_Value;
idata unsigned char Distance_Value_Old;
idata unsigned int Distance_Slow_Down;

idata unsigned char Temperature_Para = 30;
idata unsigned char Distance_Para = 30;

idata unsigned int Relay_Conut;
idata bit Relay_Led_Lock;

idata bit Long_Press_Flag;
idata unsigned int Long_Press_Time_2S;

idata bit Hot_Flag;

idata bit Run_State;
idata bit Stay_State;
idata bit Paralysis_State;

idata bit Led_Blink_Flag;
idata unsigned char Blink_Time_100ms;

idata bit Close_Flag;

idata bit First_Get_Distance;

idata bit Relay_Flag;
idata unsigned char Seg_Show_Mode;
idata unsigned char Para_Show_Mode;

void Key_Proc()
{
	if (Key_Slow_Down < 10)
		return;
	Key_Slow_Down = 0;
	Key_Val = Key_Read();
	Key_Down = Key_Val ^ (Key_Val & Key_Old);
	Key_Up = ~Key_Val ^ (Key_Val & Key_Old);
	Key_Old = Key_Val;
	if (Key_Down == 4)
	{
		Relay_Led_Lock = (Seg_Show_Mode == 1);
		Para_Show_Mode = 0;
		Seg_Show_Mode = (++Seg_Show_Mode) % 4;
	}

	if (Seg_Show_Mode == 2)
	{
		if (Key_Down == 5)
			Para_Show_Mode = (++Para_Show_Mode) % 2;
		if (Key_Down == 8)
		{
			switch (Para_Show_Mode)
			{
			case 0:

				Temperature_Para = (Temperature_Para == 80)
									   ? 80
									   : Temperature_Para + 1;
				break;

			case 1:

				Distance_Para = (Distance_Para == 80)
									? 80
									: Distance_Para + 5;
				break;
			}
		}
		if (Key_Down == 9)
		{
			switch (Para_Show_Mode)
			{

			case 0:

				Temperature_Para = (Temperature_Para == 20)
									   ? 20
									   : Temperature_Para - 1;
				break;

			case 1:

				Distance_Para = (Distance_Para == 20)
									? 20
									: Distance_Para - 5;
				break;
			}
		}
	}

	if (Seg_Show_Mode == 3)
	{
		if (Long_Press_Time_2S >= 2000)
			Relay_Conut = 0;
		if (Key_Old == 89)
			Long_Press_Flag = 1;
		else
		{
			Long_Press_Flag = 0;
			Long_Press_Time_2S = 0;
		}
	}
}

void Seg_Proc()
{
	if (Seg_Slow_Down < 80)
		return;
	Seg_Slow_Down = 0;
	switch (Seg_Show_Mode)
	{
	case 0:

		Seg_Buf[0] = 11;
		Seg_Buf[1] = Temperature_Value / 10 % 10;
		Seg_Buf[2] = Temperature_Value % 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = 10;
		Seg_Buf[6] = 12;
		Seg_Buf[7] = Light_Level;
		break;

	case 1:

		Seg_Buf[0] = 13; // L
		Seg_Buf[1] = Motion_State;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = Distance_Value / 100 % 10;
		Seg_Buf[6] = Distance_Value / 10 % 10;
		Seg_Buf[7] = Distance_Value % 10;
		break;

	case 2:

		Seg_Buf[0] = 14;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = 10;
		switch (Para_Show_Mode)
		{
		case 0:

			Seg_Buf[1] = 11;
			Seg_Buf[6] = Temperature_Para / 10 % 10;
			Seg_Buf[7] = Temperature_Para % 10;
			break;

		case 1:

			Seg_Buf[1] = 13; // L
			Seg_Buf[6] = Distance_Para / 10 % 10;
			Seg_Buf[7] = Distance_Para % 10;
			break;
		}
		break;

	case 3:

		Seg_Buf[0] = 12;
		Seg_Buf[1] = 11;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = (Relay_Conut >= 1000)
						 ? Relay_Conut / 1000 % 10
						 : 10;
		Seg_Buf[5] = (Relay_Conut >= 100)
						 ? Relay_Conut / 100 % 10
						 : 10;
		Seg_Buf[6] = (Relay_Conut >= 10)
						 ? Relay_Conut / 10 % 10
						 : 10;
		Seg_Buf[7] = Relay_Conut % 10;
		break;
	}
}

void Led_Proc()
{
	bit Temp_Relay_Flag;
	if (!Relay_Led_Lock)
	{
		ucLed[0] = (Light_Level >= 1);
		ucLed[1] = (Light_Level >= 2);
		ucLed[2] = (Light_Level >= 3);
		ucLed[3] = (Light_Level >= 4);
		Temp_Relay_Flag = (Close_Flag && Hot_Flag);
		if ((Relay_Flag == 0) && (Temp_Relay_Flag == 1))
			Relay_Conut++;
		Relay_Flag = Temp_Relay_Flag;
		Relay(Relay_Flag);
	}
	if (Stay_State)
		ucLed[7] = 0;
	else if (Paralysis_State)
		ucLed[7] = 1;
	else if (Run_State)
		ucLed[7] = Led_Blink_Flag;
	Led_Disp(ucLed);
}

void Get_Ad()
{
	unsigned char temp;
	if (AD_Slow_Down < 100)
		return;
	AD_Slow_Down = 0;
	temp = Ad_Read(0x03);
	if (temp >= 3 * 51)
		Light_Level = 1;
	else if (temp >= 2 * 51)
		Light_Level = 2;
	else if (temp >= 0.5 * 51)
		Light_Level = 3;
	else
		Light_Level = 4;
}
void Get_Temperature()
{
	float temp;
	if (Temperature_Slow_Down < 300)
		return;
	Temperature_Slow_Down = 0;
	temp = rd_temperature();
	Temperature_Value = temp;
	Hot_Flag = (temp > Temperature_Para);
}
void Get_Distance()
{
	unsigned char D_Distance;
	unsigned char Temp_Motion_State;
	if (Distance_Slow_Down < 999)
		return;
	Distance_Slow_Down = 0;
	if (First_Get_Distance == 1)
	{
		Distance_Value = Ut_Wave_Data();
		D_Distance = abs(Distance_Value_Old - Distance_Value);
		Distance_Value_Old = Distance_Value;
		Close_Flag = (Distance_Value < Distance_Para);
		if (!State_Change_Flag)
		{
			if (D_Distance < 5)
				Temp_Motion_State = 0;
			else if (D_Distance < 10)
				Temp_Motion_State = 1;
			else
				Temp_Motion_State = 2;

			State_Change_Flag = (Temp_Motion_State != Motion_State);
			Motion_State = Temp_Motion_State;
		}

		if (!Relay_Led_Lock)
		{
			Stay_State = (Motion_State == 0);
			Paralysis_State = (Motion_State == 1);
			Run_State = (Motion_State == 2);
		}
	}
	else
	{
		First_Get_Distance = 1;
		Distance_Value = Ut_Wave_Data();
		Distance_Value_Old = Distance_Value;
	}
}
void Timer1Init(void)
{
	AUXR &= 0xBF;
	TMOD &= 0x0F;
	TL1 = 0x18;
	TH1 = 0xFC;
	TF1 = 0;
	TR1 = 1;
	ET1 = 1;
	EA = 1;
}

void Timer1Isr() interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	AD_Slow_Down++;
	Temperature_Slow_Down++;
	Distance_Slow_Down++;

	Seg_Pos = (++Seg_Pos) % 8;
	if (Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
	else
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);

	if (Long_Press_Flag)
	{
		if (++Long_Press_Time_2S > 2000)
			Long_Press_Time_2S = 2001;
	}
	else
	{
		Long_Press_Time_2S = 0;
	}

	if (Run_State)
	{
		if (++Blink_Time_100ms == 100)
		{
			Blink_Time_100ms = 0;
			Led_Blink_Flag ^= 1;
		}
	}
	else
	{
		Blink_Time_100ms = 0;
		Led_Blink_Flag = 0;
	}

	if (State_Change_Flag)
	{
		if (++Change_Time_3S == 3000)
		{
			Change_Time_3S = 0;
			State_Change_Flag = 0;
		}
	}
	else
		Change_Time_3S = 0;
}

void main()
{
	System_Init();

	Timer1Init();

	while (1)
	{
		Led_Proc();
		Key_Proc();
		Seg_Proc();
		Get_Ad();
		Get_Temperature();
		Get_Distance();
	}
}