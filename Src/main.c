/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "stdio.h"
#include "math.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t LCD_String[21];
uint8_t LCD_stat=0;
uint8_t IDLE=8;
uint8_t CNBR=0;
uint8_t VNBR=0;
double CNBR_p=3.5;
double VNBR_p=2.0;
uint8_t ucLed;
//*串口专用变量
uint16_t counter = 0;
uint8_t str_str[40];
uint8_t rx_buffer;
uint8_t RX_BUF[200];//用于缓冲接收200个字节的数量
uint8_t Rx_Counter;//用于记录接收了多少个数据，同时可以索引RX_BUF中的数据位置
uint16_t uwTick_Usart_Set_Point=0;

uint8_t VNBR_Use_Num;
uint8_t CNBR_Use_Num;
uint8_t No_Use_Num = 8;

typedef struct
{
	uint8_t type[5];
	uint8_t id[5];
	uint8_t year_in;
	uint8_t month_in;
	uint8_t day_in;
	uint8_t hour_in;
	uint8_t min_in;
	uint8_t sec_in;
	_Bool notEmpty;
} Car_Data_Storage_Type;

Car_Data_Storage_Type Car_Data_Storage[8];//数据库构建完毕，用于存储8个进来的车的信息

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	counter++;
	RX_BUF[Rx_Counter] = rx_buffer;
	Rx_Counter++;
	HAL_UART_Receive_IT(&huart1, (uint8_t *)(&rx_buffer), 1);
}

_Bool CheckCmd(uint8_t* str)//用于判别接受的22个字符是否合法
{
	if(Rx_Counter != 22)
		return 0;//表示还不够22个数据
	if(((str[0]=='C')||(str[0]=='V'))&&(str[1]=='N')&&(str[2]=='B')&&(str[3]=='R')&&(str[4]==':')&&(str[9]==':'))
	{
		uint8_t i;
		for(i = 10; i< 22;i++)
		{
			if((str[i]>'9')||(str[i]<'0'))
				return 0;
		}
		return 1;//表示接收到的数据没问题
	}
}

void substr(uint8_t* d_str, uint8_t* s_str, uint8_t locate, uint8_t length)//从长字符串里边提取出一段给短字符串
{
	uint8_t i = 0;
	for(i=0; i<length; i++)
	{
		d_str[i] = s_str[locate + i];
	}
	d_str[length] = '\0';
}


//判别车的id是否在库里边
uint8_t isExist(uint8_t* str)
{
	uint8_t i = 0;	
	for(i=0; i<8; i++)
	{
		if((strcmp((const char*)str,(const char*)Car_Data_Storage[i].id)) == 0)//表示字符串匹配，有这个字符串		
		{
			return i;//如果该id在数据库存着，返回这个id在数据库当中的位置
		}
	}	
	return 0xFF;//如果没有，返回oxff
}

//判别数据库中0-7号，哪个位置有空档
uint8_t findLocate(void)
{
	uint8_t i = 0;
	for(i = 0;i <= 7; i++ )
	{
		if(Car_Data_Storage[i].notEmpty == 0)
			return i;//0-7号位
	}
	return 0XFF;
}

//CNBR:A392:200202120000
void USART_stat(){
	if((uwTick -  uwTick_Usart_Set_Point)<100){
		return;//减速函数
	}
	uwTick_Usart_Set_Point = uwTick;
	if(CheckCmd(RX_BUF)){
		uint8_t car_id[5];
		uint8_t car_type[5];
		uint8_t year_tmp,mon_tmp,day_tmp,hour_tmp,min_tmp,sec_tmp;
		
		year_tmp = (((RX_BUF[10] - '0')*10) + (RX_BUF[11] - '0')); 
		mon_tmp = (((RX_BUF[12] - '0')*10) + (RX_BUF[13] - '0')); 	
		day_tmp = (((RX_BUF[14] - '0')*10) + (RX_BUF[15] - '0')); 	
		hour_tmp = (((RX_BUF[16] - '0')*10) + (RX_BUF[17] - '0')); 	
		min_tmp = (((RX_BUF[18] - '0')*10) + (RX_BUF[19] - '0')); 	
		sec_tmp = (((RX_BUF[20] - '0')*10) + (RX_BUF[21] - '0'));
		
		if(mon_tmp>12||day_tmp>31||hour_tmp>23||min_tmp>59||sec_tmp>59){
			goto SEND_ERROR;
		}
		substr(car_id,RX_BUF,5,4);
		substr(car_type,RX_BUF,0,4);
		
		if(isExist(car_id)){ //表示库里边没有这辆车的ID，表示这个车还没进入
			uint8_t locate = findLocate(); //找到哪个地方是空的
			if(locate == 0xFF){ //没有找到哪个地方是空的
			  goto SEND_ERROR;
			}
			substr(Car_Data_Storage[locate].type, car_type, 0, 4);//把当前车的类型存入			
			substr(Car_Data_Storage[locate].id, car_id, 0, 4);//把当前车的id存入
			Car_Data_Storage[locate].year_in = year_tmp;
			Car_Data_Storage[locate].month_in = mon_tmp;			
			Car_Data_Storage[locate].day_in = day_tmp;		
			Car_Data_Storage[locate].hour_in = hour_tmp;			
			Car_Data_Storage[locate].min_in = min_tmp;			
			Car_Data_Storage[locate].sec_in = sec_tmp;		
			Car_Data_Storage[locate].notEmpty = 1;
			if(Car_Data_Storage[locate].type[0] == 'C')
				CNBR_Use_Num++;
			else if(Car_Data_Storage[locate].type[0] == 'V')
				VNBR_Use_Num++;

			No_Use_Num--;
		}
		
		else if(isExist(car_id) != 0xFF)//表示数据库里有他的信息，返回他在数据库的位置
		{
			int64_t Second_derta;//用于核算小时的差值	
			
			uint8_t in_locate = isExist(car_id);//记住在数据库中的位置
			
			
			if(strcmp((const char*)car_type,(const char*)Car_Data_Storage[in_locate].type) != 0)//说明不匹配
			{
			  goto SEND_ERROR;			
			}
			
			//2000 2001 2002  //1   2  3
			Second_derta = (year_tmp - Car_Data_Storage[in_locate].year_in)*365*24*60*60 + (mon_tmp - Car_Data_Storage[in_locate].month_in)*30*24*60*60+\
				(day_tmp - Car_Data_Storage[in_locate].day_in)*24*60*60 + (hour_tmp - Car_Data_Storage[in_locate].hour_in)*60*60 + \
				(min_tmp - Car_Data_Storage[in_locate].min_in)*60 + (sec_tmp - Car_Data_Storage[in_locate].sec_in);
			
			if(Second_derta < 0)//说明出去的时间超前进去的时间
			{
					goto SEND_ERROR;			
			}
//			
			Second_derta = (Second_derta + 3599)/3600;  //小时数据已经核算出来
			sprintf(str_str, "%s:%s:%d:%.2f\r\n",Car_Data_Storage[in_locate].type,Car_Data_Storage[in_locate].id,(unsigned int)Second_derta,(Second_derta*(Car_Data_Storage[in_locate].id[0]=='C'?CNBR_p:VNBR_p)));
			HAL_UART_Transmit(&huart1,(unsigned char *)str_str, strlen(str_str), 50);		
//			
			if(Car_Data_Storage[in_locate].type[0] == 'C')
				CNBR_Use_Num--;
			else if(Car_Data_Storage[in_locate].type[0] == 'V')
				VNBR_Use_Num--;

			No_Use_Num++;			
			
			memset(&Car_Data_Storage[in_locate],0,sizeof(Car_Data_Storage[in_locate]));//清空该位置所有内容，为0
			
		}
	goto CMD_YES;
	
	SEND_ERROR:	
			sprintf(str_str, "Error\r\n");
			HAL_UART_Transmit(&huart1,(unsigned char *)str_str, strlen(str_str), 50);
		
	CMD_YES:
			memset(&RX_BUF[0],0,sizeof(RX_BUF));//清空该位置所有内容，为0
			Rx_Counter = 0;
	}
}

void led_set(uint8_t ucLed,uint8_t led_do){
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	if(led_do==0){
		HAL_GPIO_WritePin(GPIOC,ucLed<<8,GPIO_PIN_SET);
	}else if(led_do==1){
		HAL_GPIO_WritePin(GPIOC,ucLed<<8,GPIO_PIN_RESET);
	}
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
}

void LCD_show(uint8_t LCD_stat){
	if(LCD_stat==0){
		LCD_Clear(Black);
		sprintf((char*)LCD_String,"       Data");
		LCD_DisplayStringLine(Line1,LCD_String);
		sprintf((char*)LCD_String,"   CNBR:%d",CNBR);
		LCD_DisplayStringLine(Line3,LCD_String);
		sprintf((char*)LCD_String,"   VNBR:%d",VNBR);
		LCD_DisplayStringLine(Line5,LCD_String);
		sprintf((char*)LCD_String,"   IDLE:%d",IDLE);
		LCD_DisplayStringLine(Line7,LCD_String);
	}else if(LCD_stat==1){
		LCD_Clear(Black);
		sprintf((char*)LCD_String,"       Para");
		LCD_DisplayStringLine(Line1,LCD_String);
		sprintf((char*)LCD_String,"   CNBR:%.2f",CNBR_p);
		LCD_DisplayStringLine(Line3,LCD_String);
		sprintf((char*)LCD_String,"   VNBR:%.2f",VNBR_p);
		LCD_DisplayStringLine(Line5,LCD_String);
	}
}

uint32_t Key_cur_timer=0;
uint8_t Key_val=0;
uint8_t Key_last=0;
uint8_t Key_next_stat=0;

uint8_t Key_scan()
{
	uint8_t Key_val=0;
	if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)==GPIO_PIN_RESET)
	{
		Key_val=4;
	}
  if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)==GPIO_PIN_RESET)
	{
		Key_val=1;
	}
	if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1)==GPIO_PIN_RESET)
	{
		Key_val=2;
	}
	if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2)==GPIO_PIN_RESET)
	{
		Key_val=3;
	}
	return Key_val;
}

uint8_t analogstat=0;

void analogWrite(float percent){
	const int basic_fre=500; 
	float percent2=percent/100;
	int cal_ans=round(basic_fre*percent2);
	__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_2,cal_ans);
}

void Key_stat(){
	if((uwTick-Key_cur_timer)<20){
		return;
	}else{
		Key_cur_timer=uwTick;
	}
	Key_val=Key_scan();
	Key_next_stat=Key_val & (Key_last^Key_val);
	Key_last=Key_val;
	switch(Key_next_stat){
		case 1:
			LCD_stat++;
			LCD_stat=LCD_stat%2;
			LCD_show(LCD_stat);
			break;
		case 2:
			switch(LCD_stat){
			  case 1:
					CNBR_p+=0.5;
					VNBR_p+=0.5;
					break;
			}
			LCD_show(LCD_stat);
			break;
		case 3:
			switch(LCD_stat){
			  case 1:
					CNBR_p-=0.5;
					VNBR_p-=0.5;
					break;
			}
			LCD_show(LCD_stat);
			break;
		case 4:
			if(analogstat==0){
				HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
				analogWrite(20);
				led_set(2,1);
				analogstat=1;
			}else{
				HAL_TIM_PWM_Stop(&htim3,TIM_CHANNEL_2);
				led_set(2,0);
				analogstat=0;
			}
			break;
		default:
			break;
	}
}
/* USER CODE END 0 */



/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
	HAL_UART_Receive_IT(&huart1, (uint8_t *)(&rx_buffer), 1);
  /* USER CODE BEGIN 2 */
	LCD_Init();
	LCD_SetBackColor(Black);
	LCD_SetTextColor(White);
	
	LCD_show(0);
	led_set(0,0);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		USART_stat();
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV3;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  __disable_irq();
  while (1)
  {
  }
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
