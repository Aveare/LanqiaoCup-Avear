/*头文件声明*/
#include <REGX51.H>

/*变量声明*/
unsigned char a;//循环变量

/*延时函数*/
void Delay500ms()		//@12.000MHz
{
	unsigned char i, j, k;
	i = 23;
	j = 205;
	k = 120;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}


/*其他处理函数Led*/
void Led_Proc()
{
	for(a=0;a<8;a++)
	{
		P1=~(0x01<<a);
		Delay500ms();
	}
}

/*Main*/
void main()
{
	while(1)
	{
		Led_Proc();	
	}
}