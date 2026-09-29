#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

void LCD_Enable(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_Delay(1);
}

void LCD_Send4Bits(uint8_t data)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,
                      (data & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,
                      (data & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,
                      (data & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,
                      (data & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    LCD_Enable();
}

void LCD_Command(uint8_t command)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    LCD_Send4Bits(command >> 4);
    LCD_Send4Bits(command & 0x0F);

    HAL_Delay(2);
}

void LCD_Data(uint8_t data)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

    LCD_Send4Bits(data >> 4);
    LCD_Send4Bits(data & 0x0F);

    HAL_Delay(1);
}

void LCD_String(char *text)
{
    while (*text != '\0')
    {
        LCD_Data((uint8_t)*text);
        text++;
    }
}

void LCD_Init(void)
{
    HAL_Delay(50);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    LCD_Send4Bits(0x03);
    HAL_Delay(5);

    LCD_Send4Bits(0x03);
    HAL_Delay(1);

    LCD_Send4Bits(0x03);
    HAL_Delay(1);

    LCD_Send4Bits(0x02);
    HAL_Delay(1);

    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);

    HAL_Delay(5);
}

uint8_t IsAuthorizedID(uint32_t id)
{
    if (id == 0x100 ||
        id == 0x120 ||
        id == 0x200 ||
        id == 0x300 ||
        id == 0x400)
    {
        return 1;
    }

    return 0;
}

void ShowResult(uint32_t id)
{
    char line[17];

    if (IsAuthorizedID(id))
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

        /* Screen 1 */
        LCD_Command(0x01);
        HAL_Delay(2);

        sprintf(line, "ID: 0x%03lX", id);
        LCD_Command(0x80);
        LCD_String(line);

        LCD_Command(0xC0);
        LCD_String("AUTHORIZED");

        HAL_Delay(2500);

        /* Screen 2 */
        LCD_Command(0x01);
        HAL_Delay(2);

        LCD_Command(0x80);
        LCD_String("USER: PRABHAT");

        LCD_Command(0xC0);
        LCD_String("ACCESS GRANTED");

        HAL_Delay(2500);

        /* Screen 3 */
        LCD_Command(0x01);
        HAL_Delay(2);

        LCD_Command(0x80);
        LCD_String("VEHICLE: CAR-01");

        LCD_Command(0xC0);
        LCD_String("OWNER: PRABHAT");

        HAL_Delay(2500);

        /* Screen 4 */
        LCD_Command(0x01);
        HAL_Delay(2);

        LCD_Command(0x80);
        LCD_String("SPEED: 60 KM/H");

        LCD_Command(0xC0);
        LCD_String("TEMP: 32 DEG C");

        HAL_Delay(2500);

        /* Screen 5 */
        LCD_Command(0x01);
        HAL_Delay(2);

        LCD_Command(0x80);
        LCD_String("ENGINE: ACTIVE");

        LCD_Command(0xC0);
        LCD_String("BRAKE: NORMAL");

        HAL_Delay(2500);

        /* Screen 6 */
        LCD_Command(0x01);
        HAL_Delay(2);

        LCD_Command(0x80);
        LCD_String("GATEWAY STATUS");

        LCD_Command(0xC0);
        LCD_String("DATA FORWARDED");

        HAL_Delay(2500);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);

        /* Unauthorized screen */
        LCD_Command(0x01);
        HAL_Delay(2);

        sprintf(line, "ID: 0x%03lX", id);
        LCD_Command(0x80);
        LCD_String(line);

        LCD_Command(0xC0);
        LCD_String("UNAUTHORIZED");

        HAL_Delay(2500);

        /* Alert screen */
        LCD_Command(0x01);
        HAL_Delay(2);

        LCD_Command(0x80);
        LCD_String("ACCESS DENIED");

        LCD_Command(0xC0);
        LCD_String("INTRUSION ALERT");

        HAL_Delay(2500);
    }
}

void ReadManualID(void)
{
    uint8_t received_char;
    char buffer[10];
    uint8_t index = 0;
    uint32_t id;

    char message[] = "\r\nEnter CAN ID in HEX: ";
    HAL_UART_Transmit(&huart2,
                      (uint8_t *)message,
                      strlen(message),
                      100);

    while (1)
    {
        HAL_UART_Receive(&huart2,
                         &received_char,
                         1,
                         HAL_MAX_DELAY);

        if (received_char == '\r' || received_char == '\n')
        {
            if (index > 0)
            {
                buffer[index] = '\0';

                id = strtoul(buffer, NULL, 16);

                ShowResult(id);

                index = 0;
                memset(buffer, 0, sizeof(buffer));

                HAL_Delay(3000);

                char next_message[] =
                    "\r\nEnter next CAN ID in HEX: ";

                HAL_UART_Transmit(&huart2,
                                  (uint8_t *)next_message,
                                  strlen(next_message),
                                  100);
            }
        }
        else
        {
            if (index < sizeof(buffer) - 1)
            {
                buffer[index] = received_char;
                index++;

                HAL_UART_Transmit(&huart2,
                                  &received_char,
                                  1,
                                  100);
            }
        }
    }
}

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();

    MX_USART2_UART_Init();

    LCD_Init();

    LCD_Command(0x80);
    LCD_String("SECURE GATEWAY");

    LCD_Command(0xC0);
    LCD_String("READY FOR ID");

    HAL_Delay(3000);

    while (1)
    {
        ReadManualID();
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                            FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_USART2_UART_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA8 = Green LED, PA9 = Red LED */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* LCD: PB0 to PB5 */
    GPIO_InitStruct.Pin =
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
        GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART2: PA2 = TX, PA3 = RX */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9);
        HAL_Delay(200);
    }
}
