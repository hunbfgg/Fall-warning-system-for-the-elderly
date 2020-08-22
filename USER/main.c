#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h" 		 	 
#include "lcd.h"  
#include "key.h"     
#include "malloc.h"    
#include "usart3.h"
#include "sim900a.h"  
#include "mpu6050.h"
#include "mpuiic.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 
#include "usart2.h" 
#include "hc_sr04.h"
#include "timer.h"

u8 flag=0;
u8 shuju[104]="53177EACFF1A003000305EA60030003052060030003079D200204E1C7ECFFF1A0030003000305EA60030003052060030003079D2";//北纬：度东经：度
//////////////////////// 0   /  1  /  3/ 4////////6/ 7//13  / 14 / 15 / 17 / 18 //20/21//
u8 data[10];
u8 res;
u16 g;
u8 a;
float Distance=0;
u8 cishu;
u8 flag_3=1;

 int main(void)
 {	 
	u8 i;	 
	delay_init();	    	 //延时函数初始化	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200
	usart2_init(9600);
	usart3_init(115200);		//初始化串口3
 	LED_Init();		  			//初始化与LED连接的硬件接口
	KEY_Init();					//初始化按键
	LCD_Init();			   		//初始化LCD  
	TIM_Counter();
	hc_sr04_init();
	LCD_Display_Dir(1);	
	POINT_COLOR=BLUE; 
	TIM3_Int_Init(4999,7199);
	sim900a_test(); 
	LCD_Clear (WHITE);

	LCD_ShowChar(1,20,'N',16,1);
	LCD_ShowChar(1,40,'E',16,1);
	

	
	 while(1)
	 {
			
//		Distance =(Get_hcsr04length()*340/2/1000);
//		printf("触发了%d次\r\n",cishu);
///////////////////////////////////////////////////////////////////////		
		 printf("flag=%d\r\n",flag);
			
		 if(WK_UP==1); 
		 else 
		 {
			 printf("语音触发\r\n");
			 flag=1;
		 }
		
		 if(Distance<=500)
		 {
			 cishu++;
			 delay_ms (1000);
		 }
		 else 
		 {
			 cishu=0;
		 }
		 if(cishu>30)
		 {
			 printf("摔倒触发\r\n");
			 cishu=0;
			 flag=1;
		 }
		 
////////////////////////////////////////////////////////////
		 if(flag==1)
		 {
			 printf("短信触发\r\n");
			 printf("正在发短信\r\n");
			 sim900a_sms_send_test();
			 delay_ms(1000);
			 delay_ms(1000);
			 delay_ms(1000);
			 delay_ms(1000);
			 delay_ms(1000);
			 delay_ms(1000);
		   printf("短信发送结束\r\n");
			 flag=0;
		 }
		 
//////////////////////////////////////////////////////
		 if(flag_3==1&&USART2_RX_BUF[20]=='4')
		 {
//		if(a==1)
			 if(USART2_RX_BUF[33]<='1'&&USART2_RX_BUF[33]!=','&&USART2_RX_BUF[34]!=','&&USART2_RX_BUF[38]=='.')
			 {
				 TIM_Cmd(TIM3, ENABLE);
				 flag_3=0;
			 }				 
			 else
			 {
				TIM_Cmd(TIM3, DISABLE);
				 flag_3=1;
			 }
			LCD_Fill(21,20,320,60,WHITE);
			for(i=0;i<15;i++)
			{
					LCD_ShowChar(1+20*(i),0,' ',16,1);
			}
			for(i=20;i<30;i++)
			{
				LCD_ShowChar(21+20*3,20,'.',16,1);
				if(i<22)
					LCD_ShowChar(21+20*(i%20),20,USART2_RX_BUF[i],16,1);
				else
					LCD_ShowChar(21+20*(i%18),20,USART2_RX_BUF[i],16,1);
			}
			for(i=33;i<44;i++)
			{
				LCD_ShowChar(21+20*3,40,'.',16,1);
				if(i<36)
					LCD_ShowChar(21+20*(i%33),40,USART2_RX_BUF[i],16,1);
				else
					LCD_ShowChar(21+20*(i%32),40,USART2_RX_BUF[i],16,1);
					
			}
			for(i=45;i<60;i++)
			{
					LCD_ShowChar(1+20*(i%45),60,' ',16,1);
			}
			for(i=60;i<75;i++)
			{
					LCD_ShowChar(1+20*(i%60),80,' ',16,1);
			}
			
			gouzao(USART2_RX_BUF[20],0);
			gouzao(USART2_RX_BUF[21],1);
			gouzao(USART2_RX_BUF[22],3);
			gouzao(USART2_RX_BUF[23],4);
			gouzao(USART2_RX_BUF[25],6);
			gouzao(USART2_RX_BUF[26],7);
			gouzao(USART2_RX_BUF[33],13);
			gouzao(USART2_RX_BUF[34],14);
			gouzao(USART2_RX_BUF[35],15);
			gouzao(USART2_RX_BUF[36],17);
			gouzao(USART2_RX_BUF[37],18);
			gouzao(USART2_RX_BUF[39],20);
			gouzao(USART2_RX_BUF[40],21);
			
//			Distance =(Get_hcsr04length()*340/2/1000);
//			printf("距离是%.2f,触发了%d次\r\n",Distance,cishu);
	
		}
		LED1=!LED1;
////////////////////////////////////////////////////////////////////////////
	}
}


















