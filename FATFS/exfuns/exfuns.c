#include "string.h"
#include "exfuns.h"
#include "fattester.h"	
#include "malloc.h"
#include "usart.h"
#include "ringbuffer.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//FATFS 扩展代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2016/1/7
//版本：V1.1
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved	
//********************************************************************************
//升级说明
//V1.1
//修正exf_copy函数,文件进度显示错误的bug
////////////////////////////////////////////////////////////////////////////////// 	
////////////////////////////////////////////////////////////////////////////////// 	
//gdy
//FATFS fatsd;
//FIL fileobj;   //文件对象，保存当前操作的文件状况
//FRESULT fr;
//UINT brs;
//DIR dirobj;    //目录对象
//FILINFO fileinfoobj;  //用来保存目录文件信息
/////////////////////////////////////////////

#define FILE_MAX_TYPE_NUM		7	//最多FILE_MAX_TYPE_NUM个大类
#define FILE_MAX_SUBT_NUM		4	//最多FILE_MAX_SUBT_NUM个小类
extern u8 rec_sai_fifo_read(u8 **buf);  //这里的buf要注意呦，它是什么？二维指针

 //文件类型列表
u8*const FILE_TYPE_TBL[FILE_MAX_TYPE_NUM][FILE_MAX_SUBT_NUM]=
{
{"BIN"},			//BIN文件
{"LRC"},			//LRC文件
{"NES","SMS"},		//NES/SMS文件
{"TXT","C","H"},	//文本文件
{"WAV","MP3","APE","FLAC"},//支持的音乐文件
{"BMP","JPG","JPEG","GIF"},//图片文件
{"AVI"},			//视频文件
};
///////////////////////////////公共文件区,使用malloc的时候////////////////////////////////////////////
FATFS *fs[_VOLUMES];//逻辑磁盘工作区.	 
FIL *file;	  		//文件1
FIL *ftemp;	  		//文件2.
UINT br,bw;			//读写变量
FILINFO fileinfo;	//文件信息
DIR dir;  			//目录

u8 *fatbuf;			//SD卡数据缓存区
///////////////////////////////////////////////////////////////////////////////////////
//为exfuns申请内存
//返回值:0,成功
//1,失败
u8 exfuns_init(void)
{
	u8 i;
	for(i=0;i<_VOLUMES;i++)
	{
		fs[i]=(FATFS*)mymalloc(SRAMIN,sizeof(FATFS));	//为磁盘i工作区申请内存	
		if(!fs[i])break;
	}
	file=(FIL*)mymalloc(SRAMIN,sizeof(FIL));		//为file申请内存
	ftemp=(FIL*)mymalloc(SRAMIN,sizeof(FIL));		//为ftemp申请内存
	fatbuf=(u8*)mymalloc(SRAMIN,512);				//为fatbuf申请内存
	if(i==_VOLUMES&&file&&ftemp&&fatbuf)return 0;  //申请有一个失败,即失败.
	else return 1;	
}

//将小写字母转为大写字母,如果是数字,则保持不变.
u8 char_upper(u8 c)
{
	if(c<'A')return c;//数字,保持不变.
	if(c>='a')return c-0x20;//变为大写.
	else return c;//大写,保持不变
}	      
//报告文件的类型
//fname:文件名
//返回值:0XFF,表示无法识别的文件类型编号.
//		 其他,高四位表示所属大类,低四位表示所属小类.
u8 f_typetell(u8 *fname)
{
	u8 tbuf[5];
	u8 *attr='\0';//后缀名
	u8 i=0,j;
	while(i<250)
	{
		i++;
		if(*fname=='\0')break;//偏移到了最后了.
		fname++;
	}
	if(i==250)return 0XFF;//错误的字符串.
 	for(i=0;i<5;i++)//得到后缀名
	{
		fname--;
		if(*fname=='.')
		{
			fname++;
			attr=fname;
			break;
		}
  	}
	strcpy((char *)tbuf,(const char*)attr);//copy
 	for(i=0;i<4;i++)tbuf[i]=char_upper(tbuf[i]);//全部变为大写 
	for(i=0;i<FILE_MAX_TYPE_NUM;i++)	//大类对比
	{
		for(j=0;j<FILE_MAX_SUBT_NUM;j++)//子类对比
		{
			if(*FILE_TYPE_TBL[i][j]==0)break;//此组已经没有可对比的成员了.
			if(strcmp((const char *)FILE_TYPE_TBL[i][j],(const char *)tbuf)==0)//找到了
			{
				return (i<<4)|j;
			}
		}
	}
	return 0XFF;//没找到		 			   
}	 

//得到磁盘剩余容量
//drv:磁盘编号("0:"/"1:")
//total:总容量	 （单位KB）
//free:剩余容量	 （单位KB）
//返回值:0,正常.其他,错误代码
u8 exf_getfree(u8 *drv,u32 *total,u32 *free)
{
	FATFS *fs1;
	u8 res;
    u32 fre_clust=0, fre_sect=0, tot_sect=0;
    //得到磁盘信息及空闲簇数量
    res =(u32)f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
    if(res==0)
	{											   
	    tot_sect=(fs1->n_fatent-2)*fs1->csize;	//得到总扇区数
	    fre_sect=fre_clust*fs1->csize;			//得到空闲扇区数	   
#if _MAX_SS!=512				  				//扇区大小不是512字节,则转换为512字节
		tot_sect*=fs1->ssize/512;
		fre_sect*=fs1->ssize/512;
#endif	  
		*total=tot_sect>>1;	//单位为KB
		*free=fre_sect>>1;	//单位为KB 
 	}
	return res;
}		   
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//文件复制
//注意文件大小不要超过4GB.
//将psrc文件,copy到pdst.
//fcpymsg,函数指针,用于实现拷贝时的信息显示
//        pname:文件/文件夹名
//        pct:百分比
//        mode:
//			[0]:更新文件名
//			[1]:更新百分比pct
//			[2]:更新文件夹
//			[3~7]:保留
//psrc,pdst:源文件和目标文件
//totsize:总大小(当totsize为0的时候,表示仅仅为单个文件拷贝)
//cpdsize:已复制了的大小.
//fwmode:文件写入模式
//0:不覆盖原有的文件
//1:覆盖原有的文件
//返回值:0,正常
//    其他,错误,0XFF,强制退出
u8 exf_copy(u8(*fcpymsg)(u8*pname,u8 pct,u8 mode),u8 *psrc,u8 *pdst,u32 totsize,u32 cpdsize,u8 fwmode)
{
	u8 res;
    u16 br=0;
	u16 bw=0;
	FIL *fsrc=0;
	FIL *fdst=0;
	u8 *fbuf=0;
	u8 curpct=0;
	unsigned long long lcpdsize=cpdsize; 
 	fsrc=(FIL*)mymalloc(SRAMIN,sizeof(FIL));//申请内存
 	fdst=(FIL*)mymalloc(SRAMIN,sizeof(FIL));
	fbuf=(u8*)mymalloc(SRAMIN,8192);
  	if(fsrc==NULL||fdst==NULL||fbuf==NULL)res=100;//前面的值留给fatfs
	else
	{   
		if(fwmode==0)fwmode=FA_CREATE_NEW;//不覆盖
		else fwmode=FA_CREATE_ALWAYS;	  //覆盖存在的文件
		 
	 	res=f_open(fsrc,(const TCHAR*)psrc,FA_READ|FA_OPEN_EXISTING);	//打开只读文件
	 	if(res==0)res=f_open(fdst,(const TCHAR*)pdst,FA_WRITE|fwmode); 	//第一个打开成功,才开始打开第二个
		if(res==0)//两个都打开成功了
		{
			if(totsize==0)//仅仅是单个文件复制
			{
				totsize=fsrc->obj.objsize;
				lcpdsize=0;
				curpct=0;
		 	}else curpct=(lcpdsize*100)/totsize;	//得到新百分比
			fcpymsg(psrc,curpct,0X02);			//更新百分比
			while(res==0)//开始复制
			{
				res=f_read(fsrc,fbuf,8192,(UINT*)&br);	//源头读出512字节
				if(res||br==0)break;
				res=f_write(fdst,fbuf,(UINT)br,(UINT*)&bw);	//写入目的文件
				lcpdsize+=bw;
				if(curpct!=(lcpdsize*100)/totsize)//是否需要更新百分比
				{
					curpct=(lcpdsize*100)/totsize;
					if(fcpymsg(psrc,curpct,0X02))//更新百分比
					{
						res=0XFF;//强制退出
						break;
					}
				}			     
				if(res||bw<br)break;       
			}
		    f_close(fsrc);
		    f_close(fdst);
		}
	}
	myfree(SRAMIN,fsrc);//释放内存
	myfree(SRAMIN,fdst);
	myfree(SRAMIN,fbuf);
	return res;
}

//得到路径下的文件夹
//返回值:0,路径就是个卷标号.
//    其他,文件夹名字首地址
u8* exf_get_src_dname(u8* dpfn)
{
	u16 temp=0;
 	while(*dpfn!=0)
	{
		dpfn++;
		temp++;	
	}
	if(temp<4)return 0; 
	while((*dpfn!=0x5c)&&(*dpfn!=0x2f))dpfn--;	//追述到倒数第一个"\"或者"/"处 
	return ++dpfn;
}
//得到文件夹大小
//注意文件夹大小不要超过4GB.
//返回值:0,文件夹大小为0,或者读取过程中发生了错误.
//    其他,文件夹大小.
u32 exf_fdsize(u8 *fdname)
{
#define MAX_PATHNAME_DEPTH	512+1	//最大目标文件路径+文件名深度
	u8 res=0;	  
    DIR *fddir=0;		//目录
	FILINFO *finfo=0;	//文件信息
	u8 * pathname=0;	//目标文件夹路径+文件名
 	u16 pathlen=0;		//目标路径长度
	u32 fdsize=0;

	fddir=(DIR*)mymalloc(SRAMIN,sizeof(DIR));//申请内存
 	finfo=(FILINFO*)mymalloc(SRAMIN,sizeof(FILINFO));
   	if(fddir==NULL||finfo==NULL)res=100;
	if(res==0)
	{ 
 		pathname=mymalloc(SRAMIN,MAX_PATHNAME_DEPTH);	    
 		if(pathname==NULL)res=101;	   
 		if(res==0)
		{
			pathname[0]=0;	    
			strcat((char*)pathname,(const char*)fdname); //复制路径	
		    res=f_opendir(fddir,(const TCHAR*)fdname); 		//打开源目录
		    if(res==0)//打开目录成功 
			{														   
				while(res==0)//开始复制文件夹里面的东东
				{
			        res=f_readdir(fddir,finfo);						//读取目录下的一个文件
			        if(res!=FR_OK||finfo->fname[0]==0)break;		//错误了/到末尾了,退出
			        if(finfo->fname[0]=='.')continue;     			//忽略上级目录
					if(finfo->fattrib&0X10)//是子目录(文件属性,0X20,归档文件;0X10,子目录;)
					{
 						pathlen=strlen((const char*)pathname);		//得到当前路径的长度
						strcat((char*)pathname,(const char*)"/");	//加斜杠
						strcat((char*)pathname,(const char*)finfo->fname);	//源路径加上子目录名字
 						//printf("\r\nsub folder:%s\r\n",pathname);	//打印子目录名
						fdsize+=exf_fdsize(pathname);				//得到子目录大小,递归调用
						pathname[pathlen]=0;						//加入结束符
					}else fdsize+=finfo->fsize;						//非目录,直接加上文件的大小
						
				} 
		    }	  
  			myfree(SRAMIN,pathname);	     
		}
 	}
	myfree(SRAMIN,fddir);    
	myfree(SRAMIN,finfo);
	if(res)return 0;
	else return fdsize;
}	  
//文件夹复制
//注意文件夹大小不要超过4GB.
//将psrc文件夹,copy到pdst文件夹.
//pdst:必须形如"X:"/"X:XX"/"X:XX/XX"之类的.而且要实现确认上一级文件夹存在
//fcpymsg,函数指针,用于实现拷贝时的信息显示
//        pname:文件/文件夹名
//        pct:百分比
//        mode:
//			[0]:更新文件名
//			[1]:更新百分比pct
//			[2]:更新文件夹
//			[3~7]:保留
//psrc,pdst:源文件夹和目标文件夹
//totsize:总大小(当totsize为0的时候,表示仅仅为单个文件拷贝)
//cpdsize:已复制了的大小.
//fwmode:文件写入模式
//0:不覆盖原有的文件
//1:覆盖原有的文件
//返回值:0,成功
//    其他,错误代码;0XFF,强制退出
u8 exf_fdcopy(u8(*fcpymsg)(u8*pname,u8 pct,u8 mode),u8 *psrc,u8 *pdst,u32 *totsize,u32 *cpdsize,u8 fwmode)
{
#define MAX_PATHNAME_DEPTH	512+1	//最大目标文件路径+文件名深度
	u8 res=0;	  
    DIR *srcdir=0;		//源目录
	DIR *dstdir=0;		//源目录
	FILINFO *finfo=0;	//文件信息
	u8 *fn=0;   		//长文件名

	u8 * dstpathname=0;	//目标文件夹路径+文件名
	u8 * srcpathname=0;	//源文件夹路径+文件名
	
 	u16 dstpathlen=0;	//目标路径长度
 	u16 srcpathlen=0;	//源路径长度

  
	srcdir=(DIR*)mymalloc(SRAMIN,sizeof(DIR));//申请内存
 	dstdir=(DIR*)mymalloc(SRAMIN,sizeof(DIR));
	finfo=(FILINFO*)mymalloc(SRAMIN,sizeof(FILINFO));

   	if(srcdir==NULL||dstdir==NULL||finfo==NULL)res=100;
	if(res==0)
	{ 
 		dstpathname=mymalloc(SRAMIN,MAX_PATHNAME_DEPTH);
		srcpathname=mymalloc(SRAMIN,MAX_PATHNAME_DEPTH);
 		if(dstpathname==NULL||srcpathname==NULL)res=101;	   
 		if(res==0)
		{
			dstpathname[0]=0;
			srcpathname[0]=0;
			strcat((char*)srcpathname,(const char*)psrc); 	//复制原始源文件路径	
			strcat((char*)dstpathname,(const char*)pdst); 	//复制原始目标文件路径	
		    res=f_opendir(srcdir,(const TCHAR*)psrc); 		//打开源目录
		    if(res==0)//打开目录成功 
			{
  				strcat((char*)dstpathname,(const char*)"/");//加入斜杠
 				fn=exf_get_src_dname(psrc);
				if(fn==0)//卷标拷贝
				{
					dstpathlen=strlen((const char*)dstpathname);
					dstpathname[dstpathlen]=psrc[0];	//记录卷标
					dstpathname[dstpathlen+1]=0;		//结束符 
				}else strcat((char*)dstpathname,(const char*)fn);//加文件名		
 				fcpymsg(fn,0,0X04);//更新文件夹名
				res=f_mkdir((const TCHAR*)dstpathname);//如果文件夹已经存在,就不创建.如果不存在就创建新的文件夹.
				if(res==FR_EXIST)res=0;
				while(res==0)//开始复制文件夹里面的东东
				{
			        res=f_readdir(srcdir,finfo);					//读取目录下的一个文件
			        if(res!=FR_OK||finfo->fname[0]==0)break;		//错误了/到末尾了,退出
			        if(finfo->fname[0]=='.')continue;     			//忽略上级目录
					fn=(u8*)finfo->fname; 							//得到文件名
					dstpathlen=strlen((const char*)dstpathname);	//得到当前目标路径的长度
					srcpathlen=strlen((const char*)srcpathname);	//得到源路径长度

					strcat((char*)srcpathname,(const char*)"/");//源路径加斜杠
 					if(finfo->fattrib&0X10)//是子目录(文件属性,0X20,归档文件;0X10,子目录;)
					{
						strcat((char*)srcpathname,(const char*)fn);		//源路径加上子目录名字
						res=exf_fdcopy(fcpymsg,srcpathname,dstpathname,totsize,cpdsize,fwmode);	//拷贝文件夹
					}else //非目录
					{
						strcat((char*)dstpathname,(const char*)"/");//目标路径加斜杠
						strcat((char*)dstpathname,(const char*)fn);	//目标路径加文件名
						strcat((char*)srcpathname,(const char*)fn);	//源路径加文件名
 						fcpymsg(fn,0,0X01);//更新文件名
						res=exf_copy(fcpymsg,srcpathname,dstpathname,*totsize,*cpdsize,fwmode);//复制文件
						*cpdsize+=finfo->fsize;//增加一个文件大小
					}
					srcpathname[srcpathlen]=0;//加入结束符
					dstpathname[dstpathlen]=0;//加入结束符	    
				} 
		    }	  
  			myfree(SRAMIN,dstpathname);
 			myfree(SRAMIN,srcpathname); 
		}
 	}
	myfree(SRAMIN,srcdir);
	myfree(SRAMIN,dstdir);
	myfree(SRAMIN,finfo);
    return res;	  
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//打开路径为filepath的文件，并将文件的内容保存在fdatas中

void printffile(u8 * filepath)
{
FATFS fatsd;
FIL fileobj;   //文件对象，保存当前操作的文件状况
FRESULT fr;
UINT brs;
DIR dirobj;    //目录对象
FILINFO fileinfoobj;  //用来保存目录文件信息
	u8 fdatas[255];   //保存用的数组 
	u32 size=0;
	//1、想打开文件
	fr=f_open(&fileobj,(const TCHAR*)filepath,FA_READ|FA_WRITE);   //（文件对象,打开文件路径，打开的模式）
	if(fr==FR_OK)
	{
		//2、使用文件对象fileobj关联起来，将上面打开的文件里面的数据都出来后保存在fdatas中
		size=f_size(&fileobj);  //获取文件的大小
		f_read(&fileobj,fdatas,size,&brs);    //(文件对象,保存读出数据用的buff，读多少个数据，指向数据个数的指针�)
	}
	f_close(&fileobj);  //3、关闭这个文件对象
	//文件保存在fdatas中
	printf("------------------------\r\n");	
	printf("File Path : %s \r\n",filepath);
	printf("File Content : %s\r\n",fdatas);
	printf("------------------------\r\n");	
}
//从文件的开头进行写datas中的数据，长度length
void testWrite(u8 * filepath,u8 * datas,u32 length)
{
	FATFS fatsd;
	FIL fileobj;   //文件对象，保存当前操作的文件状况
	FRESULT fr;
	UINT brs;
	DIR dirobj;    //目录对象
	FILINFO fileinfoobj;  //用来保存目录文件信息
	fr=f_open(&fileobj,(const TCHAR*)filepath,FA_READ|FA_WRITE);
	if(fr==FR_OK)
	{
		fr=f_write(&fileobj,datas,length,&brs);
	}
	f_close(&fileobj);
	printffile(filepath);  //将写进去的内容打印出来，看写的对不对
}
//从文件指定位置开始写
void testSeek(u8 * filepath,u32 index,u8 *datas,u32 length)
{ 
		FATFS fatsd;
	FIL fileobj;   //文件对象，保存当前操作的文件状况
	FRESULT fr;
	UINT brs;
	DIR dirobj;    //目录对象
	FILINFO fileinfoobj;  //用来保存目录文件信息
	fr=f_open(&fileobj,(const TCHAR*)filepath,FA_READ|FA_WRITE);
	f_lseek(&fileobj, index);//移动光标指针至index位置，此时fileobj中保存了光标指针位置
	// f_lseek(&fileobj,f_size(&fileobj)); //将指针指向文件末尾
	fr=f_write(&fileobj,datas,length,&brs);   //
	f_close(&fileobj);
	printffile(filepath);
}
//读    使用f_gets从文件中读取一个字符串，f_gets是读取字符串，而f_read不区分类型
void testGets(u8 * filepath)
{	
		FATFS fatsd;
	FIL fileobj;   //文件对象，保存当前操作的文件状况
	FRESULT fr;
	UINT brs;
	DIR dirobj;    //目录对象
	FILINFO fileinfoobj;  //用来保存目录文件信息
	u8 fdatas[255];
	u32 size=0;
	fr=f_open(&fileobj,(const TCHAR*)filepath,FA_READ|FA_WRITE);
	size=f_size(&fileobj);
	f_gets((TCHAR*)fdatas,size,&fileobj);  //f_gets读取字符串（保存读出数据用的buff，读多少个数据，文件对象）
	f_close(&fileobj);
	printf("Test Gets: %s \r\n",fdatas);
}
//写一个字符/字符串到文件
void testPutsPutc(u8 * filepath,u8 *sDatas,u8 cdata)  //（待操作文件路径，字符串，字符）
{
		FATFS fatsd;
	FIL fileobj;   //文件对象，保存当前操作的文件状况
	FRESULT fr;
	UINT brs;
	DIR dirobj;    //目录对象
	FILINFO fileinfoobj;  //用来保存目录文件信息
	fr=f_open(&fileobj,(const TCHAR*)filepath,FA_READ|FA_WRITE);
	f_puts((const TCHAR*)sDatas,&fileobj);   //f_puts写一个字符串到文件
	f_putc((TCHAR)cdata,&fileobj);   //f_putc写一个字符到文件
	f_close(&fileobj);
	printffile(filepath);
}
//写   f_printf :   格式化写一个string到文件中
void testFprintf(u8 * filepath)
{
		FATFS fatsd;
	FIL fileobj;   //文件对象，保存当前操作的文件状况
	FRESULT fr;
	UINT brs;
	DIR dirobj;    //目录对象
	FILINFO fileinfoobj;  //用来保存目录文件信息
	fr=f_open(&fileobj,(const TCHAR*)filepath,FA_READ|FA_WRITE);
	f_printf(&fileobj, "%d", 1234);            /* "1234" */   
	f_printf(&fileobj, "%6d,%3d%%", -200, 5);  /* "  -200,  5%" */
    f_printf(&fileobj, "%ld", 12345L);         /* "12345" */
	
	f_close(&fileobj);
	printffile(filepath);
	
}

//目录访问操作的函数   f_opendir    f_readdir       f_closedir 
//文件扫描，打印当前路径dirpath下的文件目录
void testDirScan(u8 * dirpath)
{
		FATFS fatsd;
	FIL fileobj;   //文件对象，保存当前操作的文件状况
	FRESULT fr;
	UINT brs;
	DIR dirobj;    //目录对象
	FILINFO fileinfoobj;  //用来保存目录文件信息
	int index=0;
	fr=f_opendir(&dirobj,(const TCHAR*)dirpath);
	
	if(fr==FR_OK)
	{
		while(1)
		{
			fr=f_readdir(&dirobj,&fileinfoobj);  //（目录对象，保存目录信息）  
			if(fr!=FR_OK||fileinfoobj.fname[0]==0) break;

			 printf("filename %d = %s\r\n",index,(u8 *)fileinfoobj.fname);  //索引号index
			 index++;
		}
		
	}
	f_closedir(&dirobj);
}
//f_mkdir  新建目录     f_unlink 删除目录     f_rename 目录重命名 
void testMK_UNlinkDir(void)
{
		FATFS fatsd;
	FIL fileobj;   //文件对象，保存当前操作的文件状况
	FRESULT fr;
	UINT brs;
	DIR dirobj;    //目录对象
	FILINFO fileinfoobj;  //用来保存目录文件信息
	DIR recdir;	
	f_mkdir("0:/123");   //新建目录文件
	f_mkdir("0:/123/Bedgy");
	f_mkdir("0:/123/DEFRGTH");	
	testDirScan("0:/AAAA");
	f_unlink("0:/AAAA/CCCC");   //f_unlink 删除目录“0:/AAAA/CCCC”
	testDirScan("0:/AAAA");	
	f_rename("0:/AAAA/BBBB","0:/AAAA/BBCC");  //将 前面的路经 重命名为 后面的路径
	testDirScan("0:/AAAA");
}

//文件查找   从目录dirpath中查找文件filenamePattern
void testfindFirst(u8 * dirpath,u8 * filenamePattern)
{
		FATFS fatsd;
	FIL fileobj;   //文件对象，保存当前操作的文件状况
	FRESULT fr;
	UINT brs;
	DIR dirobj;    //目录对象
	FILINFO fileinfoobj;  //用来保存目录文件信息
/*注意，这两个函数使用，必须设置ffconf.h里面的_USE_FIND标识符的值为1 ，也就是：#define _USE_FIND 1  */
 
	fr = f_findfirst(&dirobj, &fileinfoobj, (const TCHAR*)dirpath, (const TCHAR*)filenamePattern);
    while (fr == FR_OK && fileinfoobj.fname[0]) 
	{         /* Repeat while an item is found */
        printf("matched:%s\r\n", fileinfoobj.fname);                /* Display the object name */
        fr = f_findnext(&dirobj, &fileinfoobj);               /* Search for next item */
  }
    f_closedir(&dirobj);
}


//////////////
void sd_creatfile(void)
{
	FIL fileobj;   //文件对象，保存当前操作的文件状况
	DIR recdir;
	FRESULT fr;
	int i;
	char *sDatas ="-------------------system reset-----------------\r\n";
		//init中进行FIFO内存申请
	for(i=0;i<RB_FIFO_SIZE;i++)
	{
		sairecfifobuf[i]=mymalloc(SRAMIN,RB_FIFO_FRAME);//SAI录音FIFO内存申请
		if(sairecfifobuf[i]==NULL)break;			//申请失败
	}
	
		while(f_opendir(&recdir,"0:/SaveData"))//打开文件夹
 	{	
		f_mkdir("0:/SaveData");				//创建该目录   
		//f_open(&fileobj, "0:SaveData/GpsData.txt", FA_CREATE_NEW);			//创建该目录 
		//f_open(&fileobj, "0:SaveData/ImuData.txt", FA_CREATE_NEW);			//创建该目录 	
		f_open(&fileobj, "0:SaveData/GpsImuData.txt", FA_CREATE_NEW);			//创建该目录 			
	}
	//往GpsData.txt中写入重新上电开头
		fr=f_open(&fileobj,"0:SaveData/GpsImuData.txt",FA_READ|FA_WRITE);
	if(fr==FR_OK)
	{
		f_lseek(&fileobj,f_size(&fileobj)); 	//将指针指向文件末尾
		f_puts((const TCHAR*)sDatas,&fileobj);   //f_puts	
	}
	f_close(&fileobj);
  //	for(i=0;i<RB_FIFO_SIZE;i++)myfree(SRAMIN,sairecfifobuf[i]);//SAI录音FIFO内存释放
	
}


//void sd_savegps(u8 * filepath,u8 * datas,u32 length)
void sd_savefifo(u8 * filepath,u8 frame)   //frame 帧
{
	FIL fileobj;   //文件对象，保存当前操作的文件状况
	FRESULT fr;
	UINT brs;
	DIR recdir;
///	
	u8 *pdatabuf;
	u8 res =0,i;
	char aa[10];
	char *p = aa;

	fr=f_open(&fileobj,(const TCHAR*)filepath,FA_READ|FA_WRITE);   //（文件对象,打开文件路径，打开的模式）
	if(fr==FR_OK)
	{
		f_lseek(&fileobj,f_size(&fileobj)); 	//将指针指向文件末尾
		//fr=f_write(&fileobj,datas,length,&brs);
		//f_printf(&fileobj, "%s\n", datas);   //使用f_printf解决了后面乱码的问题
		for(i = 0;i<frame;i++)  //读取数据帧数
		{
			if(rec_sai_fifo_read(&pdatabuf))//读取一次数据,读到数据了,写入文件
				{
					//res=f_write(&fileobj,pdatabuf,52,(UINT*)&brs);//写入文件
					f_printf(&fileobj, "%s\n", pdatabuf); 
					
					memset(pdatabuf,'\0',RB_FIFO_FRAME);  //将读完的数组清空，防止后续使用时，新的较短的字符串无法将后边的旧值覆盖掉
					
					f_lseek(&fileobj,f_size(&fileobj)); 	//将指针指向文件末尾					
				}
		}
	}
		f_close(&fileobj);
}


//sd_saveimu("0:SaveData/GpsData.txt",imu_massage2->imu_data);
void sd_savesl(u8 * filepath,u8 * datas,u32 length)   //保存设定长度数据到SD卡  set length
{
		FIL fileobj;   //文件对象，保存当前操作的文件状况
		FRESULT fr;
		UINT brs;
		DIR recdir;
		fr=f_open(&fileobj,(const TCHAR*)filepath,FA_READ|FA_WRITE);   //（文件对象,打开文件路径，打开的模式）
	if(fr==FR_OK)
	{
		f_lseek(&fileobj,f_size(&fileobj)); 	//将指针指向文件末尾
		fr=f_write(&fileobj,datas,length,&brs);
//		f_printf(&fileobj, " ACC_DATA X: %d,   Y: %d   Z:%d\n",datas[0],datas[1],datas[2]);   //使用f_printf解决了后面乱码的问题
//		f_printf(&fileobj, " GYR_DATA X: %d,   Y: %d   Z:%d\n",datas[3],datas[4],datas[5]);   		
	}
	 //f_sync(&fileobj); 
	f_close(&fileobj);
}









