/***********************************************************************
 * 文件名: onewire.c
 * 功能描述: 单总线通信协议实现，用于DS18B20温度传感器通信
 * 主要函数: 
 *   - 单总线基础函数: Delay_OneWire, Write_DS18B20, Read_DS18B20, init_ds18b20
 *   - 温度读取: rd_temperature
 ***********************************************************************/

 #include "onewire.h"

 // 单总线通信引脚定义
 sbit DQ=P1^4;    // 数据线
 
 /**
  * 功能: 单总线通信延时函数
  * 描述: 提供单总线通信所需的精确延时
  * 参数: t - 延时时间参数
  */
 void Delay_OneWire(unsigned int t)  
 {
	 unsigned char i;
	 while(t--){
		 for(i=0;i<12;i++);  // 循环延时
	 }
 }
 
 /**
  * 功能: 向DS18B20写入一个字节
  * 描述: 按位发送，低位先发
  * 参数: dat - 要写入的数据
  */
 void Write_DS18B20(unsigned char dat)
 {
	 unsigned char i;
	 for(i=0;i<8;i++)    // 循环8次，发送8位数据
	 {
		 DQ = 0;             // 拉低数据线，开始写时序
		 DQ = dat&0x01;      // 输出数据位
		 Delay_OneWire(5);   // 延时保持
		 DQ = 1;             // 释放数据线
		 dat >>= 1;          // 准备下一位数据
	 }
	 Delay_OneWire(5);    // 写完一个字节后延时
 }
 
 /**
  * 功能: 从DS18B20读取一个字节
  * 描述: 按位读取，低位先到
  * 返回: 读取到的数据
  */
 unsigned char Read_DS18B20(void)
 {
	 unsigned char i;
	 unsigned char dat;
   
	 for(i=0;i<8;i++)    // 循环8次，读取8位数据
	 {
		 DQ = 0;             // 拉低数据线，开始读时序
		 dat >>= 1;          // 数据右移，为接收新的一位做准备
		 DQ = 1;             // 释放数据线
		 if(DQ)              // 读取数据线状态
		 {
			 dat |= 0x80;    // 如果是高电平，则将最高位置1
		 }	    
		 Delay_OneWire(5);   // 延时保持
	 }
	 return dat;          // 返回读取到的数据
 }
 
 /**
  * 功能: 初始化DS18B20
  * 描述: 发送复位脉冲并检测DS18B20是否存在
  * 返回: 0表示DS18B20存在，1表示DS18B20不存在
  */
 bit init_ds18b20(void)
 {
	   bit initflag = 0;
	   
	   DQ = 1;               // 数据线初始为高电平
	   Delay_OneWire(12);    // 延时
	   DQ = 0;               // 拉低数据线，发送复位脉冲
	   Delay_OneWire(80);    // 延时至少480us
	   DQ = 1;               // 释放数据线
	   Delay_OneWire(10);    // 延时等待DS18B20响应
	 initflag = DQ;        // 读取数据线状态，如果为低则表示DS18B20存在
	   Delay_OneWire(5);     // 延时
   
	   return initflag;      // 返回初始化结果
 }
 
 /**
  * 功能: 读取DS18B20温度值
  * 描述: 执行温度转换并读取温度值
  * 返回: 浮点型温度值(单位:摄氏度)
  */
 float rd_temperature()
 {
	 unsigned char low, high;  // 存储温度值的低字节和高字节
	 
	 // 初始化DS18B20
	 init_ds18b20();
	 
	 // 发送跳过ROM命令(0xCC)，直接操作器件
	 Write_DS18B20(0xcc);
	 
	 // 发送温度转换命令(0x44)
	 Write_DS18B20(0x44);
	 
	 // 等待温度转换完成(DS18B20的温度转换最长需要750ms)
	 Delay_OneWire(200);
	 
	 // 重新初始化DS18B20
	 init_ds18b20();
	 
	 // 发送跳过ROM命令(0xCC)
	 Write_DS18B20(0xcc);
	 
	 // 发送读暂存器命令(0xBE)
	 Write_DS18B20(0xbe);
	 
	 // 读取温度值(低字节在前，高字节在后)
	 low = Read_DS18B20();    // 读取温度值低字节
	 high = Read_DS18B20();   // 读取温度值高字节
	 
	 // 将高低字节组合为16位温度值，并转换为摄氏度
	 // DS18B20的分辨率为0.0625°C/位
	 return (float)(high<<8|low) * 0.0625;
 }