#include "sim900a.h"
#include "usart.h"		
#include "delay.h"	
#include "led.h"   	 
#include "key.h"	 	 	 	 	 
#include "lcd.h" 
#include "w25qxx.h" 	 
#include "touch.h" 	 
#include "malloc.h"
#include "string.h"    
#include "text.h"		
#include "usart3.h" 
#include "ff.h"
#include "usart2.h"

u8 text[]={'1','4','7','2','5'};
//////////////////////////////////////////////////////////////////////////////////	   
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F103开发板 
//ATK-SIM900A GSM/GPRS模块驱动	  
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/4/12
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved
//******************************************************************************** 
//无
/////////////////////////////////////////////////////////////////////////////////// 	
   
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//usmart支持部分
//将收到的AT指令应答数据返回给电脑串口
//mode:0,不清零USART3_RX_STA;
//     1,清零USART3_RX_STA;


void gouzao(u8 c,u8 i)
{
	switch(c)
	{
		case '0':shuju[15+4*i]='0';break;
		case '1':shuju[15+4*i]='1';break;
		case '2':shuju[15+4*i]='2';break;
		case '3':shuju[15+4*i]='3';break;
		case '4':shuju[15+4*i]='4';break;
		case '5':shuju[15+4*i]='5';break;
		case '6':shuju[15+4*i]='6';break;
		case '7':shuju[15+4*i]='7';break;
		case '8':shuju[15+4*i]='8';break;
		case '9':shuju[15+4*i]='9';break;
			
	}
	
}
void sim_at_response(u8 mode)
{
	if(USART3_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;//添加结束符
		printf("%s",USART3_RX_BUF);	//发送到串口
		if(mode)USART3_RX_STA=0;
	} 
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//ATK-SIM900A 各项测试(拨号测试、短信测试、GPRS测试)共用代码

//sim900a发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//    其他,期待应答结果的位置(str的位置)
u8* sim900a_check_cmd(u8 *str)
{
	char *strx=0;
	if(USART3_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;//添加结束符
		strx=strstr((const char*)USART3_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}
//向sim900a发送命令
//cmd:发送的命令字符串(不需要添加回车了),当cmd<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 sim900a_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART3_RX_STA=0;
	if((u32)cmd<=0XFF)
	{
		while((USART3->SR&0X40)==0);//等待上一次数据发送完成  
		USART3->DR=(u32)cmd;
	}else u3_printf("%s\r\n",cmd);//发送命令
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			delay_ms(10);
			if(USART3_RX_STA&0X8000)//接收到期待的应答结果
			{
				if(sim900a_check_cmd(ack))break;//得到有效数据 
				USART3_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
} 
//将1个字符转换为16进制数字
//chr:字符,0~9/A~F/a~F
//返回值:chr对应的16进制数值
u8 sim900a_chr2hex(u8 chr)
{
	if(chr>='0'&&chr<='9')return chr-'0';
	if(chr>='A'&&chr<='F')return (chr-'A'+10);
	if(chr>='a'&&chr<='f')return (chr-'a'+10); 
	return 0;
}
//将1个16进制数字转换为字符
//hex:16进制数字,0~15;
//返回值:字符
u8 sim900a_hex2chr(u8 hex)
{
	if(hex<=9)return hex+'0';
	if(hex>=10&&hex<=15)return (hex-10+'A'); 
	return '0';
}
//unicode gbk 转换函数
//src:输入字符串
//dst:输出(uni2gbk时为gbk内码,gbk2uni时,为unicode字符串)
//mode:0,unicode到gbk转换;
//     1,gbk到unicode转换;
void sim900a_unigbk_exchange(u8 *src,u8 *dst,u8 mode)
{
	u16 temp; 
	u8 buf[2];
	if(mode)//gbk 2 unicode
	{
		while(*src!=0)
		{
			if(*src<0X81)	//非汉字
			{
				temp=(u16)ff_convert((WCHAR)*src,1);
				src++;
			}else 			//汉字,占2个字节
			{
				buf[1]=*src++;
				buf[0]=*src++; 
				temp=(u16)ff_convert((WCHAR)*(u16*)buf,1); 
			}
			*dst++=sim900a_hex2chr((temp>>12)&0X0F);
			*dst++=sim900a_hex2chr((temp>>8)&0X0F);
			*dst++=sim900a_hex2chr((temp>>4)&0X0F);
			*dst++=sim900a_hex2chr(temp&0X0F);
		}
	}else	//unicode 2 gbk
	{ 
		while(*src!=0)
		{
			buf[1]=sim900a_chr2hex(*src++)*16;
			buf[1]+=sim900a_chr2hex(*src++);
			buf[0]=sim900a_chr2hex(*src++)*16;
			buf[0]+=sim900a_chr2hex(*src++);
 			temp=(u16)ff_convert((WCHAR)*(u16*)buf,0);
			if(temp<0X80){*dst=temp;dst++;}
			else {*(u16*)dst=swap16(temp);dst+=2;}
		} 
	}
	*dst=0;//添加结束符
} 
//键盘码表
const u8* kbd_tbl1[13]={"1","2","3","4","5","6","7","8","9","*","0","#","DEL"};
const u8* kbd_tbl2[13]={"1","2","3","4","5","6","7","8","9",".","0","#","DEL"};
u8** kbd_tbl;
u8* kbd_fn_tbl[2];
//加载键盘界面（尺寸为240*140）
//x,y:界面起始坐标（320*240分辨率的时候，x必须为0）
void sim900a_load_keyboard(u16 x,u16 y,u8 **kbtbl)
{
	u16 i;
	POINT_COLOR=RED;
	kbd_tbl=kbtbl;
	LCD_Fill(x,y,x+240,y+140,WHITE);
	LCD_DrawRectangle(x,y,x+240,y+140);						   
	LCD_DrawRectangle(x+80,y,x+160,y+140);	 
	LCD_DrawRectangle(x,y+28,x+240,y+56);
	LCD_DrawRectangle(x,y+84,x+240,y+112);
	POINT_COLOR=BLUE;
	for(i=0;i<15;i++)
	{
		if(i<13)Show_Str_Mid(x+(i%3)*80,y+6+28*(i/3),(u8*)kbd_tbl[i],16,80);
		else Show_Str_Mid(x+(i%3)*80,y+6+28*(i/3),kbd_fn_tbl[i-13],16,80); 
	}  		 					   
}
//按键状态设置
//x,y:键盘坐标
//key:键值（0~8）
//sta:状态，0，松开；1，按下；
void sim900a_key_staset(u16 x,u16 y,u8 keyx,u8 sta)
{		  
	u16 i=keyx/3,j=keyx%3;
	if(keyx>15)return;
	if(sta)LCD_Fill(x+j*80+1,y+i*28+1,x+j*80+78,y+i*28+26,GREEN);
	else LCD_Fill(x+j*80+1,y+i*28+1,x+j*80+78,y+i*28+26,WHITE); 
	if(j&&(i>3))Show_Str_Mid(x+j*80,y+6+28*i,(u8*)kbd_fn_tbl[keyx-13],16,80);
	else Show_Str_Mid(x+j*80,y+6+28*i,(u8*)kbd_tbl[keyx],16,80);		 		 
}
//得到触摸屏的输入
//x,y:键盘坐标
//返回值：按键键值（1~15有效；0,无效）
u8 sim900a_get_keynum(u16 x,u16 y)
{
	u16 i,j;
	static u8 key_x=0;//0,没有任何按键按下；1~15，1~15号按键按下
	u8 key=0;
	tp_dev.scan(0); 		 
	if(tp_dev.sta&TP_PRES_DOWN)			//触摸屏被按下
	{	
		for(i=0;i<5;i++)
		{
			for(j=0;j<3;j++)
			{
			 	if(tp_dev.x[0]<(x+j*80+80)&&tp_dev.x[0]>(x+j*80)&&tp_dev.y[0]<(y+i*28+28)&&tp_dev.y[0]>(y+i*28))
				{	
					key=i*3+j+1;	 
					break;	 		   
				}
			}
			if(key)
			{	   
				if(key_x==key)key=0;
				else 
				{
					sim900a_key_staset(x,y,key_x-1,0);
					key_x=key;
					sim900a_key_staset(x,y,key_x-1,1);
				}
				break;
			}
		}  
	}else if(key_x) 
	{
		sim900a_key_staset(x,y,key_x-1,0);
		key_x=0;
	} 
	return key; 
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//拨号测试部分代码

//sim900a拨号测试
//用于拨打电话和接听电话
//返回值:0,正常
//    其他,错误代码
u8 sim900a_call_test(void)
{
	u8 key;
	u16 lenx;
	u8 callbuf[20]; 
	u8 pohnenumlen=0;	//号码长度,最大15个数 
	u8 *p,*p1,*p2;
	u8 oldmode=0;
	u8 cmode=0;	//模式
				//0:等待拨号
				//1:拨号中
	            //2:通话中
				//3:接收到来电 
	LCD_Clear(WHITE);
	if(sim900a_send_cmd("AT+CLIP=1","OK",200))return 1;	//设置来电显示 
	if(sim900a_send_cmd("AT+COLP=1","OK",200))return 2;	//设置被叫号码显示 
 	p1=mymalloc(SRAMIN,20);								//申请20直接用于存放号码
	if(p1==NULL)return 2;	
	POINT_COLOR=RED;
	Show_Str_Mid(0,30,"ATK-SIM900A 拨号测试",16,240);				    	 
	Show_Str(40,70,200,16,"请拨号:",16,0); 
	kbd_fn_tbl[0]="拨号";
	kbd_fn_tbl[1]="返回"; 
	sim900a_load_keyboard(0,180,(u8**)kbd_tbl1);
	POINT_COLOR=BLUE; 
	while(1)
	{
		delay_ms(10);
		if(USART3_RX_STA&0X8000)		//接收到数据
		{
			sim_at_response(0);
			if(cmode==1||cmode==2)
			{
				if(cmode==1)if(sim900a_check_cmd("+COLP:"))cmode=2;	//拨号成功
				if(sim900a_check_cmd("NO CARRIER"))cmode=0;	//拨号失败
				if(sim900a_check_cmd("NO ANSWER"))cmode=0;	//拨号失败
				if(sim900a_check_cmd("ERROR"))cmode=0;		//拨号失败
			}
			if(sim900a_check_cmd("+CLIP:"))//接收到来电
			{
				cmode=3;
				p=sim900a_check_cmd("+CLIP:");
				p+=8;
				p2=(u8*)strstr((const char *)p,"\"");
				p2[0]=0;//添加结束符 
				strcpy((char*)p1,(char*)p);
			}
			USART3_RX_STA=0;
		}
		key=sim900a_get_keynum(0,180);
		if(key)
		{ 
			if(key<13)
			{
				if(cmode==0&&pohnenumlen<15)
				{ 
					callbuf[pohnenumlen++]=kbd_tbl[key-1][0];
					u3_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]); 
				}else if(cmode==2)//通话中
				{ 
					u3_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]);
					delay_ms(100);
					u3_printf("AT+VTS=%c\r\n",kbd_tbl[key-1][0]); 
					LCD_ShowChar(40+56,90,kbd_tbl[key-1][0],16,0);
				}
			}else
			{
				if(key==13)if(pohnenumlen&&cmode==0)pohnenumlen--;//删除
				if(key==14)//执行拨号
				{
					if(cmode==0)//拨号模式
					{
						callbuf[pohnenumlen]=0;			//最后加入结束符 
						u3_printf("ATD%s;\r\n",callbuf);//拨号
						cmode=1;						//拨号中模式
					}else 
					{
						sim900a_send_cmd("ATH","OK",200);//挂机
						cmode=0;
					}
				}
				if(key==15)
				{
					if(cmode==3)//接收到来电
					{
						sim900a_send_cmd("ATA","OK",200);//发送应答指令
						Show_Str(40+56,70,200,16,callbuf,16,0);
						cmode=2;
					}else
					{
						sim900a_send_cmd("ATH",0,0);//不管有没有在通话,都结束通话
						break;//退出循环
					}
				}
			} 
			if(cmode==0)//只有在等待拨号模式有效
			{
				callbuf[pohnenumlen]=0; 
				LCD_Fill(40+56,70,239,70+16,WHITE);
				Show_Str(40+56,70,200,16,callbuf,16,0);  	
			}				
		}
		if(oldmode!=cmode)//模式变化了
		{
			switch(cmode)
			{
				case 0: 
					kbd_fn_tbl[0]="拨号";
					kbd_fn_tbl[1]="返回"; 
					POINT_COLOR=RED;
					Show_Str(40,70,200,16,"请拨号:",16,0);  
					LCD_Fill(40+56,70,239,70+16,WHITE);
					if(pohnenumlen)
					{
						POINT_COLOR=BLUE;
						Show_Str(40+56,70,200,16,callbuf,16,0);
					}
					break;
				case 1:
					POINT_COLOR=RED;
					Show_Str(40,70,200,16,"拨号中:",16,0); 
					pohnenumlen=0;
				case 2:
					POINT_COLOR=RED;
					if(cmode==2)Show_Str(40,70,200,16,"通话中:",16,0); 
					kbd_fn_tbl[0]="挂断";
					kbd_fn_tbl[1]="返回"; 	
					break;
				case 3:
					POINT_COLOR=RED;
					Show_Str(40,70,200,16,"有来电:",16,0); 
					POINT_COLOR=BLUE;
					Show_Str(40+56,70,200,16,p1,16,0); 
					kbd_fn_tbl[0]="挂断";
					kbd_fn_tbl[1]="接听"; 
					break;				
			}
			if(cmode==2)Show_Str(40,90,200,16,"DTMF音:",16,0);	//通话中,可以通过键盘输入DTMF音
			else LCD_Fill(40,90,120,90+16,WHITE);
			sim900a_load_keyboard(0,180,(u8**)kbd_tbl1);		//显示键盘 
			oldmode=cmode; 
		}
		if((lenx%50)==0)LED0=!LED0; 	    				 
		lenx++;	 
	} 
	myfree(SRAMIN,p1);
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//短信测试部分代码

//SIM900A读短信测试
void sim900a_sms_read_test(void)
{}
//{ 
//	u8 *p,*p1,*p2;
//	u8 timex=0;
//	u8 msgindex[3];
//	u8 msglen=0;
//	u8 msgmaxnum=0;		//短信最大条数
//	u8 key=0;
//	u8 smsreadsta=0;	//是否在短信显示状态
//	p=mymalloc(SRAMIN,200);//申请200个字节的内存
//	LCD_Clear(WHITE); 
//	POINT_COLOR=RED;
//	Show_Str_Mid(0,30,"ATK-SIM900A 读短信测试",16,240);				    	 
//	Show_Str(30,50,200,16,"读取:     总信息:",16,0); 	
//	kbd_fn_tbl[0]="READ";
//	kbd_fn_tbl[1]="RETURN"; 
//	sim900a_load_keyboard(0,180,(u8**)kbd_tbl1);//显示键盘  
//	while(1)
//	{
//		key=sim900a_get_keynum(0,180);
//		if(key)
//		{  
//			if(smsreadsta)
//			{
//				LCD_Fill(30,75,239,179,WHITE);//清除显示的短信内容
//				smsreadsta=0;
//			}
//			if(key<10||key==11)
//			{
//				if(msglen<2)
//				{ 
//					msgindex[msglen++]=kbd_tbl[key-1][0];
//					u3_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]); 
//				} 
//				if(msglen==2)
//				{
//					key=(msgindex[0]-'0')*10+msgindex[1]-'0';
//					if(key>msgmaxnum)
//					{
//						msgindex[0]=msgmaxnum/10+'0';
//						msgindex[1]=msgmaxnum%10+'0';					
//					}
//				} 
//			}else
//			{
//				if(key==13)if(msglen)msglen--;//删除  
//				if(key==14&&msglen)//执行读取短信
//				{ 
//					LCD_Fill(30,75,239,179,WHITE);//清除之前的显示	   	 
//					sprintf((char*)p,"AT+CMGR=%s",msgindex);
//					if(sim900a_send_cmd(p,"+CMGR:",200)==0)//读取短信
//					{
//						POINT_COLOR=RED;
//						Show_Str(30,75,200,12,"状态:",12,0);
//						Show_Str(30+75,75,200,12,"来自:",12,0);
//						Show_Str(30,90,200,12,"接收时间:",12,0);
//						Show_Str(30,105,200,12,"内容:",12,0);
//						POINT_COLOR=BLUE;
//						if(strstr((const char*)(USART3_RX_BUF),"UNREAD")==0)Show_Str(30+30,75,200,12,"已读",12,0);
//						else Show_Str(30+30,75,200,12,"未读",12,0);
//						p1=(u8*)strstr((const char*)(USART3_RX_BUF),",");
//						p2=(u8*)strstr((const char*)(p1+2),"\"");
//						p2[0]=0;//加入结束符
//						sim900a_unigbk_exchange(p1+2,p,0);			//将unicode字符转换为gbk码 
//						Show_Str(30+75+30,75,200,12,p,12,0);		//显示电话号码
//						p1=(u8*)strstr((const char*)(p2+1),"/");
//						p2=(u8*)strstr((const char*)(p1),"+");
//						p2[0]=0;//加入结束符
//						Show_Str(30+54,90,200,12,p1-2,12,0);		//显示接收时间  
//						p1=(u8*)strstr((const char*)(p2+1),"\r");	//寻找回车符
//						sim900a_unigbk_exchange(p1+2,p,0);			//将unicode字符转换为gbk码
//						Show_Str(30+30,105,180,75,p,12,0);			//显示短信内容
//						smsreadsta=1;								//标记有显示短信内容 
//					}else
//					{
//						Show_Str(30,75,200,12,"无短信内容!!!请检查!!",12,0);
//						delay_ms(1000);
//						LCD_Fill(30,75,239,75+12,WHITE);//清除显示
//					}	  
//					USART3_RX_STA=0;
//				}
//				if(key==15)break;
//			} 
//			msgindex[msglen]=0; 
//			LCD_Fill(30+40,50,86,50+16,WHITE);
//			Show_Str(30+40,50,86,16,msgindex,16,0);  	
//		}
//		if(timex==0)		//2.5秒左右更新一次
//		{
//			if(sim900a_send_cmd("AT+CPMS?","+CPMS:",200)==0)	//查询优选消息存储器
//			{ 
//				p1=(u8*)strstr((const char*)(USART3_RX_BUF),","); 
//				p2=(u8*)strstr((const char*)(p1+1),",");
//				p2[0]='/'; 
//				if(p2[3]==',')//小于64K SIM卡，最多存储几十条短信
//				{
//					msgmaxnum=(p2[1]-'0')*10+p2[2]-'0';		//获取最大存储短信条数
//					p2[3]=0;
//				}else //如果是64K SIM卡，则能存储100条以上的信息
//				{
//					msgmaxnum=(p2[1]-'0')*100+(p2[2]-'0')*10+p2[3]-'0';//获取最大存储短信条数
//					p2[4]=0;
//				}
//				sprintf((char*)p,"%s",p1+1);
//				Show_Str(30+17*8,50,200,16,p,16,0);
//				USART3_RX_STA=0;		
//			}
//		}	
//		if((timex%20)==0)LED0=!LED0;//200ms闪烁 
//		timex++;
//		delay_ms(10);
//		if(USART3_RX_STA&0X8000)sim_at_response(1);//检查从GSM模块接收到的数据 
//	}
//	myfree(SRAMIN,p); 
//}


void Usart_SendString(USART_TypeDef * USARTx,u8 *s)
{	
	while(*s!='\0')	
	{ 		
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TC )==RESET);			
		USART_SendData(USARTx,*s);		
		s++;	
	}
}

//测试短信发送内容(70个字[UCS2的时候,1个字符/数字都算1个字])
const u8* sim900a_test_msg="您好，这是一条测试短信，由ATK-SIM900A GSM模块发送，模块购买地址:http://eboard.taobao.com，谢谢支持！";
//SIM900A发短信测试 
void sim900a_sms_send_test(void)
//{
//	u8 *p,*p1,*p2;
//	u8 phonebuf[20]; 		//号码缓存
//	u8 pohnenumlen=0;		//号码长度,最大15个数 
//	u8 timex=0;
//	u8 key=0;
//	u8 smssendsta=0;		//短信发送状态,0,等待发送;1,发送失败;2,发送成功 
//	p=mymalloc(SRAMIN,100);	//申请100个字节的内存,用于存放电话号码的unicode字符串
//	p1=mymalloc(SRAMIN,300);//申请300个字节的内存,用于存放短信的unicode字符串
//	p2=mymalloc(SRAMIN,100);//申请100个字节的内存 存放：AT+CMGS=p1 
//	LCD_Clear(WHITE);  
//	POINT_COLOR=RED;
//	Show_Str_Mid(0,30,"ATK-SIM900A 发短信测试",16,240);				    	 
//	Show_Str(30,50,200,16,"发送给:",16,0); 	 
//	Show_Str(30,70,200,16,"状态:",16,0);
//	Show_Str(30,90,200,16,"内容:",16,0);  
//	POINT_COLOR=BLUE;
//	Show_Str(30+40,70,170,90,"等待发送",16,0);//显示状态	
//	Show_Str(30+40,90,170,90,(u8*)sim900a_test_msg,16,0);//显示短信内容		
//	kbd_fn_tbl[0]="发送";
//	kbd_fn_tbl[1]="返回"; 
//	sim900a_load_keyboard(0,180,(u8**)kbd_tbl1);//显示键盘 
//	while(1)
//	{
//		key=sim900a_get_keynum(0,180);
//		if(key)
//		{   
//			if(smssendsta)
//			{
//				smssendsta=0;
//				Show_Str(30+40,70,170,90,"等待发送",16,0);//显示状态	
//			}
//			if(key<10||key==11)
//			{
//				if(pohnenumlen<15)
//				{ 
//					phonebuf[pohnenumlen++]=kbd_tbl[key-1][0];
//					u3_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]); 
//				}
//			}else
//			{
//				if(key==13)if(pohnenumlen)pohnenumlen--;//删除  
//				if(key==14&&pohnenumlen)				//执行发送短信
//				{  
//					Show_Str(30+40,70,170,90,"正在发送",16,0);			//显示正在发送		
//					smssendsta=1;		 
//					sim900a_unigbk_exchange(phonebuf,p,1);				//将电话号码转换为unicode字符串
//					sim900a_unigbk_exchange((u8*)sim900a_test_msg,p1,1);//将短信内容转换为unicode字符串.
//					sprintf((char*)p2,"AT+CMGS=\"%s\"",p); 
//					if(sim900a_send_cmd(p2,">",200)==0)					//发送短信命令+电话号码
//					{ 		 				 													 
//						u3_printf("%s",p1);		 						//发送短信内容到GSM模块 
// 						if(sim900a_send_cmd((u8*)0X1A,"+CMGS:",1000)==0)smssendsta=2;//发送结束符,等待发送完成(最长等待10秒钟,因为短信长了的话,等待时间会长一些)
//					}  
//					if(smssendsta==1)Show_Str(30+40,70,170,90,"发送失败",16,0);	//显示状态
//					else Show_Str(30+40,70,170,90,"发送成功",16,0);				//显示状态	
//					USART3_RX_STA=0;
//				}
//				if(key==15)break;
//			} 
//			phonebuf[pohnenumlen]=0; 
//			LCD_Fill(30+54,50,239,50+16,WHITE);
//			Show_Str(30+54,50,156,16,phonebuf,16,0);  	
//		}
//		if((timex%20)==0)LED0=!LED0;//200ms闪烁 
//		timex++;
//		delay_ms(10);
//		if(USART3_RX_STA&0X8000)sim_at_response(1);//检查从GSM模块接收到的数据 
//	}
//	myfree(SRAMIN,p);
//	myfree(SRAMIN,p1);
//	myfree(SRAMIN,p2); 
//}
 
{
	while(sim900a_send_cmd("AT+CMGD=1","OK",200));
	while(sim900a_send_cmd("AT+CMGF=1","OK",200));			//设置文本模式 
	while(sim900a_send_cmd("AT+CSCS=\"UCS2\"","OK",200));	//设置TE字符集为UCS2 
	while(sim900a_send_cmd("AT+CSMP=17,0,2,25","OK",200));	//设置短消息文本模式参数 
	
	delay_ms(1000);
	u3_printf("AT+CMGS=\"00310033003800390038003700390031003600310033\"\r\n");  //设置要发送短信的手机号码

	delay_ms(1000);
	delay_ms(1000);
	
//	u3_printf("80014EBA64545012FF0C901F53BB003100320033");  //短信发送的内容
	Usart_SendString(USART3,shuju);  //短信发送的内容
	delay_ms(1000);
	delay_ms(1000);
	
	USART3->DR =0x1A;
	delay_ms(1000);
	Show_Str_Mid(0,70,"OK",16,240); 
	delay_ms(1000);
	LCD_Fill(0,60,320,90,WHITE);
	while(sim900a_send_cmd("AT","OK",200));
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
}


//sms测试主界面
void sim900a_sms_ui(u16 x,u16 y)
{ 
	LCD_Clear(WHITE);
	POINT_COLOR=RED;
	Show_Str_Mid(0,y,"ATK-SIM900A TEST",16,240);  
	Show_Str(x,y+40,200,16,"CHOOSE:",16,0); 				    	 
	Show_Str(x,y+60,200,16,"KEY0:READ",16,0); 				    	 
	Show_Str(x,y+80,200,16,"KEY1:SEND",16,0);				    	 
	Show_Str(x,y+100,200,16,"KEY_UP:RETURN",16,0);
}
//sim900a短信测试
//用于读短信或者发短信
//返回值:0,正常
//    其他,错误代码
u8 sim900a_sms_test(void)
{
	
//	if(sim900a_send_cmd("AT+CMGF=1","OK",200))return 1;			//设置文本模式 
//	if(sim900a_send_cmd("AT+CSCS=\"UCS2\"","OK",200))return 2;	//设置TE字符集为UCS2 
//	if(sim900a_send_cmd("AT+CSMP=17,0,2,25","OK",200))return 3;	//设置短消息文本模式参数 
	while(sim900a_send_cmd("AT+CSCS=\"GSM\"","OK",200)) ;
	while(sim900a_send_cmd("AT+CNMI=2,1","OK",200)) ;	
	return 0;
} 
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
const u8 *modetbl[2]={"TCP","UDP"};//连接模式
//tcp/udp测试
//带心跳功能,以维持连接
//mode:0:TCP测试;1,UDP测试)
//ipaddr:ip地址
//port:端口 
void sim900a_tcpudp_test(u8 mode,u8* ipaddr,u8* port)
{ 
	u8 *p,*p1,*p2,*p3;
	u8 key;
	u16 timex=0;
	u8 count=0;
	const u8* cnttbl[3]={"正在连接","连接成功","连接关闭"};
	u8 connectsta=0;			//0,正在连接;1,连接成功;2,连接关闭; 
	u8 hbeaterrcnt=0;			//心跳错误计数器,连续5次心跳信号无应答,则重新连接
	u8 oldsta=0XFF;
	p=mymalloc(SRAMIN,100);		//申请100字节内存
	p1=mymalloc(SRAMIN,100);	//申请100字节内存
	LCD_Clear(WHITE);  
	POINT_COLOR=RED; 
	if(mode)Show_Str_Mid(0,30,"ATK-SIM900A UDP连接测试",16,240);
	else Show_Str_Mid(0,30,"ATK-SIM900A TCP连接测试",16,240); 
	Show_Str(30,50,200,16,"KEY_UP:退出测试  KEY0:发送数据",12,0); 	
	sprintf((char*)p,"IP地址:%s 端口:%s",ipaddr,port);
	Show_Str(30,65,200,12,p,12,0);			//显示IP地址和端口	
	Show_Str(30,80,200,12,"状态:",12,0); 	//连接状态
	Show_Str(30,100,200,12,"发送数据:",12,0); 	//连接状态
	Show_Str(30,115,200,12,"接收数据:",12,0);	//端口固定为8086
	POINT_COLOR=BLUE;
	USART3_RX_STA=0;
	sprintf((char*)p,"AT+CIPSTART=\"%s\",\"%s\",\"%s\"",modetbl[mode],ipaddr,port);
	if(sim900a_send_cmd(p,"OK",500))return;		//发起连接
	while(1)
	{ 
		key=KEY_Scan(0);
		if(key==WKUP_PRES)//退出测试		 
		{  
			sim900a_send_cmd("AT+CIPCLOSE=1","CLOSE OK",500);	//关闭连接
			sim900a_send_cmd("AT+CIPSHUT","SHUT OK",500);		//关闭移动场景 
			break;												 
		}else if(key==KEY0_PRES&(hbeaterrcnt==0))				//发送数据(心跳正常时发送)
		{
			Show_Str(30+30,80,200,12,"数据发送中...",12,0); 		//提示数据发送中
			if(sim900a_send_cmd("AT+CIPSEND",">",500)==0)		//发送数据
			{ 
 				printf("CIPSEND DATA:%s\r\n",p1);	 			//发送数据打印到串口
				u3_printf("%s\r\n",p1);
				delay_ms(10);
				if(sim900a_send_cmd((u8*)0X1A,"SEND OK",1000)==0)Show_Str(30+30,80,200,12,"数据发送成功!",12,0);//最长等待10s
				else Show_Str(30+30,80,200,12,"数据发送失败!",12,0);
				delay_ms(1000); 
			}else sim900a_send_cmd((u8*)0X1B,0,0);	//ESC,取消发送 
			oldsta=0XFF;			
		}
		if((timex%20)==0)
		{
			LED0=!LED0;
			count++;	
			if(connectsta==2||hbeaterrcnt>8)//连接中断了,或者连续8次心跳没有正确发送成功,则重新连接
			{
				sim900a_send_cmd("AT+CIPCLOSE=1","CLOSE OK",500);	//关闭连接
				sim900a_send_cmd("AT+CIPSHUT","SHUT OK",500);		//关闭移动场景 
				sim900a_send_cmd(p,"OK",500);						//尝试重新连接
				connectsta=0;	
 				hbeaterrcnt=0;
			}
			sprintf((char*)p1,"ATK-SIM900A %s测试 %d  ",modetbl[mode],count);
			Show_Str(30+54,100,200,12,p1,12,0); 
		}
		if(connectsta==0&&(timex%200)==0)//连接还没建立的时候,每2秒查询一次CIPSTATUS.
		{
			sim900a_send_cmd("AT+CIPSTATUS","OK",500);	//查询连接状态
			if(strstr((const char*)USART3_RX_BUF,"CLOSED"))connectsta=2;
			if(strstr((const char*)USART3_RX_BUF,"CONNECT OK"))connectsta=1;
		}
		if(connectsta==1&&timex>=600)//连接正常的时候,每6秒发送一次心跳
		{
			timex=0;
			if(sim900a_send_cmd("AT+CIPSEND",">",200)==0)//发送数据
			{
				sim900a_send_cmd((u8*)0X00,0,0);	//发送数据:0X00  
				delay_ms(20);						//必须加延时
				sim900a_send_cmd((u8*)0X1A,0,0);	//CTRL+Z,结束数据发送,启动一次传输	
			}else sim900a_send_cmd((u8*)0X1B,0,0);	//ESC,取消发送 		
				
			hbeaterrcnt++; 
			printf("hbeaterrcnt:%d\r\n",hbeaterrcnt);//方便调试代码
		} 
		delay_ms(10);
		if(USART3_RX_STA&0X8000)		//接收到一次数据了
		{ 
			USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;	//添加结束符 
			printf("%s",USART3_RX_BUF);				//发送到串口  
			if(hbeaterrcnt)							//需要检测心跳应答
			{
				if(strstr((const char*)USART3_RX_BUF,"SEND OK"))hbeaterrcnt=0;//心跳正常
			}				
			p2=(u8*)strstr((const char*)USART3_RX_BUF,"+IPD");
			if(p2)//接收到TCP/UDP数据
			{
				p3=(u8*)strstr((const char*)p2,",");
				p2=(u8*)strstr((const char*)p2,":");
				p2[0]=0;//加入结束符
				sprintf((char*)p1,"收到%s字节,内容如下",p3+1);//接收到的字节数
				LCD_Fill(30+54,115,239,130,WHITE);
				POINT_COLOR=BRED;
				Show_Str(30+54,115,156,12,p1,12,0); //显示接收到的数据长度
				POINT_COLOR=BLUE;
				LCD_Fill(30,130,210,319,WHITE);
				Show_Str(30,130,180,190,p2+1,12,0); //显示接收到的数据 
			}
			USART3_RX_STA=0;
		}
		if(oldsta!=connectsta)
		{
			oldsta=connectsta;
			LCD_Fill(30+30,80,239,80+12,WHITE);
			Show_Str(30+30,80,200,12,(u8*)cnttbl[connectsta],12,0); //更新状态
		} 
		timex++; 
	} 
	myfree(SRAMIN,p);
	myfree(SRAMIN,p1);
}
//gprs测试主界面
void sim900a_gprs_ui(void)
{
	LCD_Clear(WHITE);  
	POINT_COLOR=RED;
	Show_Str_Mid(0,30,"ATK-SIM900A GPRS通信测试",16,240);	 
	Show_Str(30,50,200,16,"KEY_UP:连接方式切换",16,0); 	 	
	Show_Str(30,90,200,16,"连接方式:",16,0); 	//连接方式通过KEY_UP设置(TCP/UDP)
	Show_Str(30,110,200,16,"IP地址:",16,0);		//IP地址可以键盘设置
	Show_Str(30,130,200,16,"端口:",16,0);		//端口固定为8086
	kbd_fn_tbl[0]="连接";
	kbd_fn_tbl[1]="返回"; 
	sim900a_load_keyboard(0,180,(u8**)kbd_tbl2);//显示键盘 
} 
//sim900a GPRS测试
//用于测试TCP/UDP连接
//返回值:0,正常
//    其他,错误代码
u8 sim900a_gprs_test(void)
{
	const u8 *port="8086";	//端口固定为8086,当你的电脑8086端口被其他程序占用的时候,请修改为其他空闲端口
	u8 mode=0;				//0,TCP连接;1,UDP连接
	u8 key;
	u8 timex=0; 
	u8 ipbuf[16]; 		//IP缓存
	u8 iplen=0;			//IP长度 
	sim900a_gprs_ui();	//加载主界面
	Show_Str(30+72,90,200,16,(u8*)modetbl[mode],16,0);	//显示连接方式	
	Show_Str(30+40,130,200,16,(u8*)port,16,0);			//显示端口 	
 	sim900a_send_cmd("AT+CIPCLOSE=1","CLOSE OK",100);	//关闭连接
	sim900a_send_cmd("AT+CIPSHUT","SHUT OK",100);		//关闭移动场景 
	if(sim900a_send_cmd("AT+CGCLASS=\"B\"","OK",1000))return 1;				//设置GPRS移动台类别为B,支持包交换和数据交换 
	if(sim900a_send_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",1000))return 2;//设置PDP上下文,互联网接协议,接入点等信息
	if(sim900a_send_cmd("AT+CGATT=1","OK",500))return 3;					//附着GPRS业务
	if(sim900a_send_cmd("AT+CIPCSGP=1,\"CMNET\"","OK",500))return 4;	 	//设置为GPRS连接模式
	if(sim900a_send_cmd("AT+CIPHEAD=1","OK",500))return 5;	 				//设置接收数据显示IP头(方便判断数据来源)
	ipbuf[0]=0; 		
	while(1)
	{
		key=KEY_Scan(0);
		if(key==WKUP_PRES)		 
		{  
			mode=!mode;		//连接模式切换
			Show_Str(30+72,90,200,16,(u8*)modetbl[mode],16,0); 	//显示连接模式
		} 
		key=sim900a_get_keynum(0,180);
		if(key)
		{   
			if(key<12)
			{
				if(iplen<15)
				{ 
					ipbuf[iplen++]=kbd_tbl[key-1][0];
					u3_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]); 
				}
			}else
			{
				if(key==13)if(iplen)iplen--;	//删除  
				if(key==14&&iplen)				//执行GPRS连接
				{    
					sim900a_tcpudp_test(mode,ipbuf,(u8*)port);
					sim900a_gprs_ui();			//加载主界面
					Show_Str(30+72,90,200,16,(u8*)modetbl[mode],16,0); 	//显示连接模式
					Show_Str(30+40,130,200,16,(u8*)port,16,0);//显示端口 	
					USART3_RX_STA=0;
				}
				if(key==15)break;
			} 
			ipbuf[iplen]=0; 
			LCD_Fill(30+56,110,239,110+16,WHITE);
			Show_Str(30+56,110,200,16,ipbuf,16,0);			//显示IP地址 	
		} 
		timex++;
		if(timex==20)
		{
			timex=0;
			LED0=!LED0;
		}
		delay_ms(10);
		sim_at_response(1);//检查GSM模块发送过来的数据,及时上传给电脑
	}
	return 0;
} 
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//ATK-SIM900A GSM/GPRS主测试控制部分

//测试界面主UI
void sim900a_mtest_ui(u16 x,u16 y)
{
	u8 *p,*p1,*p2; 
	p=mymalloc(SRAMIN,50);//申请50个字节的内存
	LCD_Clear(WHITE);
	POINT_COLOR=RED;
	Show_Str_Mid(0,y,"ATK-SIM900A 测试程序",16,240);  
	Show_Str(x,y+25,200,16,"请选择:",16,0); 				    	 
	Show_Str(x,y+45,200,16,"KEY0:拨号测试",16,0); 				    	 
	Show_Str(x,y+65,200,16,"KEY1:短信测试",16,0);				    	 
	Show_Str(x,y+85,200,16,"KEY_UP:GPRS测试",16,0);
	POINT_COLOR=BLUE; 	
	USART3_RX_STA=0;
	if(sim900a_send_cmd("AT+CGMI","OK",200)==0)				//查询制造商名称
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF+2),"\r\n");
		p1[0]=0;//加入结束符
		sprintf((char*)p,"制造商:%s",USART3_RX_BUF+2);
		Show_Str(x,y+110,200,16,p,16,0);
		USART3_RX_STA=0;		
	} 
	if(sim900a_send_cmd("AT+CGMM","OK",200)==0)//查询模块名字
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF+2),"\r\n"); 
		p1[0]=0;//加入结束符
		sprintf((char*)p,"模块型号:%s",USART3_RX_BUF+2);
		Show_Str(x,y+130,200,16,p,16,0);
		USART3_RX_STA=0;		
	} 
	if(sim900a_send_cmd("AT+CGSN","OK",200)==0)//查询产品序列号
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF+2),"\r\n");//查找回车
		p1[0]=0;//加入结束符 
		sprintf((char*)p,"序列号:%s",USART3_RX_BUF+2);
		Show_Str(x,y+150,200,16,p,16,0);
		USART3_RX_STA=0;		
	}
	if(sim900a_send_cmd("AT+CNUM","+CNUM",200)==0)			//查询本机号码
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF),",");
		p2=(u8*)strstr((const char*)(p1+2),"\"");
		p2[0]=0;//加入结束符
		sprintf((char*)p,"本机号码:%s",p1+2);
		Show_Str(x,y+170,200,16,p,16,0);
		USART3_RX_STA=0;		
	}
	myfree(SRAMIN,p); 
}
//GSM信息显示(信号质量,电池电量,日期时间)
//返回值:0,正常
//    其他,错误代码
u8 sim900a_gsminfo_show(u16 x,u16 y)
{
	u8 *p,*p1,*p2;
	u8 res=0;
	p=mymalloc(SRAMIN,50);//申请50个字节的内存
	POINT_COLOR=BLUE; 	
	USART3_RX_STA=0;
	if(sim900a_send_cmd("AT+CPIN?","OK",200))res|=1<<0;	//查询SIM卡是否在位 
	USART3_RX_STA=0;  
	if(sim900a_send_cmd("AT+COPS?","OK",200)==0)		//查询运营商名字
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF),"\""); 
		if(p1)//有有效数据
		{
			p2=(u8*)strstr((const char*)(p1+1),"\"");
			p2[0]=0;//加入结束符			
			sprintf((char*)p,"运营商:%s",p1+1);
			Show_Str(x,y,200,16,p,16,0);
		} 
		USART3_RX_STA=0;		
	}else res|=1<<1;
	if(sim900a_send_cmd("AT+CSQ","+CSQ:",200)==0)		//查询信号质量
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF),":");
		p2=(u8*)strstr((const char*)(p1),",");
		p2[0]=0;//加入结束符
		sprintf((char*)p,"信号质量:%s",p1+2);
		Show_Str(x,y+20,200,16,p,16,0);
		USART3_RX_STA=0;		
	}else res|=1<<2;
	if(sim900a_send_cmd("AT+CBC","+CBC:",200)==0)		//查询电池电量
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF),",");
		p2=(u8*)strstr((const char*)(p1+1),",");
		p2[0]=0;p2[5]=0;//加入结束符
		sprintf((char*)p,"电池电量:%s%%  %smV",p1+1,p2+1);
		Show_Str(x,y+40,200,16,p,16,0);
		USART3_RX_STA=0;		
	}else res|=1<<3; 
	if(sim900a_send_cmd("AT+CCLK?","+CCLK:",200)==0)		//查询电池电量
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF),"\"");
		p2=(u8*)strstr((const char*)(p1+1),":");
		p2[3]=0;//加入结束符
		sprintf((char*)p,"日期时间:%s",p1+1);
		Show_Str(x,y+60,200,16,p,16,0);
		USART3_RX_STA=0;		
	}else res|=1<<4; 
	myfree(SRAMIN,p); 
	return res;
} 
//sim900a主测试程序
void sim900a_test(void)
{
	
	while(sim900a_send_cmd("AT","OK",100))//检测是否应答AT指令 
	{
		Show_Str(40,55,200,16,"GSM ERROR!!!",16,0);
		delay_ms(800);
		LCD_Fill(40,55,200,55+16,WHITE);
		Show_Str(40,55,200,16,"GSM ERROR...",16,0);
		delay_ms(400);  
	} 	 
	LCD_Fill(40,55,200,55+16,WHITE);
	while(sim900a_send_cmd("ATE0","OK",200));//不回显
	
	sim900a_sms_test();	//短信测试
						
						
}












