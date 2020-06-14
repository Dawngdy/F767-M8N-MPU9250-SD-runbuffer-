#include "exti.h"
#include "delay.h"
#include "task1.h"

void EXTI_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_GPIOC_CLK_ENABLE();               //����GPIOAʱ��
    
    GPIO_Initure.Pin=GPIO_PIN_13;               //PC13
    GPIO_Initure.Mode=GPIO_MODE_IT_FALLING;     //�½��ش���
    GPIO_Initure.Pull=GPIO_PULLUP;				//����
    HAL_GPIO_Init(GPIOC,&GPIO_Initure);
    
    //�ж���13
    HAL_NVIC_SetPriority(EXTI15_10_IRQn,6,3);   //��ռ���ȼ�Ϊ3�������ȼ�Ϊ3
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);         //ʹ���ж���13  
}


//�жϷ�����

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);  //�����жϴ����ú���
}

//�жϷ����������Ҫ��������
//��HAL�������е��ⲿ�жϷ�����������ô˺���
//GPIO_Pin:�ж����ź�
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

