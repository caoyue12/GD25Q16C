#include "GD25Q16.h"

/*每个Block为64K*/
/*每个Sector为4k*/
/*每个Page为256字节*/
/*16页为1个Sector*/

#define Debug 1

unsigned char Write_buffer[512]={0};
unsigned char Read_buffer[512]={0};

void GD_Read(unsigned char *ReadBuffer,unsigned char Length);
void GD_SendCommand(unsigned char command);
unsigned char Compare(unsigned char* buffer1,unsigned char* buffer2,int count);


void CS(unsigned char select)
{
	if(select)
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}


unsigned char DG_ReadStatus0_7()
{
	unsigned char  recv_buf = 0;
	CS(0);
	GD_SendCommand(0x05);
	GD_Read(&recv_buf,1);
	CS(1);
	//printf("DEBUG %02X\r\n",recv_buf);
	return recv_buf;
}

void DG_WaitBusy()
{
	//printf("DEBUG %02X\r\n",DG_ReadStatus0_7());
	while(DG_ReadStatus0_7()&0x01)
	{
	};
}

void SendDummy()
{
   unsigned char temp = 0x00;
   HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)&temp, (uint8_t *)&temp, 1, 5000);  
}



void GD_SendCommand(unsigned char command)
{
	unsigned char temp_tx = command;
	unsigned char temp_rx ;
	HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)&temp_tx, (uint8_t *)&temp_rx, 1, 5000);
}

void GD_Read(unsigned char *ReadBuffer,unsigned char Length)
{
	unsigned char temp = 0x00;
	HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)&temp, (uint8_t *)ReadBuffer, Length, 5000);
}

void GD_WriteEnable()
{
	CS(0);
	GD_SendCommand(0x06);
	CS(1);
}





void GD_WriteDisenable()
{
	CS(0);
	GD_SendCommand(0x04);
	//DG_WaitBusy();
	CS(1);
}

void GD_UniqueID()
{
	CS(0);
	unsigned char i;
	unsigned char ID[16]={0};
	GD_SendCommand(0x4B);
	SendDummy();SendDummy();SendDummy();SendDummy();
	GD_Read(ID,16);
	printf("\r\nGD25Q16C ID:");
	for(i=0;i<16;i++)
	{
		printf("%02X ",ID[i]);
	}
	printf("\r\n");
	CS(1);
}

static HAL_StatusTypeDef SPI_Transmit(uint8_t* send_buf, uint16_t size)
{
    return HAL_SPI_Transmit(&hspi1, send_buf, size, 100);
}



static HAL_StatusTypeDef SPI_Receive(uint8_t* recv_buf, uint16_t size)
{
   return HAL_SPI_Receive(&hspi1, recv_buf, size, 100);
}


void GD_ManufactureID()
{
	CS(0);
	uint8_t recv_buf[2] = {0};
	uint16_t device_id = 0;
	GD_SendCommand(0x90);
	SendDummy();
	SendDummy();
	SendDummy();
	GD_Read(recv_buf,2);
	printf("ManufactureID : %02X %02X\r\n",recv_buf[0],recv_buf[1]);
	CS(1);
}





void DG_Write_Page(uint32_t   addr ,unsigned char *buffer,unsigned  int size)
{

	unsigned char temp[512];
	//GD_WriteEnable();
	CS(0);
	GD_SendCommand(0x02);
	//printf("DEBUG %02X \r\n",addr>>16);
	GD_SendCommand((uint8_t)(addr>>16));
	GD_SendCommand((uint8_t)(addr>>8));
	GD_SendCommand((uint8_t)addr);
	HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)buffer, (uint8_t *)temp, size, 5000);
	//DG_WaitBusy();
	CS(1);
}

void DG_Read(uint32_t addr ,unsigned char *buffer,unsigned int size)
{
	unsigned char temp[512];
	CS(0);
	//GD_WriteEnable();
	GD_SendCommand(0x03);
	GD_SendCommand((uint8_t)(addr>>16));
	GD_SendCommand((uint8_t)(addr>>8));
	GD_SendCommand((uint8_t)addr);
	HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)temp, (uint8_t *)buffer, size, 5000);
	//DG_WaitBusy();
	CS(1);
}

void GD_SectorErase(long int addr)
{
	//unsigned char temp;
	//GD_WriteEnable();
	CS(0);
	GD_SendCommand(0x20);
	GD_SendCommand((uint8_t)(addr>>16));
	GD_SendCommand((uint8_t)(addr>>8));
	GD_SendCommand((uint8_t)addr);
	//DG_WaitBusy();
	CS(1);
}


void DG_RDID()
{
	CS(0);
	uint8_t recv_buf[3] = {0};
	uint16_t device_id = 0;
	GD_SendCommand(0x9F);
	//SendDummy();
	//SendDummy();
	//SendDummy();
	GD_Read(recv_buf,3);
	printf("RDID : %02X %02X %02X\r\n",recv_buf[0],recv_buf[1],recv_buf[2]);
	CS(1);
}



void DG_Test()
{
	static int error_count=0;
	static int count = 0;
	int a = rand();
	count++;
	printf("\r\n-------------------------------------------------------------");
	printf("\r\n------------------------Flash Test %d ------------------------\r\n",count);
	// GD_UniqueID();
	// GD_ManufactureID();	
	DG_RDID();
	int i;
	for(i=0;i<256;i++)
	{
		Write_buffer[i] = i+(unsigned char)a;
	}
	printf("------------------------Write Flash------------------------\r\n");
	for(i=0;i<256;i++)
	{

		if(!(i%4)&&i!=0)
		printf(" ");
		if(!(i%(4*10)))
		printf("\r\n");
		if(!(i%4))
		printf("0x");
		printf("%02X",Write_buffer[i]);
		
	}


	memset(Read_buffer,0,256);
	GD_WriteEnable();
	GD_SectorErase(0x00000000);
	DG_WaitBusy();
	GD_WriteEnable();
	DG_WaitBusy();
	DG_Write_Page(0x00000000,Write_buffer,256);
	DG_WaitBusy();
	DG_Read(0x00000000,Read_buffer,256);
	DG_WaitBusy();
	printf("\r\n------------------------Read Flash------------------------\r\n");
	for(i=0;i<256;i++)
	{
		if(!(i%4)&&i!=0)
		printf(" ");
		if(!(i%(4*10)))
		printf("\r\n");
		if(!(i%4))
		printf("0x");
		printf("%02X",Read_buffer[i]);
	}
	
	printf("\r\n------------------------Compare Data------------------------\r\n");
	if(Compare(Read_buffer,Write_buffer,256))
	error_count++;
	printf("Error_Count : %d\r\n",error_count);
	printf("Count : %d\r\n",count);
}

unsigned char Compare(unsigned char* buffer1,unsigned char* buffer2,int count)
{
	unsigned char result=0;
	while(count)
	{
		count--;
		if(*buffer1!=*buffer2)
		{
			result = -1;
			return result;
		}
		buffer1++;
		buffer2++;
	}
	return result;
} 

