#include "myiic.h"
#include "delay.h"
#include "mpu9250.h"
#include "usart.h"
#include "dma.h"
#include "rtc.h"
#include <string.h>
#include "VariableType.h"

#include "task1.h"
#include "task2.h"
//#include "exfuns.h"
#include "ringbuffer.h"

extern uint8_t pps_get_flag;
uint8_t IMUTime_Data[10];
uint8_t data_count=0;
uint8_t IMU_TIME_Data[4];
uint8_t IMU_TIME[3];

GETIMU_DATE_T IMU_DATA;
RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;

void MPU9250_Init(void)
{
		 u8 res=0;
		
		 res = MPU_Read(GYRO_ADDRESS,WHO_AM_I);
	
		 if(res == 0x71)
		 {
				  MPU_Write(GYRO_ADDRESS,PWR_MGMT_1,0x00);	//解除休眠状态
				  MPU_Write(GYRO_ADDRESS,SMPLRT_DIV,0x07);
				  MPU_Write(GYRO_ADDRESS,CONFIG,0x06);
				  MPU_Write(GYRO_ADDRESS,GYRO_CONFIG,0x18);
			  	MPU_Write(GYRO_ADDRESS,ACCEL_CONFIG,0x01);
	   }
}

void Get_IMU(uint8_t *IMU_buff)
{
		 unsigned char BUF[10];       //接收数据缓存区
		 uint8_t Acc_x,Acc_y,Acc_z;
		 uint8_t Gyr_x,Gyr_y,Gyr_z;

		 BUF[0]=MPU_Read(ACCEL_ADDRESS,ACCEL_XOUT_L); 
		 BUF[1]=MPU_Read(ACCEL_ADDRESS,ACCEL_XOUT_H);
		 Acc_x=	(BUF[1]<<8)|BUF[0];//读取计算X轴数据				   

		 BUF[2]=MPU_Read(ACCEL_ADDRESS,ACCEL_YOUT_L);
		 BUF[3]=MPU_Read(ACCEL_ADDRESS,ACCEL_YOUT_H);
		 Acc_y=	(BUF[3]<<8)|BUF[2];//读取计算Y轴数据
								 
		 BUF[4]=MPU_Read(ACCEL_ADDRESS,ACCEL_ZOUT_L);
		 BUF[5]=MPU_Read(ACCEL_ADDRESS,ACCEL_ZOUT_H);
		 Acc_z=	(BUF[5]<<8)|BUF[4];//读取计算Z轴数据
	
		 BUF[0]=MPU_Read(GYRO_ADDRESS,GYRO_XOUT_L); 
		 BUF[1]=MPU_Read(GYRO_ADDRESS,GYRO_XOUT_H);
		 Gyr_x=	(BUF[1]<<8)|BUF[0];//读取计算X轴数据						   

		 BUF[2]=MPU_Read(GYRO_ADDRESS,GYRO_YOUT_L);
		 BUF[3]=MPU_Read(GYRO_ADDRESS,GYRO_YOUT_H);
		 Gyr_y=	(BUF[3]<<8)|BUF[2];//读取计算Y轴数据
							 
		 BUF[4]=MPU_Read(GYRO_ADDRESS,GYRO_ZOUT_L);
		 BUF[5]=MPU_Read(GYRO_ADDRESS,GYRO_ZOUT_H);
		 Gyr_z=	(BUF[5]<<8)|BUF[4];//读取计算Z轴数据	
	
		 IMU_buff[0] = Gyr_x;
		 IMU_buff[1] = Gyr_y;
		 IMU_buff[2] = Gyr_z;
		 IMU_buff[3] = Acc_x;
		 IMU_buff[4] = Acc_y;
		 IMU_buff[5] = Acc_z;
}

void MPU9250_Start(void)
{
		 MPU9250_Init();
}

uint8_t ProcessCheckIMU(uint8_t DNum,uint8_t IMU_DATAT[])
{
	static  uint8_t i;
	static uint8_t CheckNum;

	CheckNum=0;
	for(i=1;i<DNum;i++)
	{
	CheckNum^=IMU_DATAT[i]; 
	}   
	return CheckNum;
}

//----------------------------------------
//void ProcessCheckToA(uint8_t CheckNum)
//----------------------------------------
void ProcessCheckToAIMU(uint8_t CheckNum,uint8_t Change_Data[])
{									
static   uint8_t  i;
  			     
   Change_Data[0] = CheckNum/16 ; 			       //十位
   Change_Data[1] = CheckNum -Change_Data[0]*16;	   //个位

   for(i=0;i<2;i++)
   {
    if(Change_Data[i] <= 9)
	{
	 Change_Data[i] = (Change_Data[i] + '0') ;
	}
	else
	{
	 Change_Data[i] = ((Change_Data[i]-10) + 'A') ;
	}   
   }
}

void Change_SensorData(uint16_t SensorData,uint8_t Change_Data[])
{
    static uint8_t  i;
	
	Change_Data[0]=SensorData/16/16/16;
	Change_Data[1]=SensorData/16/16-Change_Data[0]*16;
	Change_Data[2]=SensorData/16-Change_Data[0]*16*16-Change_Data[1]*16;
	Change_Data[3]=SensorData-Change_Data[0]*16*16*16-Change_Data[1]*16*16-Change_Data[2]*16;
	
	for(i=0;i<4;i++)
	{
		if(Change_Data[i] <= 9)
		{
		Change_Data[i] = (Change_Data[i] + '0') ;
		}
		else
		{
		Change_Data[i] = ((Change_Data[i]-10) + 'A') ;
		}   
	}
}

void ProcessCount(uint8_t IMUIData)
{									
 static  uint8_t  i;
  			     
   IMU_TIME_Data[0] = IMUIData/10 ;				   //十位
   IMU_TIME_Data[1] = IMUIData -IMU_TIME_Data[0]*10;	   //个位

   for(i=0;i<2;i++)
   {
    IMU_TIME_Data[i] = IMU_TIME_Data[i] + 0x30;   
   }
}

void ProcessCountms(uint16_t IMUIData)
{									
 static  uint8_t  i;
  			     
   IMU_TIME_Data[0] = IMUIData/1000 ;				   //千位
   IMU_TIME_Data[1] = IMUIData/100 - ((uint16_t)IMU_TIME_Data[0])*10;	   //百位
	 IMU_TIME_Data[2] = IMUIData/10 - ((uint16_t)IMU_TIME_Data[0])*100 - ((uint16_t)IMU_TIME_Data[1])*10;  //十位
	 IMU_TIME_Data[3] = IMUIData - ((uint16_t)IMU_TIME_Data[0])*1000 - ((uint16_t)IMU_TIME_Data[1])*100 - ((uint16_t)IMU_TIME_Data[2])*10;  //个位
	
   for(i=0;i<4;i++)
   {
    IMU_TIME_Data[i] = IMU_TIME_Data[i] + 0x30;   
   }
}

void IMUData_Deal(Imu_Message_t *Sys_Status)
{
	uint8_t IMU_DATAT[60],Change_Data[10],Imu_Up_Num=0,CheckNumData=0,i;
	Imu_Message_t *IMU_Status;
	
	HAL_RTC_GetTime(&RTC_Handler,&RTC_TimeStruct,RTC_FORMAT_BIN);
	RTC_TimeStruct.SubSeconds=(255-RTC_TimeStruct.SubSeconds)*1000/256;
//	printf("Time: %d:%d:%d:%d\r\n",RTC_TimeStruct.Hours,RTC_TimeStruct.Minutes,RTC_TimeStruct.Seconds,RTC_TimeStruct.SubSeconds); 
	HAL_RTC_GetDate(&RTC_Handler,&RTC_DateStruct,RTC_FORMAT_BIN); 
	
	IMU_Status = Sys_Status;
	
	IMU_DATAT[0] = '$';
  IMU_DATAT[1] = 'G';
  IMU_DATAT[2] = 'P';
  IMU_DATAT[3] = 'I';
  IMU_DATAT[4] = 'M';
  IMU_DATAT[5] = 'U';
  IMU_DATAT[6] = ',';
	
	Change_SensorData(IMU_Status->imu_data[3],Change_Data);
  IMU_DATAT[Imu_Up_Num+7]  = Change_Data[0];
  IMU_DATAT[Imu_Up_Num+8]  = Change_Data[1];
  IMU_DATAT[Imu_Up_Num+9]  = Change_Data[2];
  IMU_DATAT[Imu_Up_Num+10] = Change_Data[3];
  IMU_DATAT[Imu_Up_Num+11] = ',';
	
	Change_SensorData(IMU_Status->imu_data[4],Change_Data);
	IMU_DATAT[Imu_Up_Num+12] = Change_Data[0];
	IMU_DATAT[Imu_Up_Num+13] = Change_Data[1];
	IMU_DATAT[Imu_Up_Num+14] = Change_Data[2];
	IMU_DATAT[Imu_Up_Num+15] = Change_Data[3];
	IMU_DATAT[Imu_Up_Num+16] = ',';	
	
	Change_SensorData(IMU_Status->imu_data[5],Change_Data);
	IMU_DATAT[Imu_Up_Num+17] = Change_Data[0];
	IMU_DATAT[Imu_Up_Num+18] = Change_Data[1];
	IMU_DATAT[Imu_Up_Num+19] = Change_Data[2];
	IMU_DATAT[Imu_Up_Num+20] = Change_Data[3];
	IMU_DATAT[Imu_Up_Num+21] = ',';
	
  Change_SensorData(IMU_Status->imu_data[0],Change_Data);
	IMU_DATAT[Imu_Up_Num+22] = Change_Data[0];
	IMU_DATAT[Imu_Up_Num+23] = Change_Data[1];
	IMU_DATAT[Imu_Up_Num+24] = Change_Data[2];
	IMU_DATAT[Imu_Up_Num+25] = Change_Data[3];
	IMU_DATAT[Imu_Up_Num+26] = ',';
	
	Change_SensorData(IMU_Status->imu_data[1],Change_Data);
	IMU_DATAT[Imu_Up_Num+27] = Change_Data[0];
	IMU_DATAT[Imu_Up_Num+28] = Change_Data[1];
	IMU_DATAT[Imu_Up_Num+29] = Change_Data[2];
	IMU_DATAT[Imu_Up_Num+30] = Change_Data[3];
	IMU_DATAT[Imu_Up_Num+31] = ',';	
	
	Change_SensorData(IMU_Status->imu_data[2],Change_Data);
	IMU_DATAT[Imu_Up_Num+32] = Change_Data[0];
	IMU_DATAT[Imu_Up_Num+33] = Change_Data[1];
	IMU_DATAT[Imu_Up_Num+34] = Change_Data[2];
	IMU_DATAT[Imu_Up_Num+35] = Change_Data[3];
	IMU_DATAT[Imu_Up_Num+36] = ',';
	
	ProcessCount(RTC_TimeStruct.Hours);
	IMU_DATAT[Imu_Up_Num+37] = IMU_TIME_Data[0];
	IMU_DATAT[Imu_Up_Num+38] = IMU_TIME_Data[1];
	
	ProcessCount(RTC_TimeStruct.Minutes);
	IMU_DATAT[Imu_Up_Num+39] = IMU_TIME_Data[0];
	IMU_DATAT[Imu_Up_Num+40] = IMU_TIME_Data[1];
	
	ProcessCount(RTC_TimeStruct.Seconds);
	IMU_DATAT[Imu_Up_Num+41] = IMU_TIME_Data[0];
	IMU_DATAT[Imu_Up_Num+42] = IMU_TIME_Data[1];
	IMU_DATAT[Imu_Up_Num+43] = '.';
	
	ProcessCountms(RTC_TimeStruct.SubSeconds);
	IMU_DATAT[Imu_Up_Num+44] = IMU_TIME_Data[0];
	IMU_DATAT[Imu_Up_Num+45] = IMU_TIME_Data[1];
	IMU_DATAT[Imu_Up_Num+46] = IMU_TIME_Data[2];
	IMU_DATAT[Imu_Up_Num+47] = IMU_TIME_Data[3];

	IMU_DATAT[Imu_Up_Num+48] = '*';
	
	CheckNumData    = ProcessCheckIMU(Imu_Up_Num+49,IMU_DATAT);
  ProcessCheckToAIMU(CheckNumData,Change_Data);
	IMU_DATAT[Imu_Up_Num+49] = Change_Data[0];
	IMU_DATAT[Imu_Up_Num+50] = Change_Data[1];
	
	IMU_DATAT[Imu_Up_Num+51] = '\n';	 

		
  //保存IMU数据
	 printf("IMU_DATAT \n");
	 rec_sai_fifo_write(IMU_DATAT,51);
	
	//sd_savesl("0:SaveData/GpsImuData.txt",IMU_DATAT,52);	
	//保存IMU数据到sinRingBuf
//	if( (WriteDataToRingBuffer( &sinRingBuf, IMU_DATAT, 52)) == false)
//	{
//	  printf("three have trouble, IMU_DATAT");
//	}
	
	/*HAL_UART_Transmit_DMA(&UART1_Handler,IMU_DATAT,52);//开启DMA传输
			
	while(1)
	{
		if(__HAL_DMA_GET_FLAG(&UART1TxDMA_Handler,DMA_FLAG_TCIF3_7))//等待DMA2_Steam7传输完成
		{
			 __HAL_DMA_CLEAR_FLAG(&UART1TxDMA_Handler,DMA_FLAG_TCIF3_7);//清除DMA2_Steam7传输完成标志
			 HAL_UART_DMAStop(&UART1_Handler);      //传输完成以后关闭串口DMA
			 break; 
		}   
	}
	*/
}



