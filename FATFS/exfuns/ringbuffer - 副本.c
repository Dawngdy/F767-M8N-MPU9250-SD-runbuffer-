#include "ringbuffer.h"

/*����һ���໺��ṹ��*/
multRingBufType_t multRingBufDeal;
ringBufType_t sinRingBuf;  //����ĵ�����ṹ��

/**
* @brief  д�����ݵ����λ�����
* @param  pBuf: ��д�����ݻ�����
*              len:    ��д�����ݳ���
* @retval ִ��״̬
*/
bool WriteDataToRingBuffer(ringBufType_t *pRingBuf, uint8_t *pBuf, uint32_t len)
{
	  uint32_t tmp = 0;
	
    if (pRingBuf == NULL) { return false; }
    if (pBuf == NULL) { return false; }
    /*���ݳ��Ȳ���Ϊ 0*/
    if (len <= 0) { return false; }

    /*д��ᵼ�»��������*/
    if (len > (COUNTOF(pRingBuf->ringBuf) - pRingBuf->dataLen)) { return false; }
    /*��д��ᵼ���峤�洢���������*/
    if (pRingBuf->frameNum >= COUNTOF(pRingBuf->aFrameLen)) { return false; }
    /*��д����*/
    if (pRingBuf->readWriteMutexFlag) { return false; }

  
    pRingBuf->readWriteMutexFlag = true;/*�򿪻����־*/
    pRingBuf->aFrameLen[pRingBuf->frameNum] = len;/*�洢�峤*/
    
    /*�������һȦ��ֱ�Ӵ洢*/
    if ((COUNTOF(pRingBuf->ringBuf) - pRingBuf->tail) >= pRingBuf->aFrameLen[pRingBuf->frameNum])
    {
        memcpy(&(pRingBuf->ringBuf[pRingBuf->tail]), pBuf, pRingBuf->aFrameLen[pRingBuf->frameNum]);
        pRingBuf->tail += pRingBuf->aFrameLen[pRingBuf->frameNum];
        /*�������һȦ��ص�"ͷ��"*/
        if (pRingBuf->tail == COUNTOF(pRingBuf->ringBuf))
        {
            pRingBuf->tail = 0;
        }
    }
    /*����һȦ��ֳ������ִ洢*/
    else
    {
        memcpy(&(pRingBuf->ringBuf[pRingBuf->tail]), pBuf, COUNTOF(pRingBuf->ringBuf) - pRingBuf->tail);
        tmp = COUNTOF(pRingBuf->ringBuf) - pRingBuf->tail;
        pRingBuf->tail = 0;
        memcpy(&(pRingBuf->ringBuf[pRingBuf->tail]), &pBuf[tmp], pRingBuf->aFrameLen[pRingBuf->frameNum] - tmp);
        pRingBuf->tail = pRingBuf->aFrameLen[pRingBuf->frameNum] - tmp;
    }
    
    pRingBuf->dataLen += pRingBuf->aFrameLen[pRingBuf->frameNum];/*���´洢���ݳ���*/
    pRingBuf->frameNum++;/*����֡��*/

    pRingBuf->readWriteMutexFlag = false;/*�رջ����־*/

    return true;
}
/**
* @brief  �ӻ��λ�������������
* @param  pBuf: ��Ŷ������ݻ�����
*              len:    ��Ŷ������ݻ���������
* @retval ʵ�ʶ���������
*/
uint8_t ReadDataFromRingBuffer(ringBufType_t *pRingBuf, uint8_t *pBuf, uint32_t len)
{
	  uint32_t ret = 0;
    uint32_t tmp = 0;
	  uint8_t i = 0;
    if (pRingBuf == NULL) { return 0x00; }
    if (pBuf == NULL) { return 0x00; }
    /*��������С*/
    if (len < pRingBuf->aFrameLen[0]) { return 0x00; }
    /*�����ݿɶ�*/
    if (pRingBuf->dataLen <= 0) { return 0x00; }
    /*��д����*/
    if (pRingBuf->readWriteMutexFlag) { return 0x00; }

    pRingBuf->readWriteMutexFlag = true;/*�򿪻����־*/
    pRingBuf->dataLen -= pRingBuf->aFrameLen[0];/*���´洢���ݳ���*/
    ret = pRingBuf->aFrameLen[0];/*��ȡ֡��*/

    /*�������һȦ��ֱ�Ӷ�ȡ*/
    if ((COUNTOF(pRingBuf->ringBuf) - pRingBuf->head) >= pRingBuf->aFrameLen[0])
    {
        memcpy(pBuf, &(pRingBuf->ringBuf[pRingBuf->head]), pRingBuf->aFrameLen[0]);
        pRingBuf->head += pRingBuf->aFrameLen[0];
        /*�������һȦ��ص�"β��"*/
        if (pRingBuf->head == COUNTOF(pRingBuf->ringBuf))
        {
            pRingBuf->head = 0;
        }
    }
    /*����һȦ��ֳ������ֶ�ȡ*/
    else
    {
        memcpy(pBuf, &(pRingBuf->ringBuf[pRingBuf->head]), COUNTOF(pRingBuf->ringBuf) - pRingBuf->head);
        tmp = COUNTOF(pRingBuf->ringBuf) - pRingBuf->head;
        pRingBuf->head = 0;
        memcpy(&pBuf[tmp], &(pRingBuf->ringBuf[pRingBuf->head]), pRingBuf->aFrameLen[0] - tmp);
        pRingBuf->head = pRingBuf->aFrameLen[0] - tmp;
    }

    for ( i=0; i < (pRingBuf->frameNum - 1); i++)
    {
        /*��δ���������峤ʼ������ǰ��*/
        pRingBuf->aFrameLen[i] = pRingBuf->aFrameLen[i + 1];
    }
    pRingBuf->aFrameLen[pRingBuf->frameNum - 1] = 0x00;/*֡���������������ƺ� 0*/

    pRingBuf->frameNum--;/*��ȡ��֡���� 1*/

    pRingBuf->readWriteMutexFlag = false;/*�رջ����־*/

    return ret;
}