

/*------------------------------------------------------------------*/
/* --- STC MCU International Limited -------------------------------*/
/* --- STC 1T Series MCU RC Demo -----------------------------------*/
/* --- Mobile: (86)13922805190 -------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ---------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ---------------------*/
/* --- Web: www.GXWMCU.com -----------------------------------------*/
/* --- QQ:  800003751 ----------------------------------------------*/
/* If you want to use the program or the program referenced in the  */
/* article, please specify in which data and procedures from STC    */
/*------------------------------------------------------------------*/



/*************	本程序功能说明	**************

				测试说明

	本例程是使用STC系列MCU做的模拟串口。用户根据自己的时钟和波特率自行设置后编译下载。
	
	使用串口助手向MCU发送数据，MCU收到后原样返回给PC。
	
	本例程使用资源: Timer0中断.

*/
#include "config.h"

/***************************************************************************/

typedef bit BOOL;
typedef unsigned char 	uchar;
typedef unsigned int 	uint;

#define Timer0_Reload		(65536 - MAIN_Fosc / BaudRate / 3)
#define D_RxBitLenth	9		//9: 8 + 1 stop
#define D_TxBitLenth	9		//9: 1 stop bit

sbit RXB = P3^1;                //define UART TX/RX port
sbit TXB = P3^0;

uchar  data TBUF,RBUF;
uchar  data TDAT,RDAT;
uchar  data TCNT,RCNT;	//发送和接收检测 计数器(3倍速率检测)
uchar  data TBIT,RBIT;	//发送和接收的数据计数器
uchar  data i,r;
uchar  data buf[3];

bit  TING,RING;	//正在发送或接收一个字节
bit  REND;	 	//接收完的标志位
bit  recvStatu;

#define	RxBitLenth	9	//8个数据位+1个停止位
#define	TxBitLenth	9	//8个数据位+1个停止位

//-----------------------------------------
//UART模块的初始变量	initial UART module variable
void UART_INIT()
{
      TING = 0;
      RING = 0;
      REND = 0;
      TCNT = 0;
      RCNT = 0;
}

#define	GPIO_Pin_0		0x01	//IO引脚 Px.0
#define	GPIO_Pin_1		0x02	//IO引脚 Px.1
#define	GPIO_Pin_2		0x04	//IO引脚 Px.2
#define	GPIO_Pin_3		0x08	//IO引脚 Px.3
#define	GPIO_Pin_4		0x10	//IO引脚 Px.4
#define	GPIO_Pin_5		0x20	//IO引脚 Px.5
#define	GPIO_Pin_6		0x40	//IO引脚 Px.6
#define	GPIO_Pin_7		0x80	//IO引脚 Px.7
#define	GPIO_Pin_All	0xFF	//IO所有引脚

void main()
{
	InternalRAM_enable();
//	ExternalRAM_enable();

	Timer0_1T();
	Timer0_AsTimer();
	Timer0_16bitAutoReload();
	Timer0_Load(Timer0_Reload);
	Timer0_InterruptEnable();
	Timer0_Run();
	
	EA = 1;						//打开总中断					open global interrupt switch
	
	//GPIO INIT
	P3M1 &= ~0x3c,	P3M0 |=  0x3c;	//推挽输出
	P3M1 |= 0x03,	P3M0 &= ~0x03;	//浮空输入
	//P3M1 &= ~0x03,	P3M0 &= ~0x03;  //上拉准双向口

	UART_INIT();				//UART模块的初始变量
	
	//默认继电器输出为低
	P32 = 0;
	P33 = 0;
	P34 = 0;
	P35 = 0;
	
	while (1) 
	{
		if (REND)				//如果接收完,把接收到的值存入接收buff
		{
			REND = 0;
			//buf[r++ & 0x03] = RBUF;
			
			
			// protocol 
			// head    data1	data2
			// A5	   opcode	cmd
			// opcode :0x01-relay1, 0x02-relay2, 0x04-relay3,0x08-relay4, 按位或(|)操作可以控制多个IO
			// relay  :1-P3.2, 2-P3.3, 3-P3.4, 4-P3.5
		
#define PROTO_HEAD  (0xA5)		
#define RELAY1		(0x01)
#define RELAY2		(0x02)
#define RELAY3		(0x04)
#define RELAY4		(0x08)
#define CMD_ON		(0x01)
#define CMD_OFF		(0x00)
				
			if(RBUF == PROTO_HEAD)
			{
				recvStatu = 1;
				r = 0;
			}
			
			if(recvStatu == 1)
			{
				buf[r++] = RBUF; 
			}
			
			if(r == 3)
			{
				recvStatu = 0;
				
				if(buf[1] & RELAY1)
				{
					P32 = buf[2];
				}
				
				if(buf[1] & RELAY2)
				{
					P33 = buf[2];
				}
				
				if(buf[1] & RELAY3)
				{
					P34 = buf[2];
				}
				
				if(buf[1] & RELAY4)
				{
					P35 = buf[2];
				}
			}
		
		}
	}
}


//-----------------------------------------
//定时器0中断程序for UART 以波特率3倍的速度采样判断 开始位		Timer interrupt routine for UART
void tm0(void) interrupt 1
{
	if (RING)
	{
		if (--RCNT == 0)				  //接收数据以定时器的1/3来接收
		{
			RCNT = 3;                   //重置接收计数器  接收数据以定时器的1/3来接收	reset send baudrate counter
			if (--RBIT == 0)			  //接收完一帧数据
			{
				RBUF = RDAT;            //存储数据到缓冲区	save the data to RBUF
				RING = 0;               //停止接收			stop receive
				REND = 1;               //接收完成标志设置	set receive completed flag
			}
			else
			{
				RDAT >>= 1;			  //把接收的单b数据 暂存到 RDAT(接收缓冲)
				if (RXB) RDAT |= 0x80;  //shift RX data to RX buffer
			}
		}
	}

	else if (!RXB)		//判断是不是开始位 RXB=0;
	{
		RING = 1;       //如果是则设置开始接收标志位 	set start receive flag
		RCNT = 4;       //初始化接收波特率计数器       	initial receive baudrate counter
		RBIT = RxBitLenth;       //初始化接收的数据位数(8个数据位+1个停止位)    initial receive bit number (8 data bits + 1 stop bit)
	}
}