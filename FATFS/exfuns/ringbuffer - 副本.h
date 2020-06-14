#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include "stdbool.h"
#include "stdint.h"
#include "string.h"

/*��(1)��(0)ʹ�û��λ�����*/
#define IF_USE_RING_BUFFER    1

/*���ݻ���������*/
#define DATA_BUF_LENGTH              10500

/*���λ�������ؽṹ��*/
typedef struct {
    volatile uint8_t  head;/*������ͷ��*/
    volatile uint8_t  tail;/*������β��*/
    volatile uint8_t  dataLen;/*�����������ݳ���*/
    volatile uint8_t  readWriteMutexFlag;/*��д�����־*/
    uint8_t           aFrameLen[100];/*�洢����֡�ĳ���*/  //25
    volatile uint8_t  frameNum;/*������������*/
    uint8_t           ringBuf[DATA_BUF_LENGTH];/*������*/
}ringBufType_t;

/*�໺��*/
typedef struct {
    ringBufType_t RingBuf_1;
    ringBufType_t RingBuf_2;
}multRingBufType_t;

#ifndef COUNTOF   //C++�м���һ���̶���С����ĺ�
#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))
#endif

#if IF_USE_RING_BUFFER
/*�ⲿ����*/
extern multRingBufType_t multRingBufDeal;
extern ringBufType_t sinRingBuf;  //����ĵ�����ṹ��

/*д�����ݵ����λ�����*/
bool WriteDataToRingBuffer(ringBufType_t *pRingBuf, uint8_t *pBuf, uint32_t len);
/*�ӻ��λ�������������*/
uint8_t ReadDataFromRingBuffer(ringBufType_t *pRingBuf, uint8_t *pBuf, uint32_t len);
#endif/*IF_USE_RING_BUFFER*/

#endif/*__RING_BUFFER_H*/