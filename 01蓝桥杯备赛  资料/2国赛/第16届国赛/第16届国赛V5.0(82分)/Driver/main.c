#include "at.h"

//数码管
xdata unsigned char Display_Mode = 0;
xdata unsigned char data_mode = 0;
xdata unsigned char para_mode = 0;

//按键
xdata unsigned char kbt_cnt = 0;

//串口
idata unsigned int uart_cnt = 0;
xdata unsigned char uart_valid_cnt = 0;
xdata unsigned char uart_invalid_cnt = 0;
xdata unsigned char uart_last_pos = 0;
bit uart_config_active = 0;
idata unsigned char comma_index = 0;
idata unsigned char paren_left = 0;//左括号
idata unsigned char paren_right = 0;//右括号
idata float value = 0;//临时输入值
idata unsigned char type = 0;//字符验证
bit type_ok = 0;
bit range_ok = 0;
bit type_error = 0;
idata unsigned int hour_temp,min_temp,sec_temp = 0;//时间设置

//温度
idata unsigned int temp_cnt = 0;
idata float temp_val = 0;
idata float Weight = 0;


//超声波
idata unsigned char dis_cnt = 0;
double distance = 0.0;

//频率
long freq_val = 0;
long freq_old_val = 0;
long freq_prev_val = 0;
long freq_change_base = 0;
xdata unsigned int Warning_Value = 0;
xdata float Warning_Level = 0;
unsigned int freq = 0;
idata unsigned int freq_cnt = 0;
idata unsigned char trend_count = 0;
idata unsigned char change_count = 0;
idata signed char trend_dir = 0;
bit freq_init = 0;

//液面数据
idata float Height = 1.0;//柱形、长方体容器高度
idata float r = 1.0;//球体半径
idata float h = 0.0;//液位高度
idata float W = 1.0;
idata float L = 1.0;//长
idata float Volumn = 0.0;
idata float Percentage = 0;

//参数
xdata float H1 = 1.0;//液位上限参数
xdata float H2 = 0.1;//液位下限参数
xdata unsigned int F = 2000;//频率参数

//标定界面
xdata unsigned int mark_mode = 0;
xdata float mark[5] = 0.0;//五点标定
xdata float mark_temp[5] = 0;//临时变量

//容器
idata unsigned char S = 2;//容器类型：0为球，1为柱，2为长

//ds1302
idata unsigned char rtc_cnt = 0;
idata unsigned int hour,min,sec = 0;

//重量（ADC）
idata float Voltage_raw = 0;
idata float Voltage = 0;
idata unsigned char adc_cnt = 0;

//异常类型
xdata unsigned int Warning_Num = 0;
xdata unsigned int Warning_Last = 0;
xdata unsigned char Warning_Clear_Cnt = 0;
bit trend = 0;//0为连续下降，1为连续上升
/*500: 超过液位上限
	501：低于液位下限
	502：频率数值异常
	503：频率趋势异常
	504：频率剧烈变化
*/

//LED
xdata unsigned char LedControl = 0xff;
xdata unsigned char led_cnt = 0;
xdata unsigned char shine_cnt1 = 0;
xdata unsigned char shine_cnt2 = 0;
xdata unsigned long ulms = 0;
xdata unsigned long campture_time = 0;
bit Led_shine_flag1 = 0;
bit Led_shine_flag2 = 0;

//继电器
xdata unsigned char ralay_cnt = 0;

void SmgTask();
void KbtTask();
void UartTask();
void TempTask();
void SonicTask();
void RtcTask();
void AdcTask();
void Ne555Task();
void FreqWarnTask();
void LiquidTask();//液面任务
void ExamMarkTask();//检验标定界面
void LedTask();
void RalayTask();
float GetTankHeight();

void main()
{
	DeviceCtrl(0x80,0xff);
	DeviceCtrl(0xa0,0x80);
	mark[0] = 0;mark[1] = 1.0;mark[2] = 2.0;mark[3] = 4.0;mark[4] = 5.0; //默认标定值
	SetClock(23,59,50);
	do
	{
		temp_val = ReadTemprature();
	}while(temp_val == 85.00);
	Timer0_Init();
	PcaInit();
	Uart1_Init();
	Timer1_Init();
	EA = 1;
	while(1)
	{
		UartTask();
		SmgTask();
		KbtTask();
		TempTask();
		SonicTask();
		RtcTask();
		AdcTask();
		Ne555Task();
		RalayTask();
		LedTask();
	}	
}

float GetTankHeight()
{
	if(S == 0)
		return 2.0 * r;
	return Height;
}

void PcaHandler() interrupt 7
{
	SmgScanTask();
	adc_cnt++;
	kbt_cnt++;
	uart_cnt++;
	temp_cnt++;
	dis_cnt++;
	rtc_cnt++;
	freq_cnt++;
	ralay_cnt++;
	led_cnt++;
	ulms++;
	if(Warning_Last == 503)
	{
		shine_cnt1++;
		if(shine_cnt1>=100)
		{
			shine_cnt1 = 0;
			Led_shine_flag1^= 1;
		}
	}
	else
	{
		shine_cnt1 = 0;
		Led_shine_flag1 = 0;
	}
	if(campture_time != 0 && ulms - campture_time < 3000)
	{
		shine_cnt2++;
		if(shine_cnt2>=200)
		{
			shine_cnt2 = 0;
			Led_shine_flag2 ^= 1;
		}
	}
	else
	{
		shine_cnt2 = 0;
		Led_shine_flag2 = 0;
	}
	CH = (65535 - 1000)>>8;
	CL = 65535 - 1000;
	CF = 0;
}

void RalayTask()
{
	if(Warning_Last)
		DeviceCtrl(0xa0,0x90);
	else
		DeviceCtrl(0xa0,0x80);
}

void LedTask()
{
	if(led_cnt < 20)
		return;
	led_cnt = 0;
	LedControl = 0xff;
	switch(data_mode)
	{
		case 0:LedControl &= ~0x01;break;
		case 1:LedControl &= ~0x02;break;
		case 2:LedControl &= ~0x04;break;
		case 3:LedControl &= ~0x08;break;
		default:break;
	}
	if(Warning_Last == 500 || Warning_Last == 501)
		LedControl &= ~0x10;
	else if(Warning_Last == 502)
	{
		LedControl &= ~0x20;
	}
	else if(Warning_Last == 503)
	{
		if(Led_shine_flag1 == 0)
			LedControl &= ~0x40;
	}
	if(campture_time != 0 && ulms - campture_time < 3000)
	{
		if(Led_shine_flag2 == 0)
			LedControl &= ~0x80;
	}
	DeviceCtrl(0x80,LedControl);
}
void UartTask()
{
    // ==================== 局部变量定义 ====================
    idata unsigned char i = 0;              // 循环计数器
    idata unsigned char end = 0;            // 当前命令 ')' 的位置
    idata unsigned char id_len = 0;         // 参数名的长度（如"H1"=2）
    idata unsigned char value_start = 0;    // 数值部分的起始索引
    idata unsigned char value_len = 0;      // 数值部分的长度
    idata unsigned char dot_seen = 0;       // 小数点是否已出现（用于小数解析）
    idata unsigned char new_pos = 0;        // 移动缓冲区时的新位置
    idata float decimal_base = 0.1;         // 小数位的权重（十分位、百分位...）
    idata char id[3];                       // 存储参数名（最多2字符+结束符）

    // ==================== 第一步：检测新数据到达 ====================
    // uart_pos: 串口接收中断中累加，表示buffer中已有多少字节
    // uart_last_pos: 上次处理时的字节数，两者不等说明有新数据
    if(uart_pos != uart_last_pos)
    {
        uart_last_pos = uart_pos;           // 更新上次位置
        uart_cnt = 0;                       // 重置空闲计数器
        if(uart_pos != 0)                   // 如果有数据，清空待发送的异常
            Warning_Num = 0;                // 避免异常响应和命令响应冲突
    }

    // ==================== 第二步：空闲状态处理 ====================
    // uart_pos == 0 表示当前没有待处理的串口数据
    if(uart_pos == 0)
    {
        // ----- 2.1 发送配置命令的响应（OK/ERROR/WARN）-----
        // uart_config_active: 是否收到过配置命令（至少解析了一条）
        // uart_cnt > 15: 空闲15ms后发送响应（给上位机时间收完所有数据）
        if(uart_config_active && uart_cnt > 15)
        {
            // 根据成功/失败数量决定返回什么
            if(uart_invalid_cnt == 0)
                SendString("OK");           // 全部参数合法 → OK
            else if(uart_valid_cnt == 0)
                SendString("ERROR");        // 全部参数非法 → ERROR
            else
                SendString("WARN");         // 部分合法 → WARN
            
            // 清空所有响应相关状态
            uart_valid_cnt = 0;
            uart_invalid_cnt = 0;
            uart_config_active = 0;
            Warning_Num = 0;                // 同时清空异常（避免冲突）
            uart_cnt = 0;
            return;
        }
        
        // ----- 2.2 发送异常主动上报 -----
        // Warning_Num != 0: 有异常待上报
        // uart_cnt > 20: 空闲20ms后发送（给命令响应让路）
        if(Warning_Num != 0 && uart_cnt > 20)
        {
            // 根据异常类型格式化不同的上报字符串
            if(Warning_Num == 500)
                sprintf((char*)uart_buffer,"[500,%02d%02d%02d,%.1f]",hour,min,sec,Warning_Level);
            else if(Warning_Num == 501)
                sprintf((char*)uart_buffer,"[501,%02d%02d%02d,%.1f]",hour,min,sec,Warning_Level);
            else if(Warning_Num == 502)
                sprintf((char*)uart_buffer,"[502,%02d%02d%02d,%u]",hour,min,sec,Warning_Value);
            else if(Warning_Num == 503)
                sprintf((char*)uart_buffer,"[503,%02d%02d%02d,%u]",hour,min,sec,(unsigned int)trend);
            else if(Warning_Num == 504)
                sprintf((char*)uart_buffer,"[504,%02d%02d%02d,%u]",hour,min,sec,Warning_Value);
            
            SendString(uart_buffer);        // 发送异常上报
            Warning_Num = 0;                // 清空，避免重复发送
        }
        return;                             // 空闲状态处理完毕
    }

    // ==================== 第三步：超时保护 ====================
    // 如果超过30ms还没收到完整的 ')'，说明数据出错，清空缓冲区
    if(uart_cnt > 30)
    {
        SendString("ERROR");                // 通知上位机出错
        uart_pos = 0;                       // 清空缓冲区
        uart_last_pos = 0;
        uart_valid_cnt = 0;
        uart_invalid_cnt = 0;
        uart_config_active = 0;
        return;
    }

    // ==================== 第四步：循环解析命令 ====================
    // 缓冲区可能有多个命令，如 (H1,2.3)(H2,0.5)，逐个处理
    while(uart_pos != 0)
    {
        // ----- 4.1 检查命令格式：必须以 '(' 开头 -----
        if(uart_buffer[0] != '(')
        {
            SendString("ERROR");
            uart_pos = 0;
            uart_last_pos = 0;
            uart_valid_cnt = 0;
            uart_invalid_cnt = 0;
            uart_config_active = 0;
            return;
        }
        
        // ----- 4.2 找到当前命令的结束符 ')' -----
        end = 1;
        while(end < uart_pos && uart_buffer[end] != ')')
            end++;
        if(end >= uart_pos)
            return;                         // 没找到，等下一批数据
        
        // ----- 4.3 特殊处理：时间设置命令 (T:HHMMSS) -----
        if(uart_buffer[1] == 'T')
        {
            // 格式检查：必须是 (T:140850) 共9个字符，第2个是':'
            if(end == 9 && uart_buffer[2] == ':')
            {
                // 验证6个数字是否都是数字字符
                for(i = 3; i <= 8; i++)
                {
                    if(uart_buffer[i] < '0' || uart_buffer[i] > '9')
                    {
                        SendString("ERROR");
                        uart_pos = 0;
                        uart_last_pos = 0;
                        return;
                    }
                }
                // 解析时分秒（字符转数字）
                hour_temp = (uart_buffer[3] - '0') * 10 + (uart_buffer[4] - '0');
                min_temp  = (uart_buffer[5] - '0') * 10 + (uart_buffer[6] - '0');
                sec_temp  = (uart_buffer[7] - '0') * 10 + (uart_buffer[8] - '0');
                
                // 验证时间合法性
                if(hour_temp <= 23 && min_temp <= 59 && sec_temp <= 59)
                {
                    SetClock(hour_temp, min_temp, sec_temp);  // 设置DS1302时钟
                    hour = hour_temp;
                    min = min_temp;
                    sec = sec_temp;
                    SendString("OK");
                }
                else
                    SendString("ERROR");
            }
            else
                SendString("ERROR");
            
            // 时间命令处理完毕，清空缓冲区
            uart_pos = 0;
            uart_last_pos = 0;
            return;
        }
        
        // ----- 4.4 普通参数命令：找参数名和值的分隔符 ',' -----
        comma_index = 0;
        for(i = 1; i < end; i++)
        {
            if(uart_buffer[i] == ',')
            {
                if(comma_index == 0)
                    comma_index = i;        // 记录第一个逗号的位置
                else
                {
                    // 出现第二个逗号 → 格式错误
                    SendString("ERROR");
                    uart_pos = 0;
                    uart_last_pos = 0;
                    uart_valid_cnt = 0;
                    uart_invalid_cnt = 0;
                    uart_config_active = 0;
                    return;
                }
            }
        }
        if(comma_index == 0)                // 没有逗号 → 格式错误
        {
            SendString("ERROR");
            uart_pos = 0;
            uart_last_pos = 0;
            uart_valid_cnt = 0;
            uart_invalid_cnt = 0;
            uart_config_active = 0;
            return;
        }
        
        // ----- 4.5 提取参数名（如 "H1"）-----
        id_len = comma_index - 1;           // 参数名的长度
        if(id_len == 0 || id_len > 2)       // 参数名必须是1或2个字符
        {
            SendString("ERROR");
            // ... 清空状态
            return;
        }
        for(i = 0; i < id_len; i++)
            id[i] = uart_buffer[1 + i];     // 从buffer[1]开始拷贝（跳过'('）
        id[id_len] = '\0';                  // 字符串结束符
        
        // ----- 4.6 确定数值部分的起止 -----
        value_start = comma_index + 1;      // 数值开始位置（逗号后）
        value_len = end - value_start;      // 数值长度
        
        if(value_len == 0)                  // 没有数值 → 格式错误
        {
            SendString("ERROR");
            // ... 清空状态
            return;
        }
        
        // ----- 4.7 查询命令处理 (H1,?) -----
        if(value_len == 1 && uart_buffer[value_start] == '?')
        {
            // 根据参数名返回当前值
            if(strcmp(id,"H1") == 0)//strcmp() == string compare
                sprintf((char*)uart_buffer,"(H1,%.1f)",H1);
            else if(strcmp(id,"H2") == 0)
                sprintf((char*)uart_buffer,"(H2,%.1f)",H2);
            else if(strcmp(id,"F") == 0)
                sprintf((char*)uart_buffer,"(F,%d)",F);
            else if(strcmp(id,"S") == 0)
                sprintf((char*)uart_buffer,"(S,%d)",(int)S);
            else if(strcmp(id,"r") == 0)
                sprintf((char*)uart_buffer,"(r,%.1f)",r);
            else if(strcmp(id,"L") == 0)
                sprintf((char*)uart_buffer,"(L,%.1f)",L);
            else if(strcmp(id,"W") == 0)
                sprintf((char*)uart_buffer,"(W,%.1f)",W);
            else if(strcmp(id,"H") == 0)
                sprintf((char*)uart_buffer,"(H,%.1f)",Height);
            else
            {
                SendString("ERROR");
                uart_pos = 0;
                uart_last_pos = 0;
                return;
            }
            SendString(uart_buffer);        // 返回查询结果
            uart_pos = 0;                   // 清空缓冲区
            uart_last_pos = 0;
            return;
        }
        
        // ----- 4.8 数值解析（支持整数和小数，如 2.3）-----
        type_ok = 0;        // 参数名是否有效
        range_ok = 0;       // 数值是否在合理范围内
        value = 0;          // 存放解析出的数值
        dot_seen = 0;       // 是否已遇到小数点
        decimal_base = 0.1; // 小数位权重（十分位=0.1，百分位=0.01...）
        
        for(i = value_start; i < end; i++)
        {
            if(uart_buffer[i] == '.')
            {
                if(dot_seen)                // 两个小数点 → 错误
                {
                    SendString("ERROR");
                    // ... 清空状态
                    return;
                }
                dot_seen = 1;
            }
            else if(uart_buffer[i] >= '0' && uart_buffer[i] <= '9')
            {
                if(!dot_seen)
                {
                    // 整数部分：123 → 1→12→123
                    value = value * 10 + (uart_buffer[i] - '0');
                }
                else
                {
                    // 小数部分：0.34 → 0.0 + 0.3 = 0.3，然后 + 0.04 = 0.34
                    value = value + (uart_buffer[i] - '0') * decimal_base;
                    decimal_base = decimal_base * 0.1;  // 权重递减：0.1→0.01→0.001
                }
            }
            else
            {
                // 非法字符
                SendString("ERROR");
                // ... 清空状态
                return;
            }
        }
        
        // ----- 4.9 根据参数名校验并保存数值 -----
        if(strcmp(id,"H1") == 0)
        {
            type_ok = 1;
            range_ok = (value > H2);        // H1必须大于H2
            if(range_ok)
            {
                H1 = value;
                uart_valid_cnt++;           // 合法参数计数+1
            }
        }
        else if(strcmp(id,"H2") == 0)
        {
            type_ok = 1;
            range_ok = (H1 > value);        // H2必须小于H1
            if(range_ok)
            {
                H2 = value;
                uart_valid_cnt++;
            }
        }
        else if(strcmp(id,"F") == 0)
        {
            type_ok = 1;
            range_ok = (!dot_seen && value > 0);  // F必须是整数且>0
            if(range_ok)
            {
                F = (unsigned int)value;
                uart_valid_cnt++;
            }
        }
        else if(strcmp(id,"S") == 0)
        {
            type_ok = 1;
            range_ok = (!dot_seen && (value == 0 || value == 1 || value == 2));  // 0/1/2
            if(range_ok)
            {
                S = value;
                uart_valid_cnt++;
            }
        }
        else if(strcmp(id,"r") == 0)
        {
            type_ok = 1;
            range_ok = (value > 0);
            if(range_ok)
            {
                r = value;
                uart_valid_cnt++;
            }
        }
        else if(strcmp(id,"L") == 0)
        {
            type_ok = 1;
            range_ok = (value > 0);
            if(range_ok)
            {
                L = value;
                uart_valid_cnt++;
            }
        }
        else if(strcmp(id,"W") == 0)
        {
            type_ok = 1;
            range_ok = (value > 0);
            if(range_ok)
            {
                W = value;
                uart_valid_cnt++;
            }
        }
        else if(strcmp(id,"H") == 0)
        {
            type_ok = 1;
            range_ok = (value > 0);
            if(range_ok)
            {
                Height = value;
                uart_valid_cnt++;
            }
        }
        
        // 参数名无效
        if(!type_ok)
        {
            SendString("ERROR");
            // ... 清空状态
            return;
        }
        
        // 参数名有效但数值范围无效
        if(!range_ok)
            uart_invalid_cnt++;
        
        // 至少有一条合法参数，标记有待响应
        uart_config_active = 1;
        
        // ----- 4.10 移走已处理的命令，继续解析下一条 -----
        ES = 0;                             // 关串口中断，防止干扰
        new_pos = 0;
        // 将当前命令之后的数据移到缓冲区开头
        for(i = end + 1; i < uart_pos; i++)
            uart_buffer[new_pos++] = uart_buffer[i];
        uart_pos = new_pos;                 // 更新缓冲区长度
        uart_last_pos = uart_pos;
        ES = 1;                             // 恢复串口中断
        
        // 继续循环，处理下一条命令
    }
}

void TempTask()
{
	if(temp_cnt<=750)
		return;
	temp_cnt = 0;
	temp_val = ReadTemprature();
	if(temp_val > 2048)
		temp_val -= 4096;
}

void RtcTask()
{
	if(rtc_cnt<=201)
		return;
	rtc_cnt = 0;
	hour = BcdToDec(Read_Ds1302_Byte(0x85));
	min  = BcdToDec(Read_Ds1302_Byte(0x83));
	sec =  BcdToDec(Read_Ds1302_Byte(0x81));
}

void SonicTask()
{
	if(dis_cnt<=202)
		return;
	dis_cnt = 0;
	distance = WaveRcvd();
	if(temp_val < 0)
		distance = distance * (330 + 0.6 * temp_val) / (330 + 0.6 * (temp_val + 4096));
	LiquidTask();
}

void LiquidTask()
{
	idata float tank_height = 0;

	tank_height = GetTankHeight();
	if(distance < 0)
		distance = 0;
	if(distance > tank_height)
		distance = tank_height;
	h = tank_height - distance;
	if(h < 0)
		h = 0;

	switch(S)
	{
		case 0://球形
			Volumn = 3.14*h*h*(3*r - h)/3.0;
		break;
		case 1://柱形
			Volumn = 3.14*r*r*h;
		break;
		case 2://长方形
			Volumn = L*W*h;
		break;
	}
	if(S == 0)
	{
		Percentage = 1 - Volumn / (4.0 * 3.14 * r * r * r / 3.0);
		if(Percentage < 0)Percentage = 0;
		else if(Percentage > 1)Percentage = 1;
	}
	else if(tank_height > 0)
		Percentage = distance / tank_height;
	else
		Percentage = 0;
}

void Ne555Task()
{
	idata unsigned int gate_ms = 0;
	if(freq_cnt < 1000)
		return;
	gate_ms = freq_cnt;
	freq_cnt = 0;
	TR0 = 0;
	freq = (TH0<<8|TL0);
	if(gate_ms == 0)
		freq_val = freq;
	else
		freq_val = (unsigned long)freq * 1000 / gate_ms;
	TH0 = TL0 = 0;
	TR0 = 1;
	FreqWarnTask();
}

void FreqWarnTask()
{
	idata unsigned int current_warning = 0;
	idata long diff = 0;
	idata long change_diff = 0;
	idata unsigned char trend_alarm = 0;
	idata unsigned char change_alarm = 0;

	if(!freq_init)
	{
		freq_prev_val = freq_val;
		freq_change_base = freq_val;
		freq_init = 1;
	}

	diff = freq_val - freq_prev_val;
	if(diff > 0)
	{
		if(trend_dir == 1)
		{
			if(trend_count < 5)
				trend_count++;
		}
		else
		{
			trend_dir = 1;
			trend_count = 1;
		}
		trend = 1;
	}
	else if(diff < 0)
	{
		if(trend_dir == -1)
		{
			if(trend_count < 5)
				trend_count++;
		}
		else
		{
			trend_dir = -1;
			trend_count = 1;
		}
		trend = 0;
	}
	else
	{
		trend_dir = 0;
		trend_count = 0;
	}
	if(trend_count >= 4)
		trend_alarm = 1;

	change_diff = freq_val - freq_change_base;
	if(change_diff < 0)
		change_diff = -change_diff;
	change_count++;
	if(change_diff > 1000)
	{
		change_alarm = 1;//各个异常值都需要单独设置一个异常标志位
		change_count = 0;
		freq_change_base = freq_val;
	}
	else if(change_count >= 2)
	{
		change_count = 0;
		freq_change_base = freq_val;
	}
	freq_prev_val = freq_val;

	if(h>H1)
		current_warning = 500;
	else if(h<H2)
		current_warning = 501;
	else if(change_alarm)
		current_warning = 504;
	else if(trend_alarm)
		current_warning = 503;
	else if(freq_val>F)
		current_warning = 502;

	if(current_warning == 0)
	{
		Warning_Last = 0;
		Warning_Num = 0;
		Warning_Clear_Cnt = 0;
	}
	else
	{
		Warning_Clear_Cnt = 0;
		if(current_warning != Warning_Last)
		{
			Warning_Num = current_warning;
			uart_cnt = 0;
			Warning_Last = current_warning;
			if(current_warning == 503)
			{
				shine_cnt1 = 0;
				Led_shine_flag1 = 0;
			}
			if(current_warning == 500 || current_warning == 501)
				Warning_Level = h;
			else if(current_warning == 502)
				Warning_Value = (unsigned int)freq_val;
			else if(current_warning == 504)
			{
				Warning_Value = (unsigned int)change_diff;
				campture_time = ulms;
				shine_cnt2 = 0;
				Led_shine_flag2 = 0;
			}
		}
	}
}
void AdcTask()
{
	if(adc_cnt<=101)
		return;
	adc_cnt = 0;
	Voltage_raw = PCF8591_ADC()/51.0;
	Voltage = Voltage_raw + (0.04*temp_val*temp_val - 10*temp_val + 225)*0.001;
	if(Voltage>5)Voltage = 5;
	else if(Voltage<0)Voltage = 0;
	if(Voltage <= mark[0])
		Weight = 0;
	else if(Voltage <= mark[1])
		Weight = (Voltage - mark[0]) * 5 / (mark[1] - mark[0]);
	else if(Voltage <= mark[2])
		Weight = 5 + (Voltage - mark[1]) * 5 / (mark[2] - mark[1]);
	else if(Voltage <= mark[3])
		Weight = 10 + (Voltage - mark[2]) * 5 / (mark[3] - mark[2]);
	else if(Voltage <= mark[4])
		Weight = 15 + (Voltage - mark[3]) * 5 / (mark[4] - mark[3]);
	else
		Weight = 20;
	ExamMarkTask();
}

void ExamMarkTask()
{
}

void KbtTask()
{
	if(kbt_cnt<=20)
		return;
	kbt_cnt = 0;
	KbtThree();
	if(Ktrg == 0x88)
	{
		Display_Mode = (Display_Mode+1)%3;
		if(Display_Mode == 0)
			data_mode = 0;
		else if(Display_Mode == 1)
			para_mode = 0;
		else
		{
			mark_mode = 0;
		}
	}
	if(Ktrg == 0x84)//S5
	{
		switch(Display_Mode)
		{
			case 0:data_mode = (data_mode + 1)%4;break;
			case 1:para_mode = (para_mode + 1)%8;break;
			case 2:mark_mode = (mark_mode + 1)%5;break;
			default:break;
		}
	}
	if(Ktrg == 0x48 && Display_Mode == 2)//S8做加法
	{
		switch(mark_mode)
		{
			case 0:if(mark[0] + 0.1 < mark[1])mark[0]+=0.1;break;
			case 1:if(mark[1] + 0.1 < mark[2])mark[1]+=0.1;break;
			case 2:if(mark[2] + 0.1 < mark[3])mark[2]+=0.1;break;
			case 3:if(mark[3] + 0.1 < mark[4])mark[3]+=0.1;break;
			case 4:if(mark[4] < 9.9)mark[4]+=0.1;break;
			default:break;
		}
	}
	if(Ktrg == 0x44 && Display_Mode == 2)//S9做减法
	{
		switch(mark_mode)
		{
			case 4:if(mark[4] - 0.1 > mark[3])mark[4]-=0.1;break;
			case 3:if(mark[3] - 0.1 > mark[2])mark[3]-=0.1;break;
			case 2:if(mark[2] - 0.1 > mark[1])mark[2]-=0.1;break;
			case 1:if(mark[1] - 0.1 > mark[0])mark[1]-=0.1;break;
			case 0:if(mark[0] > 0)mark[0]-=0.1;break;
			default:break;
		}
	}
}

void SmgTask()
{
	if(Display_Mode == 0)//数据界面
	{
		switch(data_mode)
		{
			case 0:sprintf(DisplayChars,"%02d-%02d-%02d",hour,min,sec);break;//时间显示
			case 1:sprintf(DisplayChars," %3d-%4.2f",(int)temp_val,h);break;//液位显示
			case 2:sprintf(DisplayChars,"%3d-%5.1f",(int)(Percentage*100),Volumn);break;//体积显示
			case 3:sprintf(DisplayChars,"%3d-%5.1f",(int)temp_val,Weight);break;
		}
		
	}
	else if(Display_Mode == 1)//参数界面
	{
		switch(para_mode)
		{
			case 0:sprintf(DisplayChars,"P1-%6.1f",H1);break;
			case 1:sprintf(DisplayChars,"P2-%6.1f",H2);break;
			case 2:sprintf(DisplayChars,"P3-%5d",F);break;
			case 3:sprintf(DisplayChars,"P4-%5d",(int)S);break;
			case 4:sprintf(DisplayChars,"P5-%6.1f",r);break;
			case 5:sprintf(DisplayChars,"P6-%6.1f",L);break;
			case 6:sprintf(DisplayChars,"P7-%6.1f",W);break;
			case 7:sprintf(DisplayChars,"P8-%6.1f",Height);break;
			default:break;
			
		}

	}
	else
	{
		switch(mark_mode)
		{
			case 0:sprintf(DisplayChars,"E00   %3.1f",mark[0]);break;
			case 1:sprintf(DisplayChars,"E05   %3.1f",mark[1]);break;
			case 2:sprintf(DisplayChars,"E10   %3.1f",mark[2]);break;
			case 3:sprintf(DisplayChars,"E15   %3.1f",mark[3]);break;
			case 4:sprintf(DisplayChars,"E20   %3.1f",mark[4]);break;
			default:break;
		}
	}
	
	SmgUpdate();
}



