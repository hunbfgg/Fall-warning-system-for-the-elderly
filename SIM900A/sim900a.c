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
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F103������ 
//ATK-SIM900A GSM/GPRSģ������	  
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2015/4/12
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved
//******************************************************************************** 
//��
/////////////////////////////////////////////////////////////////////////////////// 	
   
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//usmart֧�ֲ���
//���յ���ATָ��Ӧ�����ݷ��ظ����Դ���
//mode:0,������USART3_RX_STA;
//     1,����USART3_RX_STA;


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
	if(USART3_RX_STA&0X8000)		//���յ�һ��������
	{ 
		USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;//��ӽ�����
		printf("%s",USART3_RX_BUF);	//���͵�����
		if(mode)USART3_RX_STA=0;
	} 
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//ATK-SIM900A �������(���Ų��ԡ����Ų��ԡ�GPRS����)���ô���

//sim900a���������,�����յ���Ӧ��
//str:�ڴ���Ӧ����
//����ֵ:0,û�еõ��ڴ���Ӧ����
//    ����,�ڴ�Ӧ������λ��(str��λ��)
u8* sim900a_check_cmd(u8 *str)
{
	char *strx=0;
	if(USART3_RX_STA&0X8000)		//���յ�һ��������
	{ 
		USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;//��ӽ�����
		strx=strstr((const char*)USART3_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}
//��sim900a��������
//cmd:���͵������ַ���(����Ҫ��ӻس���),��cmd<0XFF��ʱ��,��������(���緢��0X1A),���ڵ�ʱ�����ַ���.
//ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       1,����ʧ��
u8 sim900a_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART3_RX_STA=0;
	if((u32)cmd<=0XFF)
	{
		while((USART3->SR&0X40)==0);//�ȴ���һ�����ݷ������  
		USART3->DR=(u32)cmd;
	}else u3_printf("%s\r\n",cmd);//��������
	if(ack&&waittime)		//��Ҫ�ȴ�Ӧ��
	{
		while(--waittime)	//�ȴ�����ʱ
		{
			delay_ms(10);
			if(USART3_RX_STA&0X8000)//���յ��ڴ���Ӧ����
			{
				if(sim900a_check_cmd(ack))break;//�õ���Ч���� 
				USART3_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
} 
//��1���ַ�ת��Ϊ16��������
//chr:�ַ�,0~9/A~F/a~F
//����ֵ:chr��Ӧ��16������ֵ
u8 sim900a_chr2hex(u8 chr)
{
	if(chr>='0'&&chr<='9')return chr-'0';
	if(chr>='A'&&chr<='F')return (chr-'A'+10);
	if(chr>='a'&&chr<='f')return (chr-'a'+10); 
	return 0;
}
//��1��16��������ת��Ϊ�ַ�
//hex:16��������,0~15;
//����ֵ:�ַ�
u8 sim900a_hex2chr(u8 hex)
{
	if(hex<=9)return hex+'0';
	if(hex>=10&&hex<=15)return (hex-10+'A'); 
	return '0';
}
//unicode gbk ת������
//src:�����ַ���
//dst:���(uni2gbkʱΪgbk����,gbk2uniʱ,Ϊunicode�ַ���)
//mode:0,unicode��gbkת��;
//     1,gbk��unicodeת��;
void sim900a_unigbk_exchange(u8 *src,u8 *dst,u8 mode)
{
	u16 temp; 
	u8 buf[2];
	if(mode)//gbk 2 unicode
	{
		while(*src!=0)
		{
			if(*src<0X81)	//�Ǻ���
			{
				temp=(u16)ff_convert((WCHAR)*src,1);
				src++;
			}else 			//����,ռ2���ֽ�
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
	*dst=0;//��ӽ�����
} 
//�������
const u8* kbd_tbl1[13]={"1","2","3","4","5","6","7","8","9","*","0","#","DEL"};
const u8* kbd_tbl2[13]={"1","2","3","4","5","6","7","8","9",".","0","#","DEL"};
u8** kbd_tbl;
u8* kbd_fn_tbl[2];
//���ؼ��̽��棨�ߴ�Ϊ240*140��
//x,y:������ʼ���꣨320*240�ֱ��ʵ�ʱ��x����Ϊ0��
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
//����״̬����
//x,y:��������
//key:��ֵ��0~8��
//sta:״̬��0���ɿ���1�����£�
void sim900a_key_staset(u16 x,u16 y,u8 keyx,u8 sta)
{		  
	u16 i=keyx/3,j=keyx%3;
	if(keyx>15)return;
	if(sta)LCD_Fill(x+j*80+1,y+i*28+1,x+j*80+78,y+i*28+26,GREEN);
	else LCD_Fill(x+j*80+1,y+i*28+1,x+j*80+78,y+i*28+26,WHITE); 
	if(j&&(i>3))Show_Str_Mid(x+j*80,y+6+28*i,(u8*)kbd_fn_tbl[keyx-13],16,80);
	else Show_Str_Mid(x+j*80,y+6+28*i,(u8*)kbd_tbl[keyx],16,80);		 		 
}
//�õ�������������
//x,y:��������
//����ֵ��������ֵ��1~15��Ч��0,��Ч��
u8 sim900a_get_keynum(u16 x,u16 y)
{
	u16 i,j;
	static u8 key_x=0;//0,û���κΰ������£�1~15��1~15�Ű�������
	u8 key=0;
	tp_dev.scan(0); 		 
	if(tp_dev.sta&TP_PRES_DOWN)			//������������
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
//���Ų��Բ��ִ���

//sim900a���Ų���
//���ڲ���绰�ͽ����绰
//����ֵ:0,����
//    ����,�������
u8 sim900a_call_test(void)
{
	u8 key;
	u16 lenx;
	u8 callbuf[20]; 
	u8 pohnenumlen=0;	//���볤��,���15���� 
	u8 *p,*p1,*p2;
	u8 oldmode=0;
	u8 cmode=0;	//ģʽ
				//0:�ȴ�����
				//1:������
	            //2:ͨ����
				//3:���յ����� 
	LCD_Clear(WHITE);
	if(sim900a_send_cmd("AT+CLIP=1","OK",200))return 1;	//����������ʾ 
	if(sim900a_send_cmd("AT+COLP=1","OK",200))return 2;	//���ñ��к�����ʾ 
 	p1=mymalloc(SRAMIN,20);								//����20ֱ�����ڴ�ź���
	if(p1==NULL)return 2;	
	POINT_COLOR=RED;
	Show_Str_Mid(0,30,"ATK-SIM900A ���Ų���",16,240);				    	 
	Show_Str(40,70,200,16,"�벦��:",16,0); 
	kbd_fn_tbl[0]="����";
	kbd_fn_tbl[1]="����"; 
	sim900a_load_keyboard(0,180,(u8**)kbd_tbl1);
	POINT_COLOR=BLUE; 
	while(1)
	{
		delay_ms(10);
		if(USART3_RX_STA&0X8000)		//���յ�����
		{
			sim_at_response(0);
			if(cmode==1||cmode==2)
			{
				if(cmode==1)if(sim900a_check_cmd("+COLP:"))cmode=2;	//���ųɹ�
				if(sim900a_check_cmd("NO CARRIER"))cmode=0;	//����ʧ��
				if(sim900a_check_cmd("NO ANSWER"))cmode=0;	//����ʧ��
				if(sim900a_check_cmd("ERROR"))cmode=0;		//����ʧ��
			}
			if(sim900a_check_cmd("+CLIP:"))//���յ�����
			{
				cmode=3;
				p=sim900a_check_cmd("+CLIP:");
				p+=8;
				p2=(u8*)strstr((const char *)p,"\"");
				p2[0]=0;//��ӽ����� 
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
				}else if(cmode==2)//ͨ����
				{ 
					u3_printf("AT+CLDTMF=2,\"%c\"\r\n",kbd_tbl[key-1][0]);
					delay_ms(100);
					u3_printf("AT+VTS=%c\r\n",kbd_tbl[key-1][0]); 
					LCD_ShowChar(40+56,90,kbd_tbl[key-1][0],16,0);
				}
			}else
			{
				if(key==13)if(pohnenumlen&&cmode==0)pohnenumlen--;//ɾ��
				if(key==14)//ִ�в���
				{
					if(cmode==0)//����ģʽ
					{
						callbuf[pohnenumlen]=0;			//����������� 
						u3_printf("ATD%s;\r\n",callbuf);//����
						cmode=1;						//������ģʽ
					}else 
					{
						sim900a_send_cmd("ATH","OK",200);//�һ�
						cmode=0;
					}
				}
				if(key==15)
				{
					if(cmode==3)//���յ�����
					{
						sim900a_send_cmd("ATA","OK",200);//����Ӧ��ָ��
						Show_Str(40+56,70,200,16,callbuf,16,0);
						cmode=2;
					}else
					{
						sim900a_send_cmd("ATH",0,0);//������û����ͨ��,������ͨ��
						break;//�˳�ѭ��
					}
				}
			} 
			if(cmode==0)//ֻ���ڵȴ�����ģʽ��Ч
			{
				callbuf[pohnenumlen]=0; 
				LCD_Fill(40+56,70,239,70+16,WHITE);
				Show_Str(40+56,70,200,16,callbuf,16,0);  	
			}				
		}
		if(oldmode!=cmode)//ģʽ�仯��
		{
			switch(cmode)
			{
				case 0: 
					kbd_fn_tbl[0]="����";
					kbd_fn_tbl[1]="����"; 
					POINT_COLOR=RED;
					Show_Str(40,70,200,16,"�벦��:",16,0);  
					LCD_Fill(40+56,70,239,70+16,WHITE);
					if(pohnenumlen)
					{
						POINT_COLOR=BLUE;
						Show_Str(40+56,70,200,16,callbuf,16,0);
					}
					break;
				case 1:
					POINT_COLOR=RED;
					Show_Str(40,70,200,16,"������:",16,0); 
					pohnenumlen=0;
				case 2:
					POINT_COLOR=RED;
					if(cmode==2)Show_Str(40,70,200,16,"ͨ����:",16,0); 
					kbd_fn_tbl[0]="�Ҷ�";
					kbd_fn_tbl[1]="����"; 	
					break;
				case 3:
					POINT_COLOR=RED;
					Show_Str(40,70,200,16,"������:",16,0); 
					POINT_COLOR=BLUE;
					Show_Str(40+56,70,200,16,p1,16,0); 
					kbd_fn_tbl[0]="�Ҷ�";
					kbd_fn_tbl[1]="����"; 
					break;				
			}
			if(cmode==2)Show_Str(40,90,200,16,"DTMF��:",16,0);	//ͨ����,����ͨ����������DTMF��
			else LCD_Fill(40,90,120,90+16,WHITE);
			sim900a_load_keyboard(0,180,(u8**)kbd_tbl1);		//��ʾ���� 
			oldmode=cmode; 
		}
		if((lenx%50)==0)LED0=!LED0; 	    				 
		lenx++;	 
	} 
	myfree(SRAMIN,p1);
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//���Ų��Բ��ִ���

//SIM900A�����Ų���
void sim900a_sms_read_test(void)
{}
//{ 
//	u8 *p,*p1,*p2;
//	u8 timex=0;
//	u8 msgindex[3];
//	u8 msglen=0;
//	u8 msgmaxnum=0;		//�����������
//	u8 key=0;
//	u8 smsreadsta=0;	//�Ƿ��ڶ�����ʾ״̬
//	p=mymalloc(SRAMIN,200);//����200���ֽڵ��ڴ�
//	LCD_Clear(WHITE); 
//	POINT_COLOR=RED;
//	Show_Str_Mid(0,30,"ATK-SIM900A �����Ų���",16,240);				    	 
//	Show_Str(30,50,200,16,"��ȡ:     ����Ϣ:",16,0); 	
//	kbd_fn_tbl[0]="READ";
//	kbd_fn_tbl[1]="RETURN"; 
//	sim900a_load_keyboard(0,180,(u8**)kbd_tbl1);//��ʾ����  
//	while(1)
//	{
//		key=sim900a_get_keynum(0,180);
//		if(key)
//		{  
//			if(smsreadsta)
//			{
//				LCD_Fill(30,75,239,179,WHITE);//�����ʾ�Ķ�������
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
//				if(key==13)if(msglen)msglen--;//ɾ��  
//				if(key==14&&msglen)//ִ�ж�ȡ����
//				{ 
//					LCD_Fill(30,75,239,179,WHITE);//���֮ǰ����ʾ	   	 
//					sprintf((char*)p,"AT+CMGR=%s",msgindex);
//					if(sim900a_send_cmd(p,"+CMGR:",200)==0)//��ȡ����
//					{
//						POINT_COLOR=RED;
//						Show_Str(30,75,200,12,"״̬:",12,0);
//						Show_Str(30+75,75,200,12,"����:",12,0);
//						Show_Str(30,90,200,12,"����ʱ��:",12,0);
//						Show_Str(30,105,200,12,"����:",12,0);
//						POINT_COLOR=BLUE;
//						if(strstr((const char*)(USART3_RX_BUF),"UNREAD")==0)Show_Str(30+30,75,200,12,"�Ѷ�",12,0);
//						else Show_Str(30+30,75,200,12,"δ��",12,0);
//						p1=(u8*)strstr((const char*)(USART3_RX_BUF),",");
//						p2=(u8*)strstr((const char*)(p1+2),"\"");
//						p2[0]=0;//���������
//						sim900a_unigbk_exchange(p1+2,p,0);			//��unicode�ַ�ת��Ϊgbk�� 
//						Show_Str(30+75+30,75,200,12,p,12,0);		//��ʾ�绰����
//						p1=(u8*)strstr((const char*)(p2+1),"/");
//						p2=(u8*)strstr((const char*)(p1),"+");
//						p2[0]=0;//���������
//						Show_Str(30+54,90,200,12,p1-2,12,0);		//��ʾ����ʱ��  
//						p1=(u8*)strstr((const char*)(p2+1),"\r");	//Ѱ�һس���
//						sim900a_unigbk_exchange(p1+2,p,0);			//��unicode�ַ�ת��Ϊgbk��
//						Show_Str(30+30,105,180,75,p,12,0);			//��ʾ��������
//						smsreadsta=1;								//�������ʾ�������� 
//					}else
//					{
//						Show_Str(30,75,200,12,"�޶�������!!!����!!",12,0);
//						delay_ms(1000);
//						LCD_Fill(30,75,239,75+12,WHITE);//�����ʾ
//					}	  
//					USART3_RX_STA=0;
//				}
//				if(key==15)break;
//			} 
//			msgindex[msglen]=0; 
//			LCD_Fill(30+40,50,86,50+16,WHITE);
//			Show_Str(30+40,50,86,16,msgindex,16,0);  	
//		}
//		if(timex==0)		//2.5�����Ҹ���һ��
//		{
//			if(sim900a_send_cmd("AT+CPMS?","+CPMS:",200)==0)	//��ѯ��ѡ��Ϣ�洢��
//			{ 
//				p1=(u8*)strstr((const char*)(USART3_RX_BUF),","); 
//				p2=(u8*)strstr((const char*)(p1+1),",");
//				p2[0]='/'; 
//				if(p2[3]==',')//С��64K SIM�������洢��ʮ������
//				{
//					msgmaxnum=(p2[1]-'0')*10+p2[2]-'0';		//��ȡ���洢��������
//					p2[3]=0;
//				}else //�����64K SIM�������ܴ洢100�����ϵ���Ϣ
//				{
//					msgmaxnum=(p2[1]-'0')*100+(p2[2]-'0')*10+p2[3]-'0';//��ȡ���洢��������
//					p2[4]=0;
//				}
//				sprintf((char*)p,"%s",p1+1);
//				Show_Str(30+17*8,50,200,16,p,16,0);
//				USART3_RX_STA=0;		
//			}
//		}	
//		if((timex%20)==0)LED0=!LED0;//200ms��˸ 
//		timex++;
//		delay_ms(10);
//		if(USART3_RX_STA&0X8000)sim_at_response(1);//����GSMģ����յ������� 
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

//���Զ��ŷ�������(70����[UCS2��ʱ��,1���ַ�/���ֶ���1����])
const u8* sim900a_test_msg="���ã�����һ�����Զ��ţ���ATK-SIM900A GSMģ�鷢�ͣ�ģ�鹺���ַ:http://eboard.taobao.com��лл֧�֣�";
//SIM900A�����Ų��� 
void sim900a_sms_send_test(void)
//{
//	u8 *p,*p1,*p2;
//	u8 phonebuf[20]; 		//���뻺��
//	u8 pohnenumlen=0;		//���볤��,���15���� 
//	u8 timex=0;
//	u8 key=0;
//	u8 smssendsta=0;		//���ŷ���״̬,0,�ȴ�����;1,����ʧ��;2,���ͳɹ� 
//	p=mymalloc(SRAMIN,100);	//����100���ֽڵ��ڴ�,���ڴ�ŵ绰�����unicode�ַ���
//	p1=mymalloc(SRAMIN,300);//����300���ֽڵ��ڴ�,���ڴ�Ŷ��ŵ�unicode�ַ���
//	p2=mymalloc(SRAMIN,100);//����100���ֽڵ��ڴ� ��ţ�AT+CMGS=p1 
//	LCD_Clear(WHITE);  
//	POINT_COLOR=RED;
//	Show_Str_Mid(0,30,"ATK-SIM900A �����Ų���",16,240);				    	 
//	Show_Str(30,50,200,16,"���͸�:",16,0); 	 
//	Show_Str(30,70,200,16,"״̬:",16,0);
//	Show_Str(30,90,200,16,"����:",16,0);  
//	POINT_COLOR=BLUE;
//	Show_Str(30+40,70,170,90,"�ȴ�����",16,0);//��ʾ״̬	
//	Show_Str(30+40,90,170,90,(u8*)sim900a_test_msg,16,0);//��ʾ��������		
//	kbd_fn_tbl[0]="����";
//	kbd_fn_tbl[1]="����"; 
//	sim900a_load_keyboard(0,180,(u8**)kbd_tbl1);//��ʾ���� 
//	while(1)
//	{
//		key=sim900a_get_keynum(0,180);
//		if(key)
//		{   
//			if(smssendsta)
//			{
//				smssendsta=0;
//				Show_Str(30+40,70,170,90,"�ȴ�����",16,0);//��ʾ״̬	
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
//				if(key==13)if(pohnenumlen)pohnenumlen--;//ɾ��  
//				if(key==14&&pohnenumlen)				//ִ�з��Ͷ���
//				{  
//					Show_Str(30+40,70,170,90,"���ڷ���",16,0);			//��ʾ���ڷ���		
//					smssendsta=1;		 
//					sim900a_unigbk_exchange(phonebuf,p,1);				//���绰����ת��Ϊunicode�ַ���
//					sim900a_unigbk_exchange((u8*)sim900a_test_msg,p1,1);//����������ת��Ϊunicode�ַ���.
//					sprintf((char*)p2,"AT+CMGS=\"%s\"",p); 
//					if(sim900a_send_cmd(p2,">",200)==0)					//���Ͷ�������+�绰����
//					{ 		 				 													 
//						u3_printf("%s",p1);		 						//���Ͷ������ݵ�GSMģ�� 
// 						if(sim900a_send_cmd((u8*)0X1A,"+CMGS:",1000)==0)smssendsta=2;//���ͽ�����,�ȴ��������(��ȴ�10����,��Ϊ���ų��˵Ļ�,�ȴ�ʱ��᳤һЩ)
//					}  
//					if(smssendsta==1)Show_Str(30+40,70,170,90,"����ʧ��",16,0);	//��ʾ״̬
//					else Show_Str(30+40,70,170,90,"���ͳɹ�",16,0);				//��ʾ״̬	
//					USART3_RX_STA=0;
//				}
//				if(key==15)break;
//			} 
//			phonebuf[pohnenumlen]=0; 
//			LCD_Fill(30+54,50,239,50+16,WHITE);
//			Show_Str(30+54,50,156,16,phonebuf,16,0);  	
//		}
//		if((timex%20)==0)LED0=!LED0;//200ms��˸ 
//		timex++;
//		delay_ms(10);
//		if(USART3_RX_STA&0X8000)sim_at_response(1);//����GSMģ����յ������� 
//	}
//	myfree(SRAMIN,p);
//	myfree(SRAMIN,p1);
//	myfree(SRAMIN,p2); 
//}
 
{
	while(sim900a_send_cmd("AT+CMGD=1","OK",200));
	while(sim900a_send_cmd("AT+CMGF=1","OK",200));			//�����ı�ģʽ 
	while(sim900a_send_cmd("AT+CSCS=\"UCS2\"","OK",200));	//����TE�ַ���ΪUCS2 
	while(sim900a_send_cmd("AT+CSMP=17,0,2,25","OK",200));	//���ö���Ϣ�ı�ģʽ���� 
	
	delay_ms(1000);
	u3_printf("AT+CMGS=\"00310033003800390038003700390031003600310033\"\r\n");  //����Ҫ���Ͷ��ŵ��ֻ�����

	delay_ms(1000);
	delay_ms(1000);
	
//	u3_printf("80014EBA64545012FF0C901F53BB003100320033");  //���ŷ��͵�����
	Usart_SendString(USART3,shuju);  //���ŷ��͵�����
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


//sms����������
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
//sim900a���Ų���
//���ڶ����Ż��߷�����
//����ֵ:0,����
//    ����,�������
u8 sim900a_sms_test(void)
{
	
//	if(sim900a_send_cmd("AT+CMGF=1","OK",200))return 1;			//�����ı�ģʽ 
//	if(sim900a_send_cmd("AT+CSCS=\"UCS2\"","OK",200))return 2;	//����TE�ַ���ΪUCS2 
//	if(sim900a_send_cmd("AT+CSMP=17,0,2,25","OK",200))return 3;	//���ö���Ϣ�ı�ģʽ���� 
	while(sim900a_send_cmd("AT+CSCS=\"GSM\"","OK",200)) ;
	while(sim900a_send_cmd("AT+CNMI=2,1","OK",200)) ;	
	return 0;
} 
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
const u8 *modetbl[2]={"TCP","UDP"};//����ģʽ
//tcp/udp����
//����������,��ά������
//mode:0:TCP����;1,UDP����)
//ipaddr:ip��ַ
//port:�˿� 
void sim900a_tcpudp_test(u8 mode,u8* ipaddr,u8* port)
{ 
	u8 *p,*p1,*p2,*p3;
	u8 key;
	u16 timex=0;
	u8 count=0;
	const u8* cnttbl[3]={"��������","���ӳɹ�","���ӹر�"};
	u8 connectsta=0;			//0,��������;1,���ӳɹ�;2,���ӹر�; 
	u8 hbeaterrcnt=0;			//�������������,����5�������ź���Ӧ��,����������
	u8 oldsta=0XFF;
	p=mymalloc(SRAMIN,100);		//����100�ֽ��ڴ�
	p1=mymalloc(SRAMIN,100);	//����100�ֽ��ڴ�
	LCD_Clear(WHITE);  
	POINT_COLOR=RED; 
	if(mode)Show_Str_Mid(0,30,"ATK-SIM900A UDP���Ӳ���",16,240);
	else Show_Str_Mid(0,30,"ATK-SIM900A TCP���Ӳ���",16,240); 
	Show_Str(30,50,200,16,"KEY_UP:�˳�����  KEY0:��������",12,0); 	
	sprintf((char*)p,"IP��ַ:%s �˿�:%s",ipaddr,port);
	Show_Str(30,65,200,12,p,12,0);			//��ʾIP��ַ�Ͷ˿�	
	Show_Str(30,80,200,12,"״̬:",12,0); 	//����״̬
	Show_Str(30,100,200,12,"��������:",12,0); 	//����״̬
	Show_Str(30,115,200,12,"��������:",12,0);	//�˿ڹ̶�Ϊ8086
	POINT_COLOR=BLUE;
	USART3_RX_STA=0;
	sprintf((char*)p,"AT+CIPSTART=\"%s\",\"%s\",\"%s\"",modetbl[mode],ipaddr,port);
	if(sim900a_send_cmd(p,"OK",500))return;		//��������
	while(1)
	{ 
		key=KEY_Scan(0);
		if(key==WKUP_PRES)//�˳�����		 
		{  
			sim900a_send_cmd("AT+CIPCLOSE=1","CLOSE OK",500);	//�ر�����
			sim900a_send_cmd("AT+CIPSHUT","SHUT OK",500);		//�ر��ƶ����� 
			break;												 
		}else if(key==KEY0_PRES&(hbeaterrcnt==0))				//��������(��������ʱ����)
		{
			Show_Str(30+30,80,200,12,"���ݷ�����...",12,0); 		//��ʾ���ݷ�����
			if(sim900a_send_cmd("AT+CIPSEND",">",500)==0)		//��������
			{ 
 				printf("CIPSEND DATA:%s\r\n",p1);	 			//�������ݴ�ӡ������
				u3_printf("%s\r\n",p1);
				delay_ms(10);
				if(sim900a_send_cmd((u8*)0X1A,"SEND OK",1000)==0)Show_Str(30+30,80,200,12,"���ݷ��ͳɹ�!",12,0);//��ȴ�10s
				else Show_Str(30+30,80,200,12,"���ݷ���ʧ��!",12,0);
				delay_ms(1000); 
			}else sim900a_send_cmd((u8*)0X1B,0,0);	//ESC,ȡ������ 
			oldsta=0XFF;			
		}
		if((timex%20)==0)
		{
			LED0=!LED0;
			count++;	
			if(connectsta==2||hbeaterrcnt>8)//�����ж���,��������8������û����ȷ���ͳɹ�,����������
			{
				sim900a_send_cmd("AT+CIPCLOSE=1","CLOSE OK",500);	//�ر�����
				sim900a_send_cmd("AT+CIPSHUT","SHUT OK",500);		//�ر��ƶ����� 
				sim900a_send_cmd(p,"OK",500);						//������������
				connectsta=0;	
 				hbeaterrcnt=0;
			}
			sprintf((char*)p1,"ATK-SIM900A %s���� %d  ",modetbl[mode],count);
			Show_Str(30+54,100,200,12,p1,12,0); 
		}
		if(connectsta==0&&(timex%200)==0)//���ӻ�û������ʱ��,ÿ2���ѯһ��CIPSTATUS.
		{
			sim900a_send_cmd("AT+CIPSTATUS","OK",500);	//��ѯ����״̬
			if(strstr((const char*)USART3_RX_BUF,"CLOSED"))connectsta=2;
			if(strstr((const char*)USART3_RX_BUF,"CONNECT OK"))connectsta=1;
		}
		if(connectsta==1&&timex>=600)//����������ʱ��,ÿ6�뷢��һ������
		{
			timex=0;
			if(sim900a_send_cmd("AT+CIPSEND",">",200)==0)//��������
			{
				sim900a_send_cmd((u8*)0X00,0,0);	//��������:0X00  
				delay_ms(20);						//�������ʱ
				sim900a_send_cmd((u8*)0X1A,0,0);	//CTRL+Z,�������ݷ���,����һ�δ���	
			}else sim900a_send_cmd((u8*)0X1B,0,0);	//ESC,ȡ������ 		
				
			hbeaterrcnt++; 
			printf("hbeaterrcnt:%d\r\n",hbeaterrcnt);//������Դ���
		} 
		delay_ms(10);
		if(USART3_RX_STA&0X8000)		//���յ�һ��������
		{ 
			USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;	//��ӽ����� 
			printf("%s",USART3_RX_BUF);				//���͵�����  
			if(hbeaterrcnt)							//��Ҫ�������Ӧ��
			{
				if(strstr((const char*)USART3_RX_BUF,"SEND OK"))hbeaterrcnt=0;//��������
			}				
			p2=(u8*)strstr((const char*)USART3_RX_BUF,"+IPD");
			if(p2)//���յ�TCP/UDP����
			{
				p3=(u8*)strstr((const char*)p2,",");
				p2=(u8*)strstr((const char*)p2,":");
				p2[0]=0;//���������
				sprintf((char*)p1,"�յ�%s�ֽ�,��������",p3+1);//���յ����ֽ���
				LCD_Fill(30+54,115,239,130,WHITE);
				POINT_COLOR=BRED;
				Show_Str(30+54,115,156,12,p1,12,0); //��ʾ���յ������ݳ���
				POINT_COLOR=BLUE;
				LCD_Fill(30,130,210,319,WHITE);
				Show_Str(30,130,180,190,p2+1,12,0); //��ʾ���յ������� 
			}
			USART3_RX_STA=0;
		}
		if(oldsta!=connectsta)
		{
			oldsta=connectsta;
			LCD_Fill(30+30,80,239,80+12,WHITE);
			Show_Str(30+30,80,200,12,(u8*)cnttbl[connectsta],12,0); //����״̬
		} 
		timex++; 
	} 
	myfree(SRAMIN,p);
	myfree(SRAMIN,p1);
}
//gprs����������
void sim900a_gprs_ui(void)
{
	LCD_Clear(WHITE);  
	POINT_COLOR=RED;
	Show_Str_Mid(0,30,"ATK-SIM900A GPRSͨ�Ų���",16,240);	 
	Show_Str(30,50,200,16,"KEY_UP:���ӷ�ʽ�л�",16,0); 	 	
	Show_Str(30,90,200,16,"���ӷ�ʽ:",16,0); 	//���ӷ�ʽͨ��KEY_UP����(TCP/UDP)
	Show_Str(30,110,200,16,"IP��ַ:",16,0);		//IP��ַ���Լ�������
	Show_Str(30,130,200,16,"�˿�:",16,0);		//�˿ڹ̶�Ϊ8086
	kbd_fn_tbl[0]="����";
	kbd_fn_tbl[1]="����"; 
	sim900a_load_keyboard(0,180,(u8**)kbd_tbl2);//��ʾ���� 
} 
//sim900a GPRS����
//���ڲ���TCP/UDP����
//����ֵ:0,����
//    ����,�������
u8 sim900a_gprs_test(void)
{
	const u8 *port="8086";	//�˿ڹ̶�Ϊ8086,����ĵ���8086�˿ڱ���������ռ�õ�ʱ��,���޸�Ϊ�������ж˿�
	u8 mode=0;				//0,TCP����;1,UDP����
	u8 key;
	u8 timex=0; 
	u8 ipbuf[16]; 		//IP����
	u8 iplen=0;			//IP���� 
	sim900a_gprs_ui();	//����������
	Show_Str(30+72,90,200,16,(u8*)modetbl[mode],16,0);	//��ʾ���ӷ�ʽ	
	Show_Str(30+40,130,200,16,(u8*)port,16,0);			//��ʾ�˿� 	
 	sim900a_send_cmd("AT+CIPCLOSE=1","CLOSE OK",100);	//�ر�����
	sim900a_send_cmd("AT+CIPSHUT","SHUT OK",100);		//�ر��ƶ����� 
	if(sim900a_send_cmd("AT+CGCLASS=\"B\"","OK",1000))return 1;				//����GPRS�ƶ�̨���ΪB,֧�ְ����������ݽ��� 
	if(sim900a_send_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",1000))return 2;//����PDP������,��������Э��,��������Ϣ
	if(sim900a_send_cmd("AT+CGATT=1","OK",500))return 3;					//����GPRSҵ��
	if(sim900a_send_cmd("AT+CIPCSGP=1,\"CMNET\"","OK",500))return 4;	 	//����ΪGPRS����ģʽ
	if(sim900a_send_cmd("AT+CIPHEAD=1","OK",500))return 5;	 				//���ý���������ʾIPͷ(�����ж�������Դ)
	ipbuf[0]=0; 		
	while(1)
	{
		key=KEY_Scan(0);
		if(key==WKUP_PRES)		 
		{  
			mode=!mode;		//����ģʽ�л�
			Show_Str(30+72,90,200,16,(u8*)modetbl[mode],16,0); 	//��ʾ����ģʽ
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
				if(key==13)if(iplen)iplen--;	//ɾ��  
				if(key==14&&iplen)				//ִ��GPRS����
				{    
					sim900a_tcpudp_test(mode,ipbuf,(u8*)port);
					sim900a_gprs_ui();			//����������
					Show_Str(30+72,90,200,16,(u8*)modetbl[mode],16,0); 	//��ʾ����ģʽ
					Show_Str(30+40,130,200,16,(u8*)port,16,0);//��ʾ�˿� 	
					USART3_RX_STA=0;
				}
				if(key==15)break;
			} 
			ipbuf[iplen]=0; 
			LCD_Fill(30+56,110,239,110+16,WHITE);
			Show_Str(30+56,110,200,16,ipbuf,16,0);			//��ʾIP��ַ 	
		} 
		timex++;
		if(timex==20)
		{
			timex=0;
			LED0=!LED0;
		}
		delay_ms(10);
		sim_at_response(1);//���GSMģ�鷢�͹���������,��ʱ�ϴ�������
	}
	return 0;
} 
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//ATK-SIM900A GSM/GPRS�����Կ��Ʋ���

//���Խ�����UI
void sim900a_mtest_ui(u16 x,u16 y)
{
	u8 *p,*p1,*p2; 
	p=mymalloc(SRAMIN,50);//����50���ֽڵ��ڴ�
	LCD_Clear(WHITE);
	POINT_COLOR=RED;
	Show_Str_Mid(0,y,"ATK-SIM900A ���Գ���",16,240);  
	Show_Str(x,y+25,200,16,"��ѡ��:",16,0); 				    	 
	Show_Str(x,y+45,200,16,"KEY0:���Ų���",16,0); 				    	 
	Show_Str(x,y+65,200,16,"KEY1:���Ų���",16,0);				    	 
	Show_Str(x,y+85,200,16,"KEY_UP:GPRS����",16,0);
	POINT_COLOR=BLUE; 	
	USART3_RX_STA=0;
	if(sim900a_send_cmd("AT+CGMI","OK",200)==0)				//��ѯ����������
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF+2),"\r\n");
		p1[0]=0;//���������
		sprintf((char*)p,"������:%s",USART3_RX_BUF+2);
		Show_Str(x,y+110,200,16,p,16,0);
		USART3_RX_STA=0;		
	} 
	if(sim900a_send_cmd("AT+CGMM","OK",200)==0)//��ѯģ������
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF+2),"\r\n"); 
		p1[0]=0;//���������
		sprintf((char*)p,"ģ���ͺ�:%s",USART3_RX_BUF+2);
		Show_Str(x,y+130,200,16,p,16,0);
		USART3_RX_STA=0;		
	} 
	if(sim900a_send_cmd("AT+CGSN","OK",200)==0)//��ѯ��Ʒ���к�
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF+2),"\r\n");//���һس�
		p1[0]=0;//��������� 
		sprintf((char*)p,"���к�:%s",USART3_RX_BUF+2);
		Show_Str(x,y+150,200,16,p,16,0);
		USART3_RX_STA=0;		
	}
	if(sim900a_send_cmd("AT+CNUM","+CNUM",200)==0)			//��ѯ��������
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF),",");
		p2=(u8*)strstr((const char*)(p1+2),"\"");
		p2[0]=0;//���������
		sprintf((char*)p,"��������:%s",p1+2);
		Show_Str(x,y+170,200,16,p,16,0);
		USART3_RX_STA=0;		
	}
	myfree(SRAMIN,p); 
}
//GSM��Ϣ��ʾ(�ź�����,��ص���,����ʱ��)
//����ֵ:0,����
//    ����,�������
u8 sim900a_gsminfo_show(u16 x,u16 y)
{
	u8 *p,*p1,*p2;
	u8 res=0;
	p=mymalloc(SRAMIN,50);//����50���ֽڵ��ڴ�
	POINT_COLOR=BLUE; 	
	USART3_RX_STA=0;
	if(sim900a_send_cmd("AT+CPIN?","OK",200))res|=1<<0;	//��ѯSIM���Ƿ���λ 
	USART3_RX_STA=0;  
	if(sim900a_send_cmd("AT+COPS?","OK",200)==0)		//��ѯ��Ӫ������
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF),"\""); 
		if(p1)//����Ч����
		{
			p2=(u8*)strstr((const char*)(p1+1),"\"");
			p2[0]=0;//���������			
			sprintf((char*)p,"��Ӫ��:%s",p1+1);
			Show_Str(x,y,200,16,p,16,0);
		} 
		USART3_RX_STA=0;		
	}else res|=1<<1;
	if(sim900a_send_cmd("AT+CSQ","+CSQ:",200)==0)		//��ѯ�ź�����
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF),":");
		p2=(u8*)strstr((const char*)(p1),",");
		p2[0]=0;//���������
		sprintf((char*)p,"�ź�����:%s",p1+2);
		Show_Str(x,y+20,200,16,p,16,0);
		USART3_RX_STA=0;		
	}else res|=1<<2;
	if(sim900a_send_cmd("AT+CBC","+CBC:",200)==0)		//��ѯ��ص���
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF),",");
		p2=(u8*)strstr((const char*)(p1+1),",");
		p2[0]=0;p2[5]=0;//���������
		sprintf((char*)p,"��ص���:%s%%  %smV",p1+1,p2+1);
		Show_Str(x,y+40,200,16,p,16,0);
		USART3_RX_STA=0;		
	}else res|=1<<3; 
	if(sim900a_send_cmd("AT+CCLK?","+CCLK:",200)==0)		//��ѯ��ص���
	{ 
		p1=(u8*)strstr((const char*)(USART3_RX_BUF),"\"");
		p2=(u8*)strstr((const char*)(p1+1),":");
		p2[3]=0;//���������
		sprintf((char*)p,"����ʱ��:%s",p1+1);
		Show_Str(x,y+60,200,16,p,16,0);
		USART3_RX_STA=0;		
	}else res|=1<<4; 
	myfree(SRAMIN,p); 
	return res;
} 
//sim900a�����Գ���
void sim900a_test(void)
{
	
	while(sim900a_send_cmd("AT","OK",100))//����Ƿ�Ӧ��ATָ�� 
	{
		Show_Str(40,55,200,16,"GSM ERROR!!!",16,0);
		delay_ms(800);
		LCD_Fill(40,55,200,55+16,WHITE);
		Show_Str(40,55,200,16,"GSM ERROR...",16,0);
		delay_ms(400);  
	} 	 
	LCD_Fill(40,55,200,55+16,WHITE);
	while(sim900a_send_cmd("ATE0","OK",200));//������
	
	sim900a_sms_test();	//���Ų���
						
						
}












