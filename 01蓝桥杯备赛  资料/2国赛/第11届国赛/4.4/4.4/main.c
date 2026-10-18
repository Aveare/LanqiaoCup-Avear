#include <STC15F2K60S2.H>
#include <iic.h>
#include <ultra.h>
#include <math.h>
#include <stdio.h>
#include <uart.h>
#include <string.h>
#include <stdlib.h>
#include <intrins.h>

/*==================== 底层变量 ====================*/
unsigned char Key_Slow;
unsigned int Seg_Slow;
unsigned char Key_Val, Key_Down, Key_Up, Key_Old;
unsigned char Seg_Pos;
unsigned char code seg_wei[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code seg_table[] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,0xbf,0xc7,0x86,0x8c};
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
unsigned char Seg_Point[8] = {0,0,0,0,0,0,0,0};
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};
/*底层函数声明*/
void Init();
unsigned char Key_Read();
void Seg_Disp(unsigned char wei, unsigned char table, bit point);
void Led_Disp(unsigned char addr, bit enable);
void Relay(bit enable);
/*==================== 业务变量 ====================*/
unsigned int freq;
unsigned int Timer1s;
unsigned char Disp_Mode = 0; //0-坐标界面 1-速度界面 2-参数界面
unsigned char Work_Mode = 0; //0空闲，1运行，2等待
int Place_X_Now = 0;
int Place_Y_Now = 0;
int Place_X_Destination = 0;
int Place_Y_Destination = 0;
float speed;
unsigned char distance;
unsigned char R_Set_10x = 10;
unsigned char R_Set_10x_Control = 10;
char B_Set = 0;
char B_Set_Control = 0;
bit Set_Mode = 0;
bit Wait_Flag = 0;
bit Get_Place_Flag = 0;
long int distance_count;
float temp_x, temp_y;
unsigned int Timer500ms;
bit Star_Flag = 0;
unsigned char Timer100ms;
bit Destination_Flag = 0;
bit L3_Onflag;
unsigned char Lex;
bit Dark_Flag;
unsigned char Data_Down;
unsigned int Timer3s;
/*==================== 串口变量 ====================*/
unsigned int Sys_Tick;
bit Rec_Flag;
unsigned char Uart_Rec[10];
unsigned char Uart_Rec_Index;
bit Send1;
bit Send2;
bit Send3;
bit Error_Flag = 0;

/*==================== 按键处理 ====================*/
void Key_Proc()
	{
    if(Key_Slow) return;
    Key_Slow = 1;
    Key_Val = Key_Read();
    Key_Down = Key_Val & (Key_Old ^ Key_Val);
    Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
    Key_Old = Key_Val;

    switch(Key_Down) {
        case 4: // S4
            if(Get_Place_Flag==1 && Work_Mode==0) {
                Get_Place_Flag=0;
                Wait_Flag=1;
                Work_Mode=1;
            } else if(Work_Mode==1) {
                Work_Mode=2;
            } else if(Work_Mode==2 && distance>30) {
                Work_Mode=1;
            }
            break;
        case 5: // 重置
            if(Work_Mode==0) {
                Place_X_Now = Place_Y_Now = 0;
            }
            break;
        case 8: // 界面切换
            Disp_Mode = (Disp_Mode + 1) % 3;
            if(Disp_Mode==0) {
                Set_Mode=0;
                R_Set_10x=R_Set_10x_Control;
                B_Set=B_Set_Control;
            }
            break;
        case 9: // 参数选择
            if(Disp_Mode==2) Set_Mode ^= 1;
            break;
        case 12: // +
            if(Set_Mode==0) {
                R_Set_10x_Control += 1;
                if(R_Set_10x_Control>20) R_Set_10x_Control=10;
            } else {
                B_Set_Control +=5;
                if(B_Set_Control>90) B_Set_Control=-90;
            }
            break;
        case 13: // -
            if(Set_Mode==0) {
                R_Set_10x_Control -= 1;
                if(R_Set_10x_Control<10) R_Set_10x_Control=20;
            } else {
                B_Set_Control -=5;
                if(B_Set_Control<-90) B_Set_Control=90;
            }
            break;
    }
}

/*==================== 数码管显示处理 ====================*/
void Seg_Proc() {
    if(Seg_Slow) return;
    Seg_Slow = 1;

    switch(Disp_Mode) {
        case 0: // 坐标界面
            if(Work_Mode==0) {
                Seg_Buf[0]=12; // L
                Seg_Buf[1]=Place_X_Now>100?Place_X_Now/100%10:10;
                Seg_Buf[2]=Place_X_Now>10?Place_X_Now/10%10:10;
                Seg_Buf[3]=Place_X_Now%10;
                Seg_Buf[4]=11; // -
                Seg_Buf[5]=Place_Y_Now>100?Place_Y_Now/100%10:10;
                Seg_Buf[6]=Place_Y_Now>10?Place_Y_Now/10%10:10;
                Seg_Buf[7]=Place_Y_Now%10;
            } else {
                Seg_Buf[0]=12;
                Seg_Buf[1]=Place_X_Destination>100?Place_X_Destination/100%10:10;
                Seg_Buf[2]=Place_X_Destination>10?Place_X_Destination/10%10:10;
                Seg_Buf[3]=Place_X_Destination%10;
                Seg_Buf[4]=11;
                Seg_Buf[5]=Place_Y_Destination>100?Place_Y_Destination/100%10:10;
                Seg_Buf[6]=Place_Y_Destination>10?Place_Y_Destination/10%10:10;
                Seg_Buf[7]=Place_Y_Destination%10;
            }
            Seg_Point[2]=Seg_Point[6]=0;
            break;
        case 1: // 速度界面
            switch(Work_Mode) {
                case 1:
                    Seg_Buf[0]=13; Seg_Buf[1]=1; Seg_Buf[2]=10;
                    Seg_Buf[3]=(speed>1000)?(unsigned int)speed/1000%10:10;
                    Seg_Buf[4]=(speed>100)?(unsigned int)speed/100%10:10;
                    Seg_Buf[5]=(speed>10)?(unsigned int)speed/10%10:10;
                    Seg_Buf[6]=(unsigned int)speed%10;
                    Seg_Buf[7]=(unsigned int)(speed*10)%10;
                    Seg_Point[6]=1;
                    break;
                case 0:
                    Seg_Buf[0]=13; Seg_Buf[1]=2; Seg_Buf[2]=10;
                    Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=Seg_Buf[6]=Seg_Buf[7]=11;
                    Seg_Point[6]=0;
                    break;
                case 2:
                    Seg_Buf[0]=13; Seg_Buf[1]=3; Seg_Buf[2]=10;
                    Seg_Buf[3]=distance>10000?distance/10000%10:10;
                    Seg_Buf[4]=distance>1000?distance/1000%10:10;
                    Seg_Buf[5]=distance>100?distance/100%10:10;
                    Seg_Buf[6]=distance>10?distance/10%10:10;
                    Seg_Buf[7]=distance%10;
                    Seg_Point[6]=0;
                    break;
            }
            Seg_Point[2]=0;
            break;
        case 2: // 参数界面
            Seg_Buf[0]=14; Seg_Buf[1]=Seg_Buf[4]=10;
            Seg_Buf[2]=R_Set_10x_Control/10%10;
            Seg_Buf[3]=R_Set_10x_Control%10;
            Seg_Point[2]=1;
            if(B_Set_Control<0) {
                Seg_Buf[5]=(-B_Set_Control>=10)?11:10;
                Seg_Buf[6]=(-B_Set_Control<10)?11:(-B_Set_Control)/10%10;
                Seg_Buf[7]=(-B_Set_Control)%10;
                Seg_Point[6]=0;
            } else {
                Seg_Buf[5]=(B_Set_Control<100)?10:B_Set_Control/100%10;
                Seg_Buf[6]=(B_Set_Control<10)?10:B_Set_Control/10%10;
                Seg_Buf[7]=B_Set_Control%10;
                Seg_Point[6]=0;
            }
            break;
    }
}

/*==================== LED显示处理 ====================*/
void Led_Proc() {
    if(Work_Mode==0) ucLed[0]=0;
    else if(Work_Mode==1) ucLed[0]=1;
    else if(Work_Mode==2) ucLed[0]=Star_Flag;

    if(Work_Mode && Dark_Flag) ucLed[1]=1;
    else ucLed[1]=0;

    if(Destination_Flag==0) L3_Onflag=0;
    ucLed[2]=L3_Onflag;

    Relay(Work_Mode==1);
}

/*==================== 数据处理 ====================*/
void Data_Proc() {
    if(Data_Down) return;
    Data_Down=1;

	speed=(3.14f*R_Set_10x/10.0f*freq/100.0f)+B_Set;
    distance=Ut_Wave_Data()+3;
    if(distance<30 && Work_Mode==1) Work_Mode=2;
    Lex=AD_Read(0x01);
	Dark_Flag=(Lex<61);
    if(Work_Mode==1 && Wait_Flag==1) {
        distance_count=sqrt((float)(Place_X_Destination-Place_X_Now)*(Place_X_Destination-Place_X_Now) + (float)(Place_Y_Destination-Place_Y_Now)*(Place_Y_Destination-Place_Y_Now));
        temp_x=(float)(Place_X_Destination-Place_X_Now)/distance_count;
        temp_y=(float)(Place_Y_Destination-Place_Y_Now)/distance_count;
        Wait_Flag=0;
    }

    if(Work_Mode==1) {
        if(distance_count<=0) {
            distance_count=0;
            Place_X_Now=Place_X_Destination;
            Place_Y_Now=Place_Y_Destination;
            Work_Mode=0;
            Destination_Flag=1;
            Timer3s=0;
        }
    }
}

/*==================== 串口处理 ====================*/
void Uart_Proc() {
    unsigned char i;
    if(Uart_Rec_Index==0) return;
    if(Uart_Rec_Index>=10) {Uart_Rec_Index=0; Error_Flag=1;}

    if(Sys_Tick>10) {
        Rec_Flag=Sys_Tick=0;
        if(Uart_Rec_Index>=5 && Uart_Rec[0]=='(' && Uart_Rec[Uart_Rec_Index-1]==')') {
            Send1=1;
            if(Work_Mode==0) {
                Place_X_Destination=0; Place_Y_Destination=0;
                Get_Place_Flag=1;
                for(i=1;i<Uart_Rec_Index;i++) {
                    if(Uart_Rec[i]!=',') Place_X_Destination=Place_X_Destination*10+(Uart_Rec[i]-'0');
                    else break;
                }
                i++;
                for(;i<Uart_Rec_Index;i++) {
                    if(Uart_Rec[i]!=')') Place_Y_Destination=Place_Y_Destination*10+(Uart_Rec[i]-'0');
                    else break;
                }
            }
        } else if(Uart_Rec_Index==1 && Uart_Rec[0]=='?') Send2=1;
        else if(Uart_Rec_Index==1 && Uart_Rec[0]=='#') Send3=1;
        else Error_Flag=1;

        memset(Uart_Rec,0,Uart_Rec_Index);
        Uart_Rec_Index=0;
    }

    if(Send1) {
        if(Work_Mode==0) printf("Got it");
        else if(Work_Mode==1) printf("Busy");
        Send1=0;
    }
    if(Send2) {
        if(Work_Mode==0) printf("Idle");
        else if(Work_Mode==1) printf("Busy");
        else if(Work_Mode==2) printf("Wait");
        Send2=0;
    }
    if(Send3) {
        printf("(%d,%d)",(unsigned int)Place_X_Now,(unsigned int)Place_Y_Now);
        Send3=0;
    }
    if(Error_Flag) {
        printf("Error");
        Error_Flag=0;
    }
}
/*==================== 定时器0初始化（频率计数） ====================*/
void Timer0Init(void)
{
    AUXR &= 0x7F;
    TMOD &= 0xF0;
    TMOD |= 0x05;
    TL0 = 0x00;
    TH0 = 0x00;
    TF0 = 0;
    TR0 = 1;
}

/*==================== 定时器1初始化（1ms中断） ====================*/
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
/*==================== 定时器中断 ====================*/
void Timer1server() interrupt 3 {
    if(++Key_Slow==10) Key_Slow=0;
    if(++Seg_Slow==300) Seg_Slow=0;
    if(++Data_Down==300) Data_Down=0;
    if(++Seg_Pos==8) Seg_Pos=0;

    Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], Seg_Point[Seg_Pos]);
    Led_Disp(Seg_Pos, ucLed[Seg_Pos]);

    if(Rec_Flag) Sys_Tick++;

    if(++Timer1s==1000) 
	{
        Timer1s=0;
        freq=(TH0<<8)|TL0;
        TH0=TL0=0;
       
    }

    if(Work_Mode==1 && distance_count>=0 && ++Timer500ms==500) {
        Timer500ms=0;
        distance_count -= speed*0.5;
        Place_X_Now += speed*0.5*temp_x;
        Place_Y_Now += speed*0.5*temp_y;
    }

    if(++Timer100ms==100) { Timer100ms=0; Star_Flag^=1; }

    if(Destination_Flag==1) {
        L3_Onflag=1;
        Timer3s++;
        if(Timer3s>=3000) { Timer3s=0; Destination_Flag=0; L3_Onflag=0; }
    }
}

void uart1proc() interrupt 4 {
    if(RI==1) {
        Rec_Flag=1;
        Sys_Tick=0;
        Uart_Rec[Uart_Rec_Index++]=SBUF;
        RI=0;
    }
    if(Uart_Rec_Index>=10) Uart_Rec_Index=0;
}

/*==================== 主函数 ====================*/
void main() {
    Init();
    Timer1Init();
    Timer0Init();
    UartInit();
    while(1) {
        Key_Proc();
        Data_Proc();
        Seg_Proc();
        Led_Proc();
        Uart_Proc();
    }
}


/*==================== 底层初始化 ====================*/
void Init()
{
    P2 = 0x80; P0 = 0xff; P2 = 0x00;
    P2 = 0xA0; P0 = 0x00; P2 = 0x00;
}

/*==================== 按键读取 ====================*/
unsigned char Key_Read()
{
    unsigned char temp = 0; // 初始化0，避免随机值
    P44 = 0; P42 = 1; P35 = 1;
    if(P33 == 0) temp = 4;
    if(P32 == 0) temp = 5;
    P44 = 1; P42 = 0; P35 = 1;
    if(P33 == 0) temp = 8;
    if(P32 == 0) temp = 9;
    P44 = 1; P42 = 1; P35 = 0;
    if(P33 == 0) temp = 12;
    if(P32 == 0) temp = 13;
    return temp;
}

/*==================== 数码管显示 ====================*/
void Seg_Disp(unsigned char wei, unsigned char table, bit point)
{
    P2 = 0xe0; P0 = 0x00; P2 = 0x00; // 消影（修正：全亮消影，避免鬼影）
    P2 = 0xc0; P0 = seg_wei[wei]; P2 = 0x00;
    P2 = 0xe0;
    P0 = seg_table[table];
    if(point) P0 &= ~0x80;
    P2 = 0x00;
}

/*==================== LED显示 ====================*/
void Led_Disp(unsigned char addr, bit enable)
{
    static unsigned char temp = 0x00;
    if(enable)
        temp |= (1 << addr);
    else
        temp &= ~(1 << addr);
    P2 = 0x80; P0 = ~temp; P2 = 0x00;
}

/*==================== 继电器控制 ====================*/
void Relay(bit enable)
{
    unsigned char temp = 0x00;
    if(enable)
        temp |= 0x10;
    P2 = 0xa0; P0 = temp; P2 = 0x00;
}