#include "pstwo.h"

//用于储存按键值
u16 Handkey;
//用于存储两个命令，分别是开始命令和请求数据命令
u8 Comd[2]={0x01,0x42};	
//数据存储数组
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
--------------函数功能--------------
初始化PS2接收器的GPIO口
	
--------------接口配置--------------
输入		DI->PB12 
输出		DO->PB13	CS->PB14	CLK->PB15
*/
void PS2_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	//使能PORTB时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	//配置 PB13 PB14 PB15 为 通用推挽输出，速度为50mMhz
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	//配置 PB12 为 下拉输入模式
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_12;
	GPIO_Init(GPIOB, &GPIO_InitStruct);									  
}


/*
-----------函数功能----------
发送命令给PS2，并接收PS2的数据

*/
void PS2_Cmd(u8 CMD)
{
	volatile u16 ref=0x01;
	//重置数据
	Data[1] = 0;	
	
	for(ref=0x01;ref<0x0100;ref<<=1)
	{
		//检测是否有指令需要发送，有指令则拉高电平
		if(ref&CMD)	DO_H;                   
		else DO_L;
		
		//先拉高时钟线电平，然后降低，然后再拉高，从而同步发送与接收数据
		CLK_H;                       
		DELAY_TIME;
		CLK_L;
		DELAY_TIME;
		CLK_H;
		
		//若接受到数据，则在对应数据位写1
		if(DI)
			Data[1] = ref|Data[1];
	}
	
	//发送完八位数据之后延时一段时间
	delay_us(16);
}


/*
------------函数功能------------
读取接收器接收到的数据

*/
void PS2_ReadData(void)
{
	volatile u8 byte=0;
	volatile u16 ref=0x01;
	
	//片选线拉低电平以选中接收器
	CS_L;

	//发送请求命令和请求数据命令
	PS2_Cmd(Comd[0]);  
	PS2_Cmd(Comd[1]);  

	//依次读取数组Data的后七个位置
	for(byte=2;byte<9;byte++)         
	{
		//将数据写入Data的后七个位置
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
		
		//每发送完八位数据之后延时一段时间
        delay_us(16);
	}
	
	//拉高片选线电平结束通信
	CS_H;
}


/*
------------函数功能------------
判断是否为红灯模式，return0则为红灯模式

--------------------------------
红灯的ID为“0x73”，绿灯的ID为“0x41”

*/
u8 PS2_RedLight(void)
{
	CS_L;
	PS2_Cmd(Comd[0]);  
	PS2_Cmd(Comd[1]);  
	CS_H;

	//判断是否是红灯模式的ID
	if( Data[1] == 0X73)   return 0 ;
	else return 1;

}


/*
------------函数功能--------------
重置Data数组的所有位

*/
void PS2_ClearData()
{
	u8 a;
	for(a=0;a<9;a++)
		Data[a]=0x00;
}

/*
-----------------函数功能------------------
返回按键的对应键值，键值已在头文件中用按键名的宏定义      

-------------------------------------------
按键按下为0，未按下为1

*/
u8 PS2_DataKey()
{
	u8 index;
	
	PS2_ClearData();
	PS2_ReadData();
	
	//将所有按键对应的位整合成一个16bit的数据
	Handkey=(Data[4]<<8)|Data[3];    
	
	for(index=0;index<16;index++)
	{	    

		//遍历这个16bit的数据，并返回被按下按键的值，按键的值被宏定义
		if((Handkey&(1<<(MASK[index]-1)))==0)
		return index+1;
	}
	return 0;          
}


/*
-------------函数功能------------
返回摇杆的状态数值

*/
u8 PS2_AnologData(u8 button)
{
	return Data[button];
}


/*
------------函数功能-----------
手柄配置初始化

*/
void PS2_SetInit(void)
{
	PS2_ShortPoll();
	PS2_ShortPoll();
	PS2_ShortPoll();
	PS2_EnterConfing();			//进入配置模式
	PS2_TurnOnAnalogMode();	//“红绿灯”配置模式，并选择是否保存
	//PS2_VibrationMode();	//开启震动模式
	PS2_ExitConfing();			//完成并保存配置
}


/*
--------------函数功能--------------
设置发送模式

*/
void PS2_TurnOnAnalogMode(void)
{
	CS_L;
	PS2_Cmd(0x01);  //设置成0x01为红灯模式，0x00为绿灯模式
	PS2_Cmd(0x44);  
	PS2_Cmd(0X00);
	PS2_Cmd(0x01); 	
	PS2_Cmd(0x03); 	//Ox03锁存设置，即不可通过按键“MODE”设置模式。\
										0xEE不锁存软件设置，可通过按键“MODE”设置模式。
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
Description: 手柄震动函数，
Calls:		 void PS2_Cmd(u8 CMD);
Input: motor1:右侧小震动电机 0x00关，其他开
	   motor2:左侧大震动电机 0x40~0xFF 电机开，值越大 震动越大
******************************************************/
void PS2_Vibration(u8 motor1, u8 motor2)
{
	CS_L;
	delay_us(16);
    PS2_Cmd(0x01);  //开始命令
	PS2_Cmd(0x42);  //请求数据
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
//进入配置
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

//振动设置
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
//完成并保存配置
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


















