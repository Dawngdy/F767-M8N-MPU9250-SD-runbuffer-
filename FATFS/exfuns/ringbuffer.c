#include "ringbuffer.h"
#include "ff.h"
#include "string.h" 

//REC¼��FIFO�������.
//����FATFS�ļ�д��ʱ��Ĳ�ȷ����,���ֱ���ڽ����ж�����д�ļ�,���ܵ���ĳ��д��ʱ�����
//�Ӷ��������ݶ�ʧ,�ʼ���FIFO����,�Խ��������.
vu8 sairecfifordpos=0;	//FIFO��λ��
vu8 sairecfifowrpos=0;	//FIFOдλ��
u8 *sairecfifobuf[RB_FIFO_SIZE];//����10��¼������FIFO   ////sairecfifobuf��һ����άָ��

//��ȡ¼��FIFO
//buf:���ݻ������׵�ַ
//����ֵ:0,û�����ݿɶ�;
//      1,������1�����ݿ�
u8 rec_sai_fifo_read(u8 **buf)  //�����bufҪע���ϣ�����ʲô����άָ��
{
	if(sairecfifordpos==sairecfifowrpos)return 0;  //fifo�������ݲ��ɶ�
	sairecfifordpos++;		//��λ�ü�1
	//printf("fR��%d\n",sairecfifordpos);
	if(sairecfifordpos>=RB_FIFO_SIZE)sairecfifordpos=0;//���� 
	*buf=sairecfifobuf[sairecfifordpos]; 
	return 1;
}
//дһ��¼��FIFO
//buf:���ݻ������׵�ַ
//����ֵ:0,д��ɹ�;
//      1,д��ʧ��
u8 rec_sai_fifo_write(u8 *buf,u8 cpynum)
{
	u16 i;
	u8 temp=sairecfifowrpos;//��¼��ǰдλ��
	sairecfifowrpos++;		//дλ�ü�1
	//printf("fW��%d\n",sairecfifowrpos);
	if(sairecfifowrpos>=RB_FIFO_SIZE)sairecfifowrpos=0;//����  
	if(sairecfifordpos==sairecfifowrpos)  //˵���������ˣ�дָ�붼�������ˣ�����rec_sai_fifo_read����֡����
	{
		sairecfifowrpos=temp;//��ԭԭ����дλ��,�˴�д��ʧ��
		return 1;	
	}
	for(i=0;i<cpynum;i++)
	sairecfifobuf[sairecfifowrpos][i]=buf[i];//��������  //sairecfifobuf��һ����άָ��
	return 0;
} 


	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	














