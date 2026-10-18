/*头文件引用*/
#include <REGX51.H>

/* 变量声明*/
unsigned char ucLed[8] = {0xFE,0xFD,0xFB,0xF7,0xEF,0xDF,0xBF,0x7F};
bit Led_Mode1 = 0;       
bit Led_Mode2 = 0;       
unsigned char Led_Index = 0;

/* 延时函数*/
void Delay200ms(void)	//@12.000MHz
{
	unsigned char data i, j, k;
	i = 10;
	j = 31;
	k = 147;
	do { do { while (--k); } while (--j); } while (--i);
}

void Delay20ms(void)	//@12.000MHz
{
	unsigned char data i, j;
	i = 234;
	j = 115;
	do { while (--j); } while (--i);
}

/*流水灯*/
void Led1() 
{
	P1 = ucLed[Led_Index];  //点亮灯
	Delay200ms();
	Led_Index++;            
	if (Led_Index>=8) 
	{  
		Led_Index = 0;
	}
}

/*闪烁*/
void Led2() 
{

	P1 = 0x00; Delay20ms();
	P1 = 0xFF; Delay20ms();
	P1 = 0x00; Delay20ms();
	P1 = 0xFF; Delay20ms();
	P1 = 0x00; Delay20ms();
	P1 = 0xFF; Delay20ms();
	Led_Mode2 = 0;  // 闪烁完成后标志位置0
}

/*初始化*/
void Init() {
	EA = 1;   // 总中断允许
	EX0 = 1;  // 允许INT0中断
	EX1 = 1;  // 允许INT1中断
	IT0 = 1;  // INT0下降沿触发
	IT1 = 1;  // INT1下降沿触发
	IP = 0x04;// 
	P1 = 0xFF;//LED全灭
}

/*INT0中断服务函数*/
void int0server(void) interrupt 0 
{
	Led_Mode1 = 1;
}

/*INT1中断服务函数*/
void int1server(void) interrupt 2 
{
	Led_Mode2 = 1;
}

/*Main*/
void main()
{
	Init();
	while(1)
		{
		if      (Led_Mode2) Led2(); 
		else if (Led_Mode1) Led1();
	}
}