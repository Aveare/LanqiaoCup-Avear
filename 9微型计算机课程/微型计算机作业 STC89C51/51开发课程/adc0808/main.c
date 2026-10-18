/*头文件引用*/
#include <REGX51.H>

sbit start =P3^0;
sbit eoc   =P3^2;
sbit oe    =P3^3;
unsigned int vol=0;
unsigned char bai,shi,ge;
unsigned code duanxuan[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
/*延迟函数*/
void delay(unsigned char n)
{
	unsigned char i,j;
	for(i=0;i<n;i++)
		for(j=0;j<110;j++);
}

/* 外部中断0初始化函数*/
void Int0_Init()
{
    IT0=1;
    EX0=1;    //使能外部中断0
    EA=1;    //开启总中断
}
/*ADC0808启动*/
void ADC0808_Start()
{
    start = 0;
    delay(5);
    start = 1;
    delay(5);
    start = 0;
}

/*外部中断0服务函数*/
void Int0_ISR(void) interrupt 0
{
    oe=1;    
    delay(5);
    vol=P1;   //读取ADC0808
    oe=0;     //关闭ADC
	
	vol=(vol*100)/51;     //0-255转换到0-5v
	bai=vol/100%10;
	shi=vol/10%10;
	ge=vol%10;
	
	P0=0xFE;//第0位
	P2=duanxuan[bai];
	delay(1);
	
	P0=0xFD;//第1位
	P2=0x10;
	delay(1);
	
	P0=0xFB;//第2位
	P2=duanxuan[shi];
	delay(1);
	
	P0=0xF7;//第3位
	P2=duanxuan[ge];
	delay(1);
	
	ADC0808_Start();//等待下一次
}

/*Main*/
void main()
{
	Int0_Init();  
  ADC0808_Start();   
	while(1);
}