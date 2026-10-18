#include <Led.h>


idata  unsigned char temp_1 = 0x00;
idata  unsigned char temp_old_1 = 0xff;

void Led_Disp(unsigned char *ucled)
{
   unsigned char temp;
   temp_1=0x00;
   temp_1 = (ucled[0] << 0) | (ucled[1] << 1) | (ucled[2] << 2) | (ucled[3] << 3) |
         (ucled[4] << 4) | (ucled[5] << 5) | (ucled[6] << 6) | (ucled[7] << 7);
  if (temp_1 != temp_old_1)
  {
    P0 = ~temp_1;
    
    // 操作P2锁存器
    temp = P2 & 0x1f;    // 保留P2的低5位
    temp = temp | 0x80;  // 与0x80进行或操作
    P2 = temp;           // 写入P2
    temp = P2 & 0x1f;    // 保留P2的低5位
    P2 = temp;           // 写入P2，关闭锁存器
    
    temp_old_1 = temp_1;
  }
}


void Led_Off()
{
    unsigned char temp;
	
    P0 = 0xff;
    
    // 操作P2锁存器
    temp = P2 & 0x1f;    // 保留P2的低5位
    temp = temp | 0x80;  // 与0x80进行或操作
    P2 = temp;           // 写入P2
    temp = P2 & 0x1f;    // 保留P2的低5位
    P2 = temp;           // 写入P2，关闭锁存器
    
    temp_old_1=0x00;
}


idata unsigned char temp_0 = 0x00;
idata unsigned char temp_old_0 = 0xff;
/// @brief 蜂鸣器控制
/// @param enable
void Beep(bit enable)
{
  unsigned char temp;
  
  if (enable)
    temp_0 |= 0x40;
  else
    temp_0 &= ~(0x40);
  if (temp_0 != temp_old_0)
  {
    P0 = temp_0;
    
    // 操作P2锁存器
    temp = P2 & 0x1f;    // 保留P2的低5位
    temp = temp | 0xa0;  // 与0xa0进行或操作
    P2 = temp;           // 写入P2
    temp = P2 & 0x1f;    // 保留P2的低5位
    P2 = temp;           // 写入P2，关闭锁存器
    
    temp_old_0 = temp_0;
  }
}
/// @brief 继电器控制
/// @param enable
void Relay(bit enable)
{
  unsigned char temp;
  
  if (enable)
    temp_0 |= 0x10;
  else
    temp_0 &= ~(0x10);
  if (temp_0 != temp_old_0)
  {
    P0 = temp_0;
    
    // 操作P2锁存器
    temp = P2 & 0x1f;    // 保留P2的低5位
    temp = temp | 0xa0;  // 与0xa0进行或操作
    P2 = temp;           // 写入P2
    temp = P2 & 0x1f;    // 保留P2的低5位
    P2 = temp;           // 写入P2，关闭锁存器
    
    temp_old_0 = temp_0;
  }
}

