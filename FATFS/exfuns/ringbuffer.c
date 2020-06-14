#include "ringbuffer.h"
#include "ff.h"
#include "string.h" 

//REC录音FIFO管理参数.
//由于FATFS文件写入时间的不确定性,如果直接在接收中断里面写文件,可能导致某次写入时间过长
//从而引起数据丢失,故加入FIFO控制,以解决此问题.
vu8 sairecfifordpos=0;	//FIFO读位置
vu8 sairecfifowrpos=0;	//FIFO写位置
u8 *sairecfifobuf[RB_FIFO_SIZE];//定义10个录音接收FIFO   ////sairecfifobuf是一个二维指针

//读取录音FIFO
//buf:数据缓存区首地址
//返回值:0,没有数据可读;
//      1,读到了1个数据块
u8 rec_sai_fifo_read(u8 **buf)  //这里的buf要注意呦，它是什么？二维指针
{
	if(sairecfifordpos==sairecfifowrpos)return 0;  //fifo中无数据不可读
	sairecfifordpos++;		//读位置加1
	//printf("fR：%d\n",sairecfifordpos);
	if(sairecfifordpos>=RB_FIFO_SIZE)sairecfifordpos=0;//归零 
	*buf=sairecfifobuf[sairecfifordpos]; 
	return 1;
}
//写一个录音FIFO
//buf:数据缓存区首地址
//返回值:0,写入成功;
//      1,写入失败
u8 rec_sai_fifo_write(u8 *buf,u8 cpynum)
{
	u16 i;
	u8 temp=sairecfifowrpos;//记录当前写位置
	sairecfifowrpos++;		//写位置加1
	//printf("fW：%d\n",sairecfifowrpos);
	if(sairecfifowrpos>=RB_FIFO_SIZE)sairecfifowrpos=0;//归零  
	if(sairecfifordpos==sairecfifowrpos)  //说明读得满了，写指针都跟上来了，增加rec_sai_fifo_read读的帧数，
	{
		sairecfifowrpos=temp;//还原原来的写位置,此次写入失败
		return 1;	
	}
	for(i=0;i<cpynum;i++)
	sairecfifobuf[sairecfifowrpos][i]=buf[i];//拷贝数据  //sairecfifobuf是一个二维指针
	return 0;
} 


	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	














