#include <string.h> 
#include "stm32f10x.h"
#include "Delay.h"
#include "LED.h"
#include "lze_lcd.h"
#include "usart.h"	
#include "ADS1292.h"	
#include "spi.h"
#include "EXTInterrupt.h"
#include "PeripheralInit.h"
#include "arm_math.h"
#include "max30102.h"
#include "OLED.h"
//-----------------------------------------------------------------
// 主程序
//-----------------------------------------------------------------
/************************************心率血氧********************************************/
#define MAX_BRIGHTNESS 255
#define INTERRUPT_REG 0X00
uint32_t aun_ir_buffer[500]; 	 //IR LED   红外光数据，用于计算血氧
int32_t n_ir_buffer_length;    //数据长度
uint32_t aun_red_buffer[500];  //Red LED	红光数据，用于计算心率曲线以及计算心率
int32_t n_sp02; //SPO2值
int8_t ch_spo2_valid;   //用于显示SP02计算是否有效的指示符
int32_t n_heart_rate;   //心率值
int8_t  ch_hr_valid;    //用于显示心率计算是否有效的指示符
uint8_t Temp;
uint32_t un_min, un_max, un_prev_data;  
int i;
int32_t n_brightness;
float f_temp;
u8 temp[6];
u8 str[100];
u8 dis_hr=0,dis_spo2=0;
// 全局变量：记录MAX30102样本处理索引（替代原大循环）
u16 max_sample_idx = 0; 
u8 calc_hr_spo2_flag = 0; // HR/SPO2计算标志位

// ========== HR/SPO2非0值统计变量 ==========
uint32_t hr_sum = 0;        // HR非0值累计和
uint16_t hr_count = 0;      // HR非0值有效计数
uint32_t spo2_sum = 0;      // SPO2非0值累计和
uint16_t spo2_count = 0;    // SPO2非0值有效计数
float hr_avg = 0.0f;        // HR平均值
float spo2_avg = 0.0f;      // SPO2平均值
// ===============================================
u8 uart_rx_buf[UART_RX_BUF_SIZE];  // 接收缓冲区
u8 uart_rx_head = 0;              // 缓冲区头指针
u8 uart_rx_tail = 0;              // 缓冲区尾指针
u8 uart_rx_flag = 0;              // 接收完成标志（换行符触发）
u8 display_str[17] = {0};         // OLED最后一行显示字符串（128/8=16个字符）

// ========== OH风险等级处理相关 ==========
#define OH_RISK_LEVEL_COUNT 5
const u8* oh_risk_level_en[] = {
    "Insufficient Data",    // 0 - 数据不足
    "Normal BP",            // 1 - 正常
    "Mild Hypotension",     // 2 - 轻度低血压
    "Moderate Hypotension", // 3 - 中度低血压
    "Severe Hypotension"    // 4 - 重度低血压
};

u8 oh_risk_level = 0;        // 当前OH风险等级（0-4）
u8 oh_update_flag = 0;       // OH风险等级更新标志
u8 oh_display_str[17] = {0}; // OH显示字符串

/****************************************************************************************/
// 处理串口接收的OH风险等级数据
void uart_oh_data_process(void)
{
	int i = 0;
    if(uart_rx_flag == 1)
    {
        u8 risk_level = 0;
        u8 temp_buffer[UART_RX_BUF_SIZE];
        u8 temp_idx = 0;
        
        // 读取缓冲区中的所有数据
        while(uart_rx_head != uart_rx_tail)
        {
            u8 ch = uart_rx_buf[uart_rx_head];
            temp_buffer[temp_idx++] = ch;
            uart_rx_head = (uart_rx_head + 1) % UART_RX_BUF_SIZE;
            
            if(temp_idx >= UART_RX_BUF_SIZE - 1) break;
        }
        temp_buffer[temp_idx] = '\0'; // 添加结束符
        
        // 查找"RISK:"前缀
        for(i = 0; i < temp_idx - 2; i++)
        {
            if(temp_buffer[i] == 'R' && 
               temp_buffer[i+1] == 'I' && 
               temp_buffer[i+2] == 'S' && 
               temp_buffer[i+3] == 'K' && 
               temp_buffer[i+4] == ':')
            {
                if(i + 5 < temp_idx)
                {
                    // 提取风险等级数字
                    char risk_char = temp_buffer[i + 5];
                    if(risk_char >= '0' && risk_char <= '4')
                    {
                        risk_level = risk_char - '0';
                        oh_risk_level = risk_level;
                        oh_update_flag = 1; // 设置更新标志
                        
                        // 复制对应的显示字符串
                        if(risk_level < OH_RISK_LEVEL_COUNT)
                        {
                            strncpy((char*)oh_display_str, (char*)oh_risk_level_en[risk_level], 16);
                            oh_display_str[16] = '\0'; // 确保以空字符结尾
                        }
                        break;
                    }
                }
            }
        }
        
        // 清空标志
        uart_rx_flag = 0;
    }
}

// 显示OH风险等级在OLED上
void display_oh_risk_level(void)
{
    if(oh_update_flag)
    {
        // 清除风险等级显示区域（第4行）
        OLED_ShowString(4, 1, "                ");
        
        // 显示风险等级（第4行，第1列开始）
        OLED_ShowString(4, 1, (char*)oh_display_str);
        
        // 可选：在第5行显示风险等级数字
        OLED_ShowString(5, 1, "Risk Level: ");
        OLED_ShowNum(5, 13, oh_risk_level, 1);
        
        oh_update_flag = 0; // 清除更新标志
    }
}

// 显示心率血氧标题（英文）
void cimeifen(void)
{
    OLED_Clear(); // 初始化时清屏
    // 第1行：标题
    OLED_ShowString(1, 1, "HR/SPO2 Monitor");
    
    // 第2行，第1列：固定显示"HR:"
    OLED_ShowString(2, 1, "HR:");
    // 第2行，第8列：固定显示"BPM"（心率单位）
    OLED_ShowString(2, 8, "BPM");
    
    // 第3行，第1列：固定显示"SPO2:"
    OLED_ShowString(3, 1, "SPO2:");
    // 第3行，第8列：固定显示"%"（血氧单位）
    OLED_ShowString(3, 8, "%");
    
    // 第4行：OH风险等级标题
    OLED_ShowString(4, 1, "OH Risk:");
    
    // 第5行：风险等级值（初始为空）
    OLED_ShowString(5, 1, "                ");
}

// 显示等待测量提示（英文）
void Wait(void)
{
    // 第6行，第4列：显示"Measuring..."
    OLED_ShowString(1, 2, "Measuring...");
}

// 心率血氧测量主函数
void welcome(void)
{
    cimeifen();
    Wait(); // 显示等待测量（英文）
    
    while(1)
    {
        // 原有的MAX30102数据处理逻辑...
        // 这里简化为一个示例循环
        Delay_1ms(100);
    }
}

u32 ch1_data;		// 通道1的数据
u32 ch2_data;		// 通道2的数据
u8 flog;				// 触发中断标志位
u16 point_cnt;	// 两个峰值之间的采集点数，用于计算心率
u32 BPM_LH[3];
#define Samples_Number  1    											// 采样点数
#define Block_Size      1     										// 调用一次arm_fir_f32处理的采样点个数
#define NumTaps        	129     									// 滤波器系数个数

uint32_t blockSize = Block_Size;									// 调用一次arm_fir_f32处理的采样点个数
uint32_t numBlocks = Samples_Number/Block_Size;   // 需要调用arm_fir_f32的次数

float32_t Input_data1; 														// 输入缓冲区
float32_t Output_data1;         									// 输出缓冲区
float32_t firState1[Block_Size + NumTaps - 1]; 		// 状态缓存，大小numTaps + blockSize - 1
float32_t Input_data2; 														// 输入缓冲区
float32_t Output_data2;         									// 输出缓冲区
float32_t firState2[Block_Size + NumTaps - 1]; 		// 状态缓存，大小numTaps + blockSize - 1

// 心电带通滤波器系数：采样频率为250Hz，截止频率为5Hz~40Hz 通过filterDesigner获取
const float32_t BPF_5Hz_40Hz[NumTaps]  = {
  3.523997657e-05,0.0002562592272,0.0005757701583,0.0008397826459, 0.000908970891,
  0.0007304374012,0.0003793779761,4.222582356e-05,-6.521392788e-05,0.0001839015895,
  0.0007320778677, 0.001328663086, 0.001635892317, 0.001413777587,0.0006883906899,
  -0.0002056905651,-0.0007648666506,-0.0005919140531,0.0003351111372, 0.001569915912,
   0.002375603188, 0.002117323689,0.0006689901347,-0.001414557919,-0.003109993879,
  -0.003462586319, -0.00217742566,8.629632794e-05, 0.001947802957, 0.002011778764,
  -0.0002987752669,-0.004264956806, -0.00809297245,-0.009811084718,-0.008411717601,
  -0.004596390296,-0.0006214127061,0.0007985962438,-0.001978532877,-0.008395017125,
   -0.01568987407, -0.02018531598, -0.01929843985, -0.01321159769,-0.005181713495,
  -0.0001112028476,-0.001950757345, -0.01125541423,  -0.0243169684, -0.03460548073,
   -0.03605531529, -0.02662901953, -0.01020727865, 0.004513713531, 0.008002913557,
  -0.004921500571, -0.03125274926, -0.05950148031, -0.07363011688, -0.05986980721,
   -0.01351031102,  0.05752891302,   0.1343045086,   0.1933406889,   0.2154731899,
     0.1933406889,   0.1343045086,  0.05752891302, -0.01351031102, -0.05986980721,
   -0.07363011688, -0.05950148031, -0.03125274926,-0.004921500571, 0.008002913557,
   0.004513713531, -0.01020727865, -0.02662901953, -0.03605531529, -0.03460548073,
    -0.0243169684, -0.01125541423,-0.001950757345,-0.0001112028476,-0.005181713495,
   -0.01321159769, -0.01929843985, -0.02018531598, -0.01568987407,-0.008395017125,
  -0.001978532877,0.0007985962438,-0.0006214127061,-0.004596390296,-0.008411717601,
  -0.009811084718, -0.00809297245,-0.004264956806,-0.0002987752669, 0.002011778764,
   0.001947802957,8.629632794e-05, -0.00217742566,-0.003462586319,-0.003109993879,
  -0.001414557919,0.0006689901347, 0.002117323689, 0.002375603188, 0.001569915912,
  0.0003351111372,-0.0005919140531,-0.0007648666506,-0.0002056905651,0.0006883906899,
   0.001413777587, 0.001635892317, 0.001328663086,0.0007320778677,0.0001839015895,
  -6.521392788e-05,4.222582356e-05,0.0003793779761,0.0007304374012, 0.000908970891,
  0.0008397826459,0.0005757701583,0.0002562592272,3.523997657e-05
};

// 呼吸波低通滤波器系数：采样频率为250Hz，截止频率为2Hz 通过filterDesigner获取
const float32_t LPF_2Hz[NumTaps]  = {
  -0.0004293085367,-0.0004170549801,-0.0004080719373,-0.0004015014856,-0.0003963182389,
  -0.000391335343,-0.0003852125083,-0.0003764661378,-0.0003634814057,-0.0003445262846,
  -0.0003177672043,-0.0002812864841,-0.0002331012802,-0.0001711835939,-9.348169988e-05,
  2.057720394e-06,0.0001174666468, 0.000254732382,0.0004157739459,0.0006024184986,
   0.000816378044, 0.001059226575, 0.001332378131,  0.00163706555,  0.00197432097,
   0.002344956854, 0.002749550389, 0.003188427072, 0.003661649302, 0.004169005435,
    0.00471000094, 0.005283853505, 0.005889489781, 0.006525543984, 0.007190360688,
   0.007882000878, 0.008598247543, 0.009336617775,  0.01009437256,  0.01086853724,
    0.01165591553,  0.01245311089,  0.01325654797,  0.01406249963,  0.01486710832,
    0.01566641964,  0.01645640284,  0.01723298989,  0.01799209975,  0.01872966997,
    0.01944169216,  0.02012423798,  0.02077349275,  0.02138578519,  0.02195761539,
     0.0224856846,  0.02296692133,  0.02339850739,  0.02377789468,  0.02410283685,
    0.02437139489,  0.02458196506,  0.02473328263,  0.02482444048,  0.02485488541,
    0.02482444048,  0.02473328263,  0.02458196506,  0.02437139489,  0.02410283685,
    0.02377789468,  0.02339850739,  0.02296692133,   0.0224856846,  0.02195761539,
    0.02138578519,  0.02077349275,  0.02012423798,  0.01944169216,  0.01872966997,
    0.01799209975,  0.01723298989,  0.01645640284,  0.01566641964,  0.01486710832,
    0.01406249963,  0.01325654797,  0.01245311089,  0.01165591553,  0.01086853724,
    0.01009437256, 0.009336617775, 0.008598247543, 0.007882000878, 0.007190360688,
   0.006525543984, 0.005889489781, 0.005283853505,  0.00471000094, 0.004169005435,
   0.003661649302, 0.003188427072, 0.002749550389, 0.002344956854,  0.00197432097,
    0.00163706555, 0.001332378131, 0.001059226575, 0.000816378044,0.0006024184986,
  0.0004157739459, 0.000254732382,0.0001174666468,2.057720394e-06,-9.348169988e-05,
  -0.0001711835939,-0.0002331012802,-0.0002812864841,-0.0003177672043,-0.0003445262846,
  -0.0003634814057,-0.0003764661378,-0.0003852125083,-0.000391335343,-0.0003963182389,
  -0.0004015014856,-0.0004080719373,-0.0004170549801,-0.0004293085367
};

int main(void)
{
    arm_fir_instance_f32 S1;
    arm_fir_instance_f32 S2;
    
    u32 p_num=0;	  	// 用于刷新最大值和最小值
    u32 min[2]={0xFFFFFFFF,0xFFFFFFFF};
    u32 max[2]={0,0};
    u32 Peak;					// 峰峰值
    float BPM;				// 心率
    
    flog=0;
    OLED_Init();    //OLED初始化
    OLED_Clear();
    MAX30102_Init();
    
    // 串口初始化（在PeripheralInit中可能已经初始化）
    uart_init(115200);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    NVIC_EnableIRQ(USART1_IRQn);
    
    un_min=0x3FFFF;un_max=0;n_ir_buffer_length=500;
    for(i=0;i<n_ir_buffer_length;i++)//读取前500个样本，并确定信号范围
    {
        while(MAX30102_INT==1);   //等待，直到中断引脚断言
        max30102_FIFO_ReadBytes(REG_FIFO_DATA,temp);
        aun_red_buffer[i] =  (long)((long)((long)temp[0]&0x03)<<16) | (long)temp[1]<<8 | (long)temp[2];    // 将值合并得到实际数字
        aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03)<<16) |(long)temp[4]<<8 | (long)temp[5];   	 // 将值合并得到实际数字
        if(un_min>aun_red_buffer[i])un_min=aun_red_buffer[i];    //更新计算最小值
        if(un_max<aun_red_buffer[i])un_max=aun_red_buffer[i];    //更新计算最大值
    }
    un_prev_data=aun_red_buffer[i];
    //计算前500个样本（前5秒的样本）后的心率和血氧饱和度
    maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid); 
    
    cimeifen();
    Wait(); // 显示等待测量（英文）
    
    PeripheralInit(); // 外设初始化
    // 初始化结构体S1, 呼吸波使用
    arm_fir_init_f32(&S1, NumTaps, (float32_t *)LPF_2Hz, firState1, blockSize);
    // 初始化结构体S2, 心电波使用
    arm_fir_init_f32(&S2, NumTaps, (float32_t *)BPF_5Hz_40Hz, firState2, blockSize);
    
    CS_L;
    Delay_1us(10);
    SPI1_ReadWriteByte(RDATAC);		// 发送启动连续读取数据命令
    Delay_1us(10);
    CS_H;						
    START_H; 				// 启动转换
    CS_L;
    
    // 主循环
    while (1)
    {	
        u32 timeout = 0;
        
        // ========== 新增：处理OH风险等级数据 ==========
        uart_oh_data_process();      // 处理串口接收的OH数据
        display_oh_risk_level();     // 在OLED上显示OH风险等级f
        
        // ========== 原有的MAX30102处理逻辑 ==========
        if(max_sample_idx < 100) { // 处理100~500 -> 0~400的移位（分100次处理，分散CPU负载）
            aun_red_buffer[max_sample_idx] = aun_red_buffer[max_sample_idx + 100];
            aun_ir_buffer[max_sample_idx] = aun_ir_buffer[max_sample_idx + 100];
            // 更新最小/最大值
            if(un_min > aun_red_buffer[max_sample_idx + 100]) un_min = aun_red_buffer[max_sample_idx + 100];
            if(un_max < aun_red_buffer[max_sample_idx + 100]) un_max = aun_red_buffer[max_sample_idx + 100];
            max_sample_idx++;
        } 
        else if(max_sample_idx < 200) { // 处理400~500的采样（分100次处理）
            u16 idx = 400 + (max_sample_idx - 100); // 对应原i=400~499
            un_prev_data = aun_red_buffer[idx - 1];
            
            // 优化：添加1ms超时，避免无限等待MAX30102_INT
            while(MAX30102_INT == 1 && timeout < 1000) { 
                timeout++;
                Delay_1us(1); // 短延时，让出CPU给ADS1292
            }

            if(timeout >= 1000) { // 超时重置，防止卡死,超时则跳过本次采样，不卡CPU
                continue;
            }
            
            // 读取MAX30102 FIFO数据
            max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp);
            aun_red_buffer[idx] =  (long)((long)((long)temp[0]&0x03)<<16) | (long)temp[1]<<8 | (long)temp[2];
            aun_ir_buffer[idx] = (long)((long)((long)temp[3] & 0x03)<<16) |(long)temp[4]<<8 | (long)temp[5];
            
            // 亮度计算（原逻辑保留）
            if(aun_red_buffer[idx] > un_prev_data) {
                f_temp = aun_red_buffer[idx] - un_prev_data;
                f_temp /= (un_max - un_min);
                f_temp *= MAX_BRIGHTNESS;
                n_brightness -= (int)f_temp;
                n_brightness = n_brightness < 0 ? 0 : n_brightness;
            } else {
                f_temp = un_prev_data - aun_red_buffer[idx];
                f_temp /= (un_max - un_min);
                f_temp *= MAX_BRIGHTNESS;
                n_brightness += (int)f_temp;
                n_brightness = n_brightness > MAX_BRIGHTNESS ? MAX_BRIGHTNESS : n_brightness;
            }
            
            max_sample_idx++;
        } 
        else { // 100次移位+100次采样完成，先更新HR/SPO2，再统计+计算平均值
            // 第一步：调用算法更新最新的HR/SPO2值
            maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
            
            // 第二步：基于最新值更新dis_hr/dis_spo2（关键：移到算法调用后）
            dis_hr = (ch_hr_valid == 1 && n_heart_rate < 120 && n_heart_rate > 0) ? n_heart_rate : 0;
            dis_spo2 = (ch_spo2_valid == 1 && n_sp02 < 101 && n_sp02 > 0) ? n_sp02 : 0;
            
            // 第三步：统计非0值（关键：移到算法调用+dis更新后）
            if(dis_hr > 0) {
                hr_sum += dis_hr;
                hr_count++;
            }
            if(dis_spo2 > 0) {
                spo2_sum += dis_spo2;
                spo2_count++;
            }
            
            // 第四步：计算平均值
            hr_avg = (hr_count > 0) ? (float)hr_sum / hr_count : 0.0f;
            spo2_avg = (spo2_count > 0) ? (float)spo2_sum / spo2_count : 0.0f;
            
            // 第五步：更新OLED显示（适配函数接口）
            OLED_ShowNum(2, 5, (uint32_t)hr_avg, 3);      // 第2行，第5列显示心率平均值
            OLED_ShowNum(3, 7, (uint32_t)spo2_avg, 3);    // 第3行，第7列显示血氧平均值

            // 可选：累计一定次数后重置平均值（避免数值溢出，根据需求调整）
            if(hr_count > 1000 || spo2_count > 1000) {
                hr_sum = 0; hr_count = 0;
                spo2_sum = 0; spo2_count = 0;
            }
            
            // 重置索引和标志
            max_sample_idx = 0;
            un_min = 0x3FFFF;
            un_max = 0;
            calc_hr_spo2_flag = 1; // 标记HR/SPO2已更新
        }

        // 原有的ADS1292数据处理逻辑
        if(flog==1)
        {
            // 通道1呼吸波数据			
            Input_data1=(float32_t)(ch1_data^0x800000);
            // 实现FIR滤波
            arm_fir_f32(&S1, &Input_data1, &Output_data1, blockSize);
            
            // 通道2心电波形数据			
            Input_data2=(float32_t)(ch2_data^0x800000);
            // 实现FIR滤波
            arm_fir_f32(&S2, &Input_data2, &Output_data2, blockSize);
            
            // 比较大小
            if(min[1]>Output_data2)
                min[1]=Output_data2;
            if(max[1]<Output_data2)
                max[1]=Output_data2;
            
            BPM_LH[0]=BPM_LH[1];
            BPM_LH[1]=BPM_LH[2];
            BPM_LH[2]=Output_data2;
            if((BPM_LH[0]<BPM_LH[1])&(BPM_LH[1]>max[0]-Peak/3)&(BPM_LH[2]<BPM_LH[1]))
            {
                BPM=(float)60000.0/(point_cnt*4);
                point_cnt=0;
            }
            
            // 每隔2000个点重新测量一次最大最小值
            p_num++;
            if(p_num>2000)
            {
                min[0]=min[1];			
                max[0]=max[1];
                min[1]=0xFFFFFFFF;
                max[1]=0;
                Peak=max[0]-min[0];
                p_num=0;
            }
            
            // 串口打印（此时avg已基于最新值计算）
            printf("HR: %3d SPO2: %3d HR_AVG: %6.2f SPO2_AVG: %6.2f B: %8d A: %8d C: %6.2f\n", 
                   dis_hr, dis_spo2, hr_avg, spo2_avg, (u32)Output_data1, (u32)Output_data2, BPM);
            
            flog=0;
        }
        
        // 短暂延时，避免CPU占用过高
        Delay_1us(100);
    }
}
