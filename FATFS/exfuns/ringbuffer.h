#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H
#include "sys.h"

	
//RingBuffer_Version1
#define RingBuffer_Version1   1

#if  RingBuffer_Version1
//GPS 5HZ��IMU 100HZ��ÿ���FIFO����SSD��д5�Σ���ÿ0.2��ÿ�ζ�30֡д��SD������ÿ0.2���ֻ����IMU+GPS��25֡��FIFO��
//30 > 25 �ʣ�ÿ0.2������ٶȻ��ǿ���д���ٶȵģ�ѡ150������FIFO�㹻���ˣ�ʵ�����50�����У�150����ͦ��

#define RB_FIFO_FRAME 150  	//4096		//����ÿ��С����Ĵ�СΪRB_FIFO_FRAME����ÿ֡�ɱ������ݵĴ�С���ݣ���ÿ֡���յ������һ֡����
#define RB_FIFO_SIZE	150  	//10			//�������FIFO��С,����ά�������RB_FIFO_SIZE��С����
extern u8 *sairecfifobuf[RB_FIFO_SIZE];//����10��¼������FIFO   ////sairecfifobuf��һ����άָ��

extern u8 rec_sai_fifo_read(u8 **buf);  //�����bufҪע���ϣ�����ʲô����άָ��
extern u8 rec_sai_fifo_write(u8 *buf,u8 cpynum);
#endif

#endif

