#include "stm32f10x.h"
#include "Delay.h"


// void TimingDelay_Decrement(void)
// 函数功能: 延时计数器递减函数

void TimingDelay_Decrement(void)
{
}

// void Delay_ns (unsigned char t)
// 函数功能: 时基为ns的延时

void Delay_ns(u8 t)
{
  do
  {
    ;
  } while (--t);
}

// void Delay_1us (unsigned char t)
// 函数功能: 时基为1us的延时
// 入口参数: 无符号8bit整数

void Delay_1us(u8 t)
{
  u8 i = 0;

  do
  {
    i = 8; // i=7,t=0.986us,误差14ns
    do
    {
    } while (--i);
  } while (--t);
}


// void Delay_2us (u16 t)
// 函数功能: 时基为2us的延时
// 入口参数: 无符号8bit整数

void Delay_2us(u16 t)
{
  u8 i = 0;

  do
  {
    i = 15; // i=7,t=0.986us,误差14ns
    do
    {
    } while (--i);
  } while (--t);
}

// void Delay_10us (u8 t)
// 函数功能: 时基为10us的延时
// 入口参数: 无符号8bit整数

void Delay_10us(u8 t)
{
  u16 i, j;

  do
  {
    j = 7; // j=6,i=13,737,10.236us,误差236ns
    do
    { // j=7,i=11,731,10.152us,误差152ns
      i = 11;
      do
      {
      } while (--i);
    } while (--j);
  } while (--t);
}


// void Delay_250us (u8 t)
// 函数功能: 时基为250us的延时
// 入口参数: 无符号8bit整数

void Delay_250us(u8 t)
{
  unsigned char i, j;

  do
  {
    j = 66; // j=66,i=30, 18035,250.486us
    do
    {
      i = 30;
      do
      {
      } while (--i);
    } while (--j);
  } while (--t);
}

// void Delay_882us (void)
// 函数功能: 延时882us
void Delay_882us(void)
{
  u16 i, j;
  j = 101; // j=101,i=88,63431,880.986us
  do
  {
    i = 88;
    do
    {
    } while (--i);
  } while (--j);
}


// void Delay_1ms (unsigned char t)
// 函数功能: 时基为1ms的延时
// 入口参数: 无符号8bit整数

void Delay_1ms(u8 t)
{
  u16 i, j;

  do
  {
    j = 119;
    do
    {
      i = 67; // 1.002278ms ,误差2us
      do
      {
      } while (--i); //
    } while (--j);
  } while (--t);
}


// void Delay_5ms (unsigned char t)
// 函数功能: 时基为5ms的延时
// 入口参数: 无符号8bit整数

void Delay_5ms(u8 t)
{
  u16 i, j;

  do
  {
    j = 625; //j=625,u=63,误差2.30us
    do
    {
      i = 63;
      do
      {
      } while (--i);
    } while (--j);
  } while (--t);
}


// void Delay_50ms (u8 t)
// 函数功能: 时基为50ms的延时
// 例子提示: 调用Delay_50ms(20),得到1s延时
// 入口参数: 无符号8bit整数

void Delay_50ms(u8 t)
{
  u16 i, j;

  do
  {
    j = 1000; // j=1000,i=513-->6us
    do
    {
      i = 513;
      do
      {
      } while (--i);
    } while (--j);
  } while (--t);
}


// void Delay(__IO uint32_t nCount)
// 函数功能: 粗略延时
// 入口参数: 延时长度

void Delay(__IO uint32_t nCount)
{
  for (; nCount != 0; nCount--)
    ; // 23个
}


