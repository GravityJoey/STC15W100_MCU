

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



/*************	��������˵��	**************

				����˵��

	��������ʹ��STCϵ��MCU����ģ�⴮�ڡ��û������Լ���ʱ�ӺͲ������������ú�������ء�
	
	ʹ�ô���������MCU�������ݣ�MCU�յ���ԭ�����ظ�PC��
	
	������ʹ����Դ: Timer0�ж�.

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
uchar  data TCNT,RCNT;	//���ͺͽ��ռ�� ������(3�����ʼ��)
uchar  data TBIT,RBIT;	//���ͺͽ��յ����ݼ�����
uchar  data i,r;
uchar  data buf[3];

bit  TING,RING;	//���ڷ��ͻ����һ���ֽ�
bit  REND;	 	//������ı�־λ
bit  recvStatu;

#define	RxBitLenth	9	//8������λ+1��ֹͣλ
#define	TxBitLenth	9	//8������λ+1��ֹͣλ

//-----------------------------------------
//UARTģ��ĳ�ʼ����	initial UART module variable
void UART_INIT()
{
      TING = 0;
      RING = 0;
      REND = 0;
      TCNT = 0;
      RCNT = 0;
}

#define	GPIO_Pin_0		0x01	//IO���� Px.0
#define	GPIO_Pin_1		0x02	//IO���� Px.1
#define	GPIO_Pin_2		0x04	//IO���� Px.2
#define	GPIO_Pin_3		0x08	//IO���� Px.3
#define	GPIO_Pin_4		0x10	//IO���� Px.4
#define	GPIO_Pin_5		0x20	//IO���� Px.5
#define	GPIO_Pin_6		0x40	//IO���� Px.6
#define	GPIO_Pin_7		0x80	//IO���� Px.7
#define	GPIO_Pin_All	0xFF	//IO��������

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
	
	EA = 1;						//�����ж�					open global interrupt switch
	
	//GPIO INIT
	P3M1 &= ~0x3c,	P3M0 |=  0x3c;	//�������
	P3M1 |= 0x03,	P3M0 &= ~0x03;	//��������
	//P3M1 &= ~0x03,	P3M0 &= ~0x03;  //����׼˫���

	UART_INIT();				//UARTģ��ĳ�ʼ����
	
	//Ĭ�ϼ̵������Ϊ��
	P32 = 0;
	P33 = 0;
	P34 = 0;
	P35 = 0;
	
	while (1) 
	{
		if (REND)				//���������,�ѽ��յ���ֵ�������buff
		{
			REND = 0;
			//buf[r++ & 0x03] = RBUF;
			
			
			// protocol 
			// head    data1	data2
			// A5	   opcode	cmd
			// opcode :0x01-relay1, 0x02-relay2, 0x04-relay3,0x08-relay4, ��λ��(|)�������Կ��ƶ��IO
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
//��ʱ��0�жϳ���for UART �Բ�����3�����ٶȲ����ж� ��ʼλ		Timer interrupt routine for UART
void tm0(void) interrupt 1
{
	if (RING)
	{
		if (--RCNT == 0)				  //���������Զ�ʱ����1/3������
		{
			RCNT = 3;                   //���ý��ռ�����  ���������Զ�ʱ����1/3������	reset send baudrate counter
			if (--RBIT == 0)			  //������һ֡����
			{
				RBUF = RDAT;            //�洢���ݵ�������	save the data to RBUF
				RING = 0;               //ֹͣ����			stop receive
				REND = 1;               //������ɱ�־����	set receive completed flag
			}
			else
			{
				RDAT >>= 1;			  //�ѽ��յĵ�b���� �ݴ浽 RDAT(���ջ���)
				if (RXB) RDAT |= 0x80;  //shift RX data to RX buffer
			}
		}
	}

	else if (!RXB)		//�ж��ǲ��ǿ�ʼλ RXB=0;
	{
		RING = 1;       //����������ÿ�ʼ���ձ�־λ 	set start receive flag
		RCNT = 4;       //��ʼ�����ղ����ʼ�����       	initial receive baudrate counter
		RBIT = RxBitLenth;       //��ʼ�����յ�����λ��(8������λ+1��ֹͣλ)    initial receive bit number (8 data bits + 1 stop bit)
	}
}