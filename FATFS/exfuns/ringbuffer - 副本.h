#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include "stdbool.h"
#include "stdint.h"
#include "string.h"

/*是(1)否(0)使用环形缓冲区*/
#define IF_USE_RING_BUFFER    1

/*数据缓冲区长度*/
#define DATA_BUF_LENGTH              10500

/*环形缓冲区相关结构体*/
typedef struct {
    volatile uint8_t  head;/*缓冲区头部*/
    volatile uint8_t  tail;/*缓冲区尾部*/
    volatile uint8_t  dataLen;/*缓冲区内数据长度*/
    volatile uint8_t  readWriteMutexFlag;/*读写互斥标志*/
    uint8_t           aFrameLen[100];/*存储接收帧的长度*/  //25
    volatile uint8_t  frameNum;/*缓冲区内桢数*/
    uint8_t           ringBuf[DATA_BUF_LENGTH];/*缓冲区*/
}ringBufType_t;

/*多缓冲*/
typedef struct {
    ringBufType_t RingBuf_1;
    ringBufType_t RingBuf_2;
}multRingBufType_t;

#ifndef COUNTOF   //C++中计算一个固定大小数组的宏
#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))
#endif

#if IF_USE_RING_BUFFER
/*外部声明*/
extern multRingBufType_t multRingBufDeal;
extern ringBufType_t sinRingBuf;  //定义的单缓冲结构体

/*写入数据到环形缓冲区*/
bool WriteDataToRingBuffer(ringBufType_t *pRingBuf, uint8_t *pBuf, uint32_t len);
/*从环形缓冲区读出数据*/
uint8_t ReadDataFromRingBuffer(ringBufType_t *pRingBuf, uint8_t *pBuf, uint32_t len);
#endif/*IF_USE_RING_BUFFER*/

#endif/*__RING_BUFFER_H*/