#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H
#include "sys.h"

	
//RingBuffer_Version1
#define RingBuffer_Version1   1

#if  RingBuffer_Version1
//GPS 5HZ，IMU 100HZ，每秒从FIFO中往SSD卡写5次，即每0.2秒每次读30帧写到SD卡，而每0.2秒又会存入IMU+GPS共25帧到FIFO。
//30 > 25 故，每0.2秒读的速度还是快于写的速度的，选150个接收FIFO足够用了，实验表明50个不行，150用着挺好

#define RB_FIFO_FRAME 150  	//4096		//定义每个小数组的大小为RB_FIFO_FRAME，即每帧可保存数据的大小数据，由每帧接收的最长的那一帧决定
#define RB_FIFO_SIZE	150  	//10			//定义接收FIFO大小,即二维数组包含RB_FIFO_SIZE个小数组
extern u8 *sairecfifobuf[RB_FIFO_SIZE];//定义10个录音接收FIFO   ////sairecfifobuf是一个二维指针

extern u8 rec_sai_fifo_read(u8 **buf);  //这里的buf要注意呦，它是什么？二维指针
extern u8 rec_sai_fifo_write(u8 *buf,u8 cpynum);
#endif

#endif

