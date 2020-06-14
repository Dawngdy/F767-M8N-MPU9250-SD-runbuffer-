#include "exti.h"
#include "delay.h"
#include "task1.h"

void EXTI_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_GPIOC_CLK_ENABLE();               //开启GPIOA时钟
    
    GPIO_Initure.Pin=GPIO_PIN_13;               //PC13
    GPIO_Initure.Mode=GPIO_MODE_IT_FALLING;     //下降沿触发
    GPIO_Initure.Pull=GPIO_PULLUP;				//上拉
    HAL_GPIO_Init(GPIOC,&GPIO_Initure);
    
    //中断线13
    HAL_NVIC_SetPriority(EXTI15_10_IRQn,6,3);   //抢占优先级为3，子优先级为3
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);         //使能中断线13  
}


//中断服务函数

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);  //调用中断处理公用函数
}

//中断服务程序中需要做的事情
//在HAL库中所有的外部中断服务函数都会调用此函数
//GPIO_Pin:中断引脚号
uint8_t pps_get_flag;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch(GPIO_Pin)
    {

        case GPIO_PIN_13:
						//printf("here is pps\r\n");
				
						pps_get_flag = 1;
				
						if(pps_get_flag)
						{
							imu_massage.imu_pps = 1;
						}
				
            break;
    }
}

