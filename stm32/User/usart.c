#include "usart.h"	 

//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	//循环发送,直到发送完毕
	while((USART1->SR&0X40)==0)
		;   
  USART1->DR = (u8) ch;      
	return ch;
}
#endif 
	
u8 USART_RX_BUF[USART_REC_LEN];     // 接收缓冲,最大USART_REC_LEN个字节
u16 USART_RX_STA=0; // 接收状态标记（bit15：接收完成标志  bit14：接收到0x0d   bit13~0：接收到的有效字节数目）

//-----------------------------------------------------------------
// void uart_init(u32 bound)
//-----------------------------------------------------------------
//
// 函数功能: 串口初始化
// 入口参数: 无
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void uart_init(u32 bound)
{
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
  
	// USART1_TX -> PA9
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; 				// PA.9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	// 高速
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		// 复用推挽输出
  GPIO_Init(GPIOA, &GPIO_InitStructure);						// 初始化
   
  // USART1_RX -> PA10
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;						// PA10
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	// 浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);								// 初始化 

  // Usart1 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;	// 抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;				// 子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;						// IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	// 根据指定的参数初始化VIC寄存器
  
  // USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;									// 串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	// 字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;			// 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;					// 无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	// 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	// 收发模式

  USART_Init(USART1, &USART_InitStructure); 			// 初始化串口1
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);	// 开启串口接受中断
  USART_Cmd(USART1, ENABLE);                    	// 使能串口1 

}

//-----------------------------------------------------------------
// void uart_init(u32 bound)
//-----------------------------------------------------------------
//
// 函数功能: 串口1中断服务函数
// 入口参数: 无
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
// USART1中断服务函数
// 串口中断服务函数示例（适配完整格式）
void USART1_IRQHandler(void)
{
    u8 ch;
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        ch = USART_ReceiveData(USART1);
        
        // 接收所有ASCII字符（包括数字、字母和特殊字符）
        if(ch >= 32 && ch <= 126)  // 所有可打印ASCII字符
        {
            // 循环缓冲区写入
            u8 next_tail = (uart_rx_tail + 1) % UART_RX_BUF_SIZE;
            if(next_tail != uart_rx_head)
            {
                uart_rx_buf[uart_rx_tail] = ch;
                uart_rx_tail = next_tail;
            }
        }
        else if(ch == '\n' || ch == '\r')  // 换行符或回车符作为结束
        {
            uart_rx_flag = 1;  // 触发数据处理标志
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------
