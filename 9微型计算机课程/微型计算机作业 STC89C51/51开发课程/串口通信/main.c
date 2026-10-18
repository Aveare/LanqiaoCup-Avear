/*头文件引用*/
#include <REGX51.H>
#include <Key.h>
/*变量声明*/
unsigned char Key_Val,Key_Down,Key_Old;//按键专用变量
unsigned char seg_dula[] = {0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff};
unsigned char seg_wela[] = {0x01,0x02};
unsigned char Seg_Pos;//数码管扫描变量
unsigned char Seg_Buf[2];
unsigned char rec_val; 

void Delay300us(void)	//@11.0592MHz
{
	unsigned char data i, j;
	i = 4;
	j = 54;
	do
	{
		while (--j);
	} while (--i);
}


/*串口初始化*/
void uart_init() 
{
    TMOD |= 0x20;  // T1工作于模式2
    TH1 = 0xFD;    // 波特率9600
    TL1 = 0xFD;
    TR1 = 1;       // 启动T1
    SCON = 0x50;   // 串口模式1，允许接收
    EA = 1;        // 开总中断
    ES = 1;        // 开串口中断
}


/* 键盘处理函数 */
void Key_Proc()
{
	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Old = Key_Val;//辅助扫描变量
	
  if(Key_Down>=1&&Key_Down<=16)
    {
        SBUF=Key_Down;    // 发送键值到串口
        while(!TI);         // 等待发送完成
        TI = 0;             // 清除标志位
    }
        
}
/*数码管显示函数*/
void display(unsigned char num)
{
	   unsigned char shi, ge;
    unsigned char shi_num, ge_num; 
    shi_num = num / 10; 
    ge_num = num % 10; 
    shi = seg_dula[shi_num];
    ge = seg_dula[ge_num];             //算出十位和个位的段码

 
    P2 = seg_wela[0];
    P0 = shi; 
		Delay300us();


    P2 = seg_wela[1];
    P0 = ge; 
		Delay300us();
}

/*串口中断服务函数*/
void uart_isr() interrupt 4
{
    if (RI) 
		{     
        rec_val = SBUF;  // 读取接收数据
        RI = 0;       // 清除接收标志
    }
    if (TI) 
		{         
        TI = 0;       // 清除标志
    }
}

/*Main*/
void main() 
{
    uart_init();
    while(1) 
		{
      Key_Proc(); 
			display(rec_val);
    } 
}