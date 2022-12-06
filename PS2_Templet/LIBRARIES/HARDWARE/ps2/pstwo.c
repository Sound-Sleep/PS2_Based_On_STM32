#include "pstwo.h"

//���ڴ��水��ֵ
u16 Handkey;
//���ڴ洢��������ֱ��ǿ�ʼ�����������������
u8 Comd[2]={0x01,0x42};	
//���ݴ洢����
u8 Data[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; 
//
u16 MASK[]={
    PSB_SELECT,
    PSB_L3,
    PSB_R3 ,
    PSB_START,
    PSB_PAD_UP,
    PSB_PAD_RIGHT,
    PSB_PAD_DOWN,
    PSB_PAD_LEFT,
    PSB_L2,
    PSB_R2,
    PSB_L1,
    PSB_R1 ,
    PSB_GREEN,
    PSB_RED,
    PSB_BLUE,
    PSB_PINK
	};

	
/*
--------------��������--------------
��ʼ��PS2��������GPIO��
	
--------------�ӿ�����--------------
����		DI->PB12 
���		DO->PB13	CS->PB14	CLK->PB15
*/
void PS2_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	//ʹ��PORTBʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	//���� PB13 PB14 PB15 Ϊ ͨ������������ٶ�Ϊ50mMhz
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	//���� PB12 Ϊ ��������ģʽ
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_12;
	GPIO_Init(GPIOB, &GPIO_InitStruct);									  
}


/*
-----------��������----------
���������PS2��������PS2������

*/
void PS2_Cmd(u8 CMD)
{
	volatile u16 ref=0x01;
	//��������
	Data[1] = 0;	
	
	for(ref=0x01;ref<0x0100;ref<<=1)
	{
		//����Ƿ���ָ����Ҫ���ͣ���ָ�������ߵ�ƽ
		if(ref&CMD)	DO_H;                   
		else DO_L;
		
		//������ʱ���ߵ�ƽ��Ȼ�󽵵ͣ�Ȼ�������ߣ��Ӷ�ͬ���������������
		CLK_H;                       
		DELAY_TIME;
		CLK_L;
		DELAY_TIME;
		CLK_H;
		
		//�����ܵ����ݣ����ڶ�Ӧ����λд1
		if(DI)
			Data[1] = ref|Data[1];
	}
	
	//�������λ����֮����ʱһ��ʱ��
	delay_us(16);
}


/*
------------��������------------
��ȡ���������յ�������

*/
void PS2_ReadData(void)
{
	volatile u8 byte=0;
	volatile u16 ref=0x01;
	
	//Ƭѡ�����͵�ƽ��ѡ�н�����
	CS_L;

	//�������������������������
	PS2_Cmd(Comd[0]);  
	PS2_Cmd(Comd[1]);  

	//���ζ�ȡ����Data�ĺ��߸�λ��
	for(byte=2;byte<9;byte++)         
	{
		//������д��Data�ĺ��߸�λ��
		for(ref=0x01;ref<0x100;ref<<=1)
		{
			CLK_H;
			DELAY_TIME;
			CLK_L;
			DELAY_TIME;
			CLK_H;
		      if(DI)
		      Data[byte] = ref|Data[byte];
		}
		
		//ÿ�������λ����֮����ʱһ��ʱ��
        delay_us(16);
	}
	
	//����Ƭѡ�ߵ�ƽ����ͨ��
	CS_H;
}


/*
------------��������------------
�ж��Ƿ�Ϊ���ģʽ��return0��Ϊ���ģʽ

--------------------------------
��Ƶ�IDΪ��0x73�����̵Ƶ�IDΪ��0x41��

*/
u8 PS2_RedLight(void)
{
	CS_L;
	PS2_Cmd(Comd[0]);  
	PS2_Cmd(Comd[1]);  
	CS_H;

	//�ж��Ƿ��Ǻ��ģʽ��ID
	if( Data[1] == 0X73)   return 0 ;
	else return 1;

}


/*
------------��������--------------
����Data���������λ

*/
void PS2_ClearData()
{
	u8 a;
	for(a=0;a<9;a++)
		Data[a]=0x00;
}

/*
-----------------��������------------------
���ذ����Ķ�Ӧ��ֵ����ֵ����ͷ�ļ����ð������ĺ궨��      

-------------------------------------------
��������Ϊ0��δ����Ϊ1

*/
u8 PS2_DataKey()
{
	u8 index;
	
	PS2_ClearData();
	PS2_ReadData();
	
	//�����а�����Ӧ��λ���ϳ�һ��16bit������
	Handkey=(Data[4]<<8)|Data[3];    
	
	for(index=0;index<16;index++)
	{	    

		//�������16bit�����ݣ������ر����°�����ֵ��������ֵ���궨��
		if((Handkey&(1<<(MASK[index]-1)))==0)
		return index+1;
	}
	return 0;          
}


/*
-------------��������------------
����ҡ�˵�״̬��ֵ

*/
u8 PS2_AnologData(u8 button)
{
	return Data[button];
}


/*
------------��������-----------
�ֱ����ó�ʼ��

*/
void PS2_SetInit(void)
{
	PS2_ShortPoll();
	PS2_ShortPoll();
	PS2_ShortPoll();
	PS2_EnterConfing();			//��������ģʽ
	PS2_TurnOnAnalogMode();	//�����̵ơ�����ģʽ����ѡ���Ƿ񱣴�
	//PS2_VibrationMode();	//������ģʽ
	PS2_ExitConfing();			//��ɲ���������
}


/*
--------------��������--------------
���÷���ģʽ

*/
void PS2_TurnOnAnalogMode(void)
{
	CS_L;
	PS2_Cmd(0x01);  //���ó�0x01Ϊ���ģʽ��0x00Ϊ�̵�ģʽ
	PS2_Cmd(0x44);  
	PS2_Cmd(0X00);
	PS2_Cmd(0x01); 	
	PS2_Cmd(0x03); 	//Ox03�������ã�������ͨ��������MODE������ģʽ��\
										0xEE������������ã���ͨ��������MODE������ģʽ��
	PS2_Cmd(0X00);
	PS2_Cmd(0X00);
	PS2_Cmd(0X00);
	PS2_Cmd(0X00);
	CS_H;
	delay_us(16);
}





/*---------------------------------------------------------------------*/




/******************************************************
Function:    void PS2_Vibration(u8 motor1, u8 motor2)
Description: �ֱ��𶯺�����
Calls:		 void PS2_Cmd(u8 CMD);
Input: motor1:�Ҳ�С�𶯵�� 0x00�أ�������
	   motor2:�����𶯵�� 0x40~0xFF �������ֵԽ�� ��Խ��
******************************************************/
void PS2_Vibration(u8 motor1, u8 motor2)
{
	CS_L;
	delay_us(16);
    PS2_Cmd(0x01);  //��ʼ����
	PS2_Cmd(0x42);  //��������
	PS2_Cmd(0X00);
	PS2_Cmd(motor1);
	PS2_Cmd(motor2);
	PS2_Cmd(0X00);
	PS2_Cmd(0X00);
	PS2_Cmd(0X00);
	PS2_Cmd(0X00);
	CS_H;
	delay_us(16);  
}
//short poll
void PS2_ShortPoll(void)
{
	CS_L;
	delay_us(16);
	PS2_Cmd(0x01);  
	PS2_Cmd(0x42);  
	PS2_Cmd(0X00);
	PS2_Cmd(0x00);
	PS2_Cmd(0x00);
	CS_H;
	delay_us(16);	
}
//��������
void PS2_EnterConfing(void)
{
    CS_L;
	delay_us(16);
	PS2_Cmd(0x01);  
	PS2_Cmd(0x43);  
	PS2_Cmd(0X00);
	PS2_Cmd(0x01);
	PS2_Cmd(0x00);
	PS2_Cmd(0X00);
	PS2_Cmd(0X00);
	PS2_Cmd(0X00);
	PS2_Cmd(0X00);
	CS_H;
	delay_us(16);
}

//������
void PS2_VibrationMode(void)
{
	CS_L;
	delay_us(16);
	PS2_Cmd(0x01);  
	PS2_Cmd(0x4D);  
	PS2_Cmd(0X00);
	PS2_Cmd(0x00);
	PS2_Cmd(0X01);
	CS_H;
	delay_us(16);	
}
//��ɲ���������
void PS2_ExitConfing(void)
{
    CS_L;
	delay_us(16);
	PS2_Cmd(0x01);  
	PS2_Cmd(0x43);  
	PS2_Cmd(0X00);
	PS2_Cmd(0x00);
	PS2_Cmd(0x5A);
	PS2_Cmd(0x5A);
	PS2_Cmd(0x5A);
	PS2_Cmd(0x5A);
	PS2_Cmd(0x5A);
	CS_H;
	delay_us(16);
}


















