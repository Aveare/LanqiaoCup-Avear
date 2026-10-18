/**********************************************************************
 * 文件名: iic.c
 * 功能描述: I2C通信模块，实现I2C总线协议和外设操作
 * 包含功能:
 *   - I2C总线基本操作(起始、停止、发送字节、接收字节等)
 *   - PCF8591芯片AD/DA转换操作
 *   - AT24C02 EEPROM读写操作
 *
 * 说明: 
 *   1. 本文件提供的驱动代码供参赛选手完成程序设计参考
 *   2. 参赛选手可以自行编写相关代码或以该代码为基础进行修改
 **********************************************************************/

 #include "iic.h"
 #include "intrins.h"
 
 #define DELAY_TIME	10  // I2C总线时序延时参数
 
 // I2C引脚定义
 sbit scl = P2^0;       // I2C时钟线
 sbit sda = P2^1;       // I2C数据线
 
 /**
  * 功能: I2C延时函数
  * 描述: 通过空操作指令实现微秒级延时
  * 参数:
  *   n - 延时参数，控制延时时间
  */
 static void I2C_Delay(unsigned char n)
 {
	 do
	 {
		 // 执行多个空操作指令实现延时
		 _nop_();_nop_();_nop_();_nop_();_nop_();
		 _nop_();_nop_();_nop_();_nop_();_nop_();
		 _nop_();_nop_();_nop_();_nop_();_nop_();		
	 }
	 while(n--);      	
 }
 
 /**
  * 功能: I2C起始信号
  * 描述: 
  *   - 产生I2C通信起始信号
  *   - 时序: SCL、SDA同为高 -> SDA拉低 -> SCL拉低
  */
 void I2CStart(void)
 {
	 sda = 1;             // 数据线拉高
	 scl = 1;             // 时钟线拉高
	 I2C_Delay(DELAY_TIME);
	 sda = 0;             // 数据线拉低(在时钟线为高时拉低数据线，产生起始信号)
	 I2C_Delay(DELAY_TIME);
	 scl = 0;             // 时钟线拉低，准备发送数据
 }
 
 /**
  * 功能: I2C停止信号
  * 描述: 
  *   - 产生I2C通信停止信号
  *   - 时序: SCL低SDA低 -> SCL拉高 -> SDA拉高
  */
 void I2CStop(void)
 {
	 sda = 0;             // 数据线拉低
	 scl = 1;             // 时钟线拉高
	 I2C_Delay(DELAY_TIME);
	 sda = 1;             // 数据线拉高(在时钟线为高时拉高数据线，产生停止信号)
	 I2C_Delay(DELAY_TIME);
 }
 
 /**
  * 功能: I2C发送一个字节
  * 描述: 
  *   - 按位传输一个字节数据(高位先传)
  *   - 每位数据在SCL低电平时准备，SCL高电平时保持稳定
  * 参数:
  *   byt - 要发送的字节
  */
 void I2CSendByte(unsigned char byt)
 {
	 unsigned char i;
	 
	 for(i=0; i<8; i++){
		 scl = 0;         // 时钟线拉低，准备设置数据线
		 I2C_Delay(DELAY_TIME);
		 if(byt & 0x80){  // 判断最高位
			 sda = 1;     // 如果最高位为1，则数据线拉高
		 }
		 else{
			 sda = 0;     // 如果最高位为0，则数据线拉低
		 }
		 I2C_Delay(DELAY_TIME);
		 scl = 1;         // 时钟线拉高，从机读取数据位
		 byt <<= 1;       // 数据左移一位，准备发送下一位
		 I2C_Delay(DELAY_TIME);
	 }
	 
	 scl = 0;             // 发送完8位数据后，时钟线拉低
 }
 
 /**
  * 功能: I2C接收一个字节
  * 描述: 
  *   - 按位接收一个字节数据(高位先接收)
  *   - 每位数据在SCL高电平时采样
  * 返回值:
  *   接收到的字节
  */
 unsigned char I2CReceiveByte(void)
 {
	 unsigned char da = 0;
	 unsigned char i;
	 for(i=0;i<8;i++){   
		 scl = 1;         // 时钟线拉高，准备读取数据线
		 I2C_Delay(DELAY_TIME);
		 da <<= 1;        // 数据左移一位，为接收新的一位做准备
		 if(sda)          // 读取数据线状态
			 da |= 0x01;  // 如果数据线为高，则将接收数据的最低位置1
		 scl = 0;         // 时钟线拉低，准备接收下一位
		 I2C_Delay(DELAY_TIME);
	 }
	 return da;           // 返回接收到的字节
 }
 
 /**
  * 功能: I2C等待从机应答
  * 描述: 
  *   - 发送完一个字节后等待从机应答
  *   - 应答信号为SDA线拉低
  * 返回值:
  *   0 - 接收到应答
  *   1 - 未接收到应答
  */
 unsigned char I2CWaitAck(void)
 {
	 unsigned char ackbit;
	 
	 scl = 1;             // 时钟线拉高
	 I2C_Delay(DELAY_TIME);
	 ackbit = sda;        // 读取应答位(0表示应答，1表示非应答)
	 scl = 0;             // 时钟线拉低
	 I2C_Delay(DELAY_TIME);
	 
	 return ackbit;       // 返回应答位
 }
 
 /**
  * 功能: I2C发送应答
  * 描述: 
  *   - 接收完一个字节后发送应答或非应答
  *   - 应答信号为SDA线拉低，非应答为SDA线拉高
  * 参数:
  *   ackbit - 应答位值(0表示应答，1表示非应答)
  */
 void I2CSendAck(unsigned char ackbit)
 {
	 scl = 0;             // 时钟线拉低，准备设置应答位
	 sda = ackbit;        // 设置应答位
	 I2C_Delay(DELAY_TIME);
	 scl = 1;             // 时钟线拉高，从机读取应答位
	 I2C_Delay(DELAY_TIME);
	 scl = 0;             // 时钟线拉低
	 sda = 1;             // 释放数据线
	 I2C_Delay(DELAY_TIME);
 }
 
 /**
  * 功能: PCF8591 AD转换
  * 描述: 
  *   - 从PCF8591的指定通道读取AD转换值
  *   - 读取范围: 0~255 (对应0~5V)
  * 参数:
  *   addr - 控制字，指定通道和配置
  *     0x40/0x00 - 开启DA/不开启DA，外部输入
  *     0x41/0x01 - 开启DA/不开启DA，光敏电阻输入
  *     0x42/0x02 - 开启DA/不开启DA，差分输入
  *     0x43/0x03 - 开启DA/不开启DA，滑动变阻器输入
  * 返回值:
  *   AD转换结果(0~255)
  */
 unsigned char Ad_Read(unsigned char addr)
 {
	 unsigned char temp;
	 
	 I2CStart();                 // 发送起始信号
	 I2CSendByte(0x90);          // 发送PCF8591设备地址+写操作(0x48<<1|0)
	 I2CWaitAck();               // 等待应答
	 I2CSendByte(addr);          // 发送控制字(通道选择)
	 I2CWaitAck();               // 等待应答
	 
	 I2CStart();                 // 重新发送起始信号
	 I2CSendByte(0x91);          // 发送PCF8591设备地址+读操作(0x48<<1|1)
	 I2CWaitAck();               // 等待应答
	 temp = I2CReceiveByte();    // 读取AD转换结果
	 I2CSendAck(1);              // 发送非应答信号(结束读取)
	 I2CStop();                  // 发送停止信号
	 
	 return temp;                // 返回AD转换结果
 }
	 
 /**
  * 功能: PCF8591 DA转换
  * 描述: 
  *   - 向PCF8591的DA输出设置模拟量
  *   - 设置范围: 0~255 (对应0~5V)
  * 参数:
  *   dat - DA转换值(0~255)
  */
 void Da_Write(unsigned char dat)
 {
	 I2CStart();                 // 发送起始信号
	 I2CSendByte(0x90);          // 发送PCF8591设备地址+写操作
	 I2CWaitAck();               // 等待应答
	 I2CSendByte(0x41);          // 发送控制字(开启DA输出)
	 I2CWaitAck();               // 等待应答
	 I2CSendByte(dat);           // 发送DA值
	 I2CWaitAck();               // 等待应答
 }
 
 /**
  * 功能: AT24C02 EEPROM写入
  * 描述: 
  *   - 向AT24C02指定地址写入一组数据
  * 参数:
  *   str - 待写入数据的数组指针
  *   addr - 写入的起始地址(0~255)
  *   num - 写入的数据个数
  */
 void EEPROM_Write(unsigned char *str, unsigned char addr, unsigned char num)
 {
	 I2CStart();                 // 发送起始信号
	 I2CSendByte(0xa0);          // 发送AT24C02设备地址+写操作(0x50<<1|0)
	 I2CWaitAck();               // 等待应答
	 I2CSendByte(addr);          // 发送写入地址
	 I2CWaitAck();               // 等待应答
	 
	 // 循环写入多个字节
	 while(num--)
	 {
		 I2CSendByte(*str++);    // 发送数据
		 I2CWaitAck();           // 等待应答
		 I2C_Delay(200);         // 写入延时
	 }
	 I2CStop();                  // 发送停止信号
	 
	 // 写入后延时，确保写入时序正常(部分AT24C02需要较长时间完成写入)
	 I2C_Delay(255);
	 I2C_Delay(255);
	 I2C_Delay(255);
	 I2C_Delay(255);
 }
 
 /**
  * 功能: AT24C02 EEPROM读取
  * 描述: 
  *   - 从AT24C02指定地址读取一组数据
  * 参数:
  *   str - 用于存储读取结果的数组指针
  *   addr - 读取的起始地址(0~255)
  *   num - 读取的数据个数
  */
 void EEPROM_Read(unsigned char *str, unsigned char addr, unsigned char num)
 {
	 I2CStart();                 // 发送起始信号
	 I2CSendByte(0xa0);          // 发送AT24C02设备地址+写操作
	 I2CWaitAck();               // 等待应答
	 I2CSendByte(addr);          // 发送读取地址
	 I2CWaitAck();               // 等待应答
	 
	 I2CStart();                 // 重新发送起始信号
	 I2CSendByte(0xa1);          // 发送AT24C02设备地址+读操作(0x50<<1|1)
	 I2CWaitAck();               // 等待应答
	 
	 // 循环读取多个字节
	 while(num--)
	 {
		 *str++=I2CReceiveByte(); // 读取数据并存储
		 if(num)
			 I2CSendAck(0);      // 如果还有数据，发送应答信号继续读取
		 else
			 I2CSendAck(1);      // 如果是最后一个数据，发送非应答信号结束读取
	 }
	 I2CStop();                  // 发送停止信号
 }