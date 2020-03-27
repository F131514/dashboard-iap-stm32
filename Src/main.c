/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"
#include "gpio.h"



/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_customhid.h" //?????????
#include "flash.h"

typedef  void (*pFunction)(void);
#define ApplicationAddress    (0x8004000)
pFunction Jump_To_Application;
uint32_t JumpAddress;

uint32_t FLASH_ADDR = APP_FLASH_START;						//д��ҳ�ĵ�ַ  


ST_Data FLASH_Data;
ST_Data *FLASH_DATA = &FLASH_Data;



extern USBD_HandleTypeDef hUsbDeviceFS; //????USB????
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define RegBase  (0x40005C00L)  /* USB_IP Peripheral Registers base address */
/* Control register */
#define CNTR    ((__IO unsigned *)(RegBase + 0x40))
#define _SetCNTR(wRegValue)  (*CNTR   = (uint16_t)wRegValue)
/* GetCNTR */
#define _GetCNTR()   ((uint16_t) *CNTR)

#define GPIOA_ODR_Addr    (GPIOA_BASE+12) //0x4001080C 

#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //��� 
 
void usb_port_set(uint8_t enable) 
{ 
		__HAL_RCC_GPIOA_CLK_ENABLE();           	//����GPIOAʱ��	 
		if(enable){ 
			GPIOA->CRH&=0XFFF00FFF; 
			GPIOA->CRH|=0X00044000; 
			_SetCNTR(_GetCNTR()&(~(1<<1)));
		}
		else { 
			_SetCNTR(_GetCNTR()|(1<<1)); 
			GPIOA->CRH&=0XFFF00FFF; 
			GPIOA->CRH|=0X00033000; 
			PAout(12)=0; 
		} 
} 
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint8_t send_buf[64] = {0};
uint8_t USB_Recive_Buffer[64] = {0}; //USB????


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void buffer_clear(uint8_t * buf);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/*Variable used for Erase procedure*/





/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	static uint8_t Status = 0;
	static int time_flag = 0;				//��¼ʱ��
	uint8_t flash_flag = 0; //�жϵ�ǰ �Ƿ����appģʽ�ı�־λ ��flash��15ҳ���һ�����ж�ȡ����
	
  /* USER CODE END 1 */
  
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	usb_port_set(1);		//USB���ƼĴ��� ��USB		
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
	Flash_ReadData(ADDR_FLASH_PAGE_16-1,&flash_flag,1);
	if(flash_flag == 0x55){
		while(1){
		usb_port_set(0);		//USB���ƼĴ��� �ر�USB			

		UserAppStart();
		__set_FAULTMASK(1); 
		NVIC_SystemReset();				//ϵͳ�����λ			
		};
			
	}
	
	FLASH_STRUCT_Init(FLASH_DATA);

  /* USER CODE END 2 */
	
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		
		if(Status == 0){					//δ���ܵ�������Ϣ�� ֮ǰ ÿ��һ�� ����λ������ ׼������
			time_flag++;		
			if(time_flag > 10){					//10*100ms = 1s
				time_flag = 0;
				send_buf[0] = UPGRED_READY_PACK;   //��ʼ����� ֪ͨ��λ�� ��������
				USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, send_buf, sizeof(send_buf));
				buffer_clear(send_buf);										
			}
			HAL_Delay(100);    //100ms
		}

	
		if(USB_Recive_Buffer[0] != 0){			
			switch (Status){							
				
				case 0:																							//״̬0 �ȴ�������Ϣ�� ��ͬʱ ����λ�����;�����
					if(USB_Recive_Buffer[0]==UPGRED_INFORM_PACK){			//�յ�������Ϣ��
						upgrepInform(&Status);
					}
					break;
				
				
				case 1:																							//״̬1 ������Ϣ���ݴ��� 
					if(USB_Recive_Buffer[0] == UPGRED_DATA_PACK){		  //�յ����ݰ�
						 upgrepData(&Status);
					}
					break;
					
				case 2:
					
					break;
			
			}
			buffer_clear(USB_Recive_Buffer);			// ��� USB �洢����PC���������ݵ����� 
			
			
			//���ѭ�����������ֽ��� ����flash��һҳ��
			if(FLASH_DATA->DATA_8_LEN >= FLASH_PAGE_SIZE){		
				transform(FLASH_DATA);									//�ֽ� ת��Ϊ�� 
				Flash_WriteData(FLASH_ADDR,FLASH_DATA->DATA_32); //д��flash
				DATA32_Init(FLASH_DATA->DATA_32);									//д�����ݺ�Ѷ�Ӧ����ȫ����λ 0XFF
				FLASH_ADDR += FLASH_PAGE_SIZE;							//flash��ַ ����ƶ�һҳ	
			}
			//����Ѿ����������һ�������� �� �������ݲ�û�иպ�д�����һҳ			
			if(Status == 2 && (FLASH_DATA->TOTAL_BYTE%FLASH_PAGE_SIZE) != 0){				
				transform_extra(FLASH_DATA);
				Flash_WriteData(FLASH_ADDR,FLASH_DATA->DATA_32); //д��flash	
				DATA32_Init(FLASH_DATA->DATA_32);									//д�����ݺ�Ѷ�Ӧ����ȫ����λ 0XFF
				
			}

		}
	
		if(Status == 2){
			
			send_buf[0] = UPGRED_STATUS_PACK;
			if(FLASH_DATA->CHECKSUM == 0){			//checksum��ȷ
				send_buf[1] = YES;
				Status = 3;		
				
			}
			else {
				send_buf[1]= NO;							//checksum ����
				Status = 0;
				FLASH_STRUCT_Init(FLASH_DATA);
			}
			USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, send_buf, sizeof(send_buf));			
			buffer_clear(send_buf);
			Flash_WriteWord(ADDR_FLASH_PAGE_15,0x55555555,255);
			HAL_Delay(1000);
			__set_FAULTMASK(1); 
			NVIC_SystemReset();				//ϵͳ�����λ			
			while(1);
		}		
		
//    /* USER CODE BEGIN 3 */
		

//		/* USER CODE END 3 */
  }

}


void upgrepInform(uint8_t *Status){
						
		FLASH_DATA->TOTAL_BYTE = USB_Recive_Buffer[1];				//  �������� һ���ж��ٸ��ֽ�
		FLASH_DATA->TOTAL_BYTE <<= 8;
		FLASH_DATA->TOTAL_BYTE += USB_Recive_Buffer[2];					//STM32��С��ģʽ  ���Ƶ�ַ���
		FLASH_DATA->TOTAL_BYTE <<= 8;
		FLASH_DATA->TOTAL_BYTE += USB_Recive_Buffer[3];
		FLASH_DATA->TOTAL_BYTE <<= 8;
		FLASH_DATA->TOTAL_BYTE += USB_Recive_Buffer[4];					
		
		if((FLASH_DATA->TOTAL_BYTE)%61 == 0)												//����� һ���ж��ٰ� һ����61���ֽ� װ����
			FLASH_DATA->TOTAL_PACK = (FLASH_DATA->TOTAL_BYTE)/61;
		else 
			FLASH_DATA->TOTAL_PACK = ((FLASH_DATA->TOTAL_BYTE)/61)+1;
		
		if((FLASH_DATA->TOTAL_BYTE)%1024 == 0)											// �����һ���ж���ҳ  STM32F1 һҳ��1024���ֽ�
			FLASH_DATA->TOTAL_PAGE = FLASH_DATA->TOTAL_BYTE/1024;
		else
			FLASH_DATA->TOTAL_PAGE = (FLASH_DATA->TOTAL_BYTE/1024)+1;
		
		
		FLASH_DATA->CHECKSUM = USB_Recive_Buffer[5];							//У���						
		
		send_buf[0] = AKC_PACK;
		if(FLASH_DATA->TOTAL_BYTE != 0 && FLASH_DATA->TOTAL_BYTE < (128-16)*1024){						//���ֽ��� ���ܵ���0 ���ܴ��� оƬ����flash оƬ����128K bootloaderռ����16K
			send_buf[1] =	YES;										//���������ɹ�														
			*Status = 1;											
		}
		else{
			send_buf[1] =	NO;												//ʧ�� ����
		}
		USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, send_buf, sizeof(send_buf));	
		buffer_clear(send_buf);		
}


void upgrepData(uint8_t * Status){
		FLASH_DATA->PACK_NUM_PC = USB_Recive_Buffer[1];			//����PC�������ݰ��ı��
		FLASH_DATA->PACK_NUM_PC <<= 8;
		FLASH_DATA->PACK_NUM_PC += USB_Recive_Buffer[2];
			
		send_buf[0] = AKC_PACK;		//Ӧ���ͷ
		
		if(FLASH_DATA->PACK_NUM_PC == FLASH_DATA->PACK_NUM ){		//���ݰ������ȷ 
				send_buf[1] = YES;  			 // ���δ���û����
			
				//�������һ������ �� ���һ������ǡ��װ��
				if(FLASH_DATA->PACK_NUM_PC != FLASH_DATA->TOTAL_PACK || (FLASH_DATA->TOTAL_BYTE%61)==0){	
					for(int i = 3; i < 64 ; i++){
						FLASH_DATA->DATA_8[FLASH_DATA->DATA_8_INDEX_END] = USB_Recive_Buffer[i];
						FLASH_DATA->CHECKSUM += USB_Recive_Buffer[i];
						FLASH_DATA->DATA_8_INDEX_END++;
						if(FLASH_DATA->DATA_8_INDEX_END >= MAX_round_queue) FLASH_DATA->DATA_8_INDEX_END -= MAX_round_queue;   //ѭ������							
						FLASH_DATA->DATA_8_LEN++;			  //���г���+1				
					}							
				
				}
				else{			// ���һ������ �� δװ�� 
					for(int i = 3; i < ((FLASH_DATA->TOTAL_BYTE%61)+3); i++){
						FLASH_DATA->DATA_8[FLASH_DATA->DATA_8_INDEX_END] = USB_Recive_Buffer[i];
						FLASH_DATA->CHECKSUM += USB_Recive_Buffer[i];
						FLASH_DATA->DATA_8_INDEX_END++;
						if(FLASH_DATA->DATA_8_INDEX_END >= MAX_round_queue) FLASH_DATA->DATA_8_INDEX_END -= MAX_round_queue;    //ѭ������
						FLASH_DATA->DATA_8_LEN++;
					}
				}

				
				
				if(FLASH_DATA->PACK_NUM == FLASH_DATA->TOTAL_PACK){					//��������� ���һ������  ����״̬������һ��״̬
					*Status = 2;
					FLASH_DATA->PACK_NUM = 0;								
				}	
				FLASH_DATA->PACK_NUM++;			// ��ǰ׼�����ܵ����ݰ����+1
		}						
		else{
			send_buf[1] = NO;			 //	����ʧ��
		}
		USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, send_buf, sizeof(send_buf));	  
		buffer_clear(send_buf);			
}




void UserAppStart(void)

{
//		__set_PRIMASK(1);																	
	  if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
    { 
      /* Jump to user application */
//				__ASM("CPSID  I");	
      JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);

      Jump_To_Application = (pFunction) JumpAddress;

      /* Initialize user application's Stack Pointer */

      __set_MSP(*(__IO uint32_t*) ApplicationAddress);

	
		for(int i = 0; i < 8; i++)

		{			

			NVIC->ICER[i] = 0xFFFFFFFF;	/* �ر��ж�*/

			NVIC->ICPR[i] = 0xFFFFFFFF;	/* ����жϱ�־λ */

		}

      Jump_To_Application();
    }

}





void buffer_clear(uint8_t * buf){

	for(int i = 0; i < 64; i++){
		buf[i] = 0;
	}
}










/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
