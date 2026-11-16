
#include <stdio.h>
#include <string.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_err.h"
#include "esp_log.h"

#include "esp_system.h"
#include "esp_chip_info.h"

#define UART_PORT       UART_NUM_1
#define UART_TX_PIN     GPIO_NUM_17
#define UART_RX_PIN     GPIO_NUM_16
#define RS485_DIR_GPIO  GPIO_NUM_18
#define MODBUS_REQUEST_LENGTH 8U

#define UART_BAUD_RATE  9600U

static const char *TAG = "DEVICE_2";

//Modbus Request Frame Structure
#define MODBUS_DEVICE_ADDRESS       0x01U
#define MODBUS_FUNCTION_READ_HOLDING 0x03U

#define HOLDING_REGISTER_START      0x0010U
#define HOLDING_REGISTER_COUNT      2U

static const uint16_t holding_registers[HOLDING_REGISTER_COUNT] =
{
    25U,     // Address 0x0010: Temperature
    230U     // Address 0x0011: Voltage
};

static uint16_t modbus_crc16(const uint8_t *data,size_t length)
{
    uint16_t crc = 0xFFFFU;

    for (size_t i = 0U; i < length; ++i)
    {
        crc ^= (uint16_t)data[i];

        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            if ((crc & 0x0001U) != 0U)
            {
                crc = (uint16_t)(
                    (crc >> 1U) ^ 0xA001U
                );
            }
            else
            {
                crc = (uint16_t)(crc >> 1U);
            }
        }
    }

    return crc;
}

static bool modbus_validate_crc(const uint8_t *frame, size_t length)
{
    if ((frame == NULL) || (length < 3U))
    {
        return false;
    }

    /* Calculate CRC for all bytes except the last two */
    uint16_t calculated_crc = modbus_crc16(
        frame,
        length - 2U
    );

    /* Extract received CRC: low byte first */
    uint16_t received_crc =
        (uint16_t)frame[length - 2U] |
        ((uint16_t)frame[length - 1U] << 8U);

    ESP_LOGI(TAG, "Calculated CRC: 0x%04X",
             (unsigned int)calculated_crc);

    ESP_LOGI(TAG, "Received CRC:   0x%04X",
             (unsigned int)received_crc);

    return (calculated_crc == received_crc);
}

static void process_modbus_request(const uint8_t *frame, size_t length)
{
    if ((frame == NULL) || (length != MODBUS_REQUEST_LENGTH))
    {
        ESP_LOGE(TAG, "Invalid frame length");
        return;
    }

    /* Check device address */
    if (frame[0] != MODBUS_DEVICE_ADDRESS)
    {
        ESP_LOGW(TAG, "Request for another device");
        return;
    }

    /* Check function code */
    if (frame[1] != MODBUS_FUNCTION_READ_HOLDING)
    {
        ESP_LOGW(TAG, "Unsupported function code");
        return;
    }

    /* Extract starting register address */
    uint16_t start_address =
        ((uint16_t)frame[2] << 8U) |
        (uint16_t)frame[3];

    /* Extract number of registers */
    uint16_t quantity =
        ((uint16_t)frame[4] << 8U) |
        (uint16_t)frame[5];

    ESP_LOGI(TAG, "Starting address: 0x%04X",
             (unsigned int)start_address);

    ESP_LOGI(TAG, "Register quantity: %u",
             (unsigned int)quantity);

    /* Validate requested register range */
    /* These checks prevent zero-length or out-of-range reads and ensure the
     * later register index/count stay within holding_registers. The ordering
     * also prevents unsigned subtraction from underflowing. */
    if ((quantity == 0U) ||
        (start_address < HOLDING_REGISTER_START) ||
        (quantity > HOLDING_REGISTER_COUNT) ||
        ((start_address - HOLDING_REGISTER_START) >
         (HOLDING_REGISTER_COUNT - quantity)))
    {
        ESP_LOGE(TAG, "Invalid register range");
        return;
    }

    /* Read requested registers */
    size_t register_index =
        (size_t)(start_address - HOLDING_REGISTER_START);

    for (uint16_t i = 0U; i < quantity; i++)
    {
        ESP_LOGI(TAG, "Register 0x%04X = %u",
                 (unsigned int)(start_address + i),
                 (unsigned int)holding_registers[register_index + i]);
    }
}

void app_main(void)
{
    /*esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("\n========== ESP32 CHIP INFORMATION ==========\n");

    switch (chip_info.model)
    {
        case CHIP_ESP32:
            printf("Chip Model: ESP32\n");
            break;

        case CHIP_ESP32S2:
            printf("Chip Model: ESP32-S2\n");
            break;

        case CHIP_ESP32S3:
            printf("Chip Model: ESP32-S3\n");
            break;

        case CHIP_ESP32C3:
            printf("Chip Model: ESP32-C3\n");
            break;

        default:
            printf("Chip Model: Other ESP32 variant (ID: %d)\n",
                   chip_info.model);
            break;
    }

    printf("Chip Revision: %d\n", chip_info.revision);
    printf("CPU Cores: %d\n", chip_info.cores);

    printf("============================================\n");
    */
    
    /* Configure UART1 for 9600 baud, 8 data bits, no parity, and one stop bit. */
    uart_config_t uart_config =
    {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(
        uart_param_config(UART_PORT, &uart_config)
    );

    ESP_ERROR_CHECK(
        uart_set_pin(
            UART_PORT,
            UART_TX_PIN,
            UART_RX_PIN,
            RS485_DIR_GPIO,
            UART_PIN_NO_CHANGE
        )
    );

        ESP_ERROR_CHECK(
        uart_driver_install(
            UART_PORT,
            1024,
            1024,
            0,
            NULL,
            0
        )
    );

    ESP_ERROR_CHECK(
        uart_set_mode(UART_PORT, UART_MODE_RS485_HALF_DUPLEX)
    );


    /* Keep the MAX485 transceiver in receive mode until a transmission is requested. */
    ESP_ERROR_CHECK(
        gpio_set_direction(RS485_DIR_GPIO, GPIO_MODE_OUTPUT)
    );
    ESP_ERROR_CHECK(
        gpio_set_level(RS485_DIR_GPIO, 0)
    );

    // uint8_t rx_buffer[128];

    ESP_LOGI(TAG, "RS485 receiver ready");

    uint8_t rx_buffer[MODBUS_REQUEST_LENGTH];

    
    while (1)
    {
        int rx_length = uart_read_bytes(
        UART_PORT,
        rx_buffer,
        sizeof(rx_buffer),
        pdMS_TO_TICKS(100U)
    );

    ESP_LOGI(TAG, "Received %d bytes", rx_length);
    if (rx_length > 0)
    {
        ESP_LOGI(TAG, "Received %d bytes:", rx_length);

        for (int i = 0; i < rx_length; i++)
        {
            ESP_LOGI(TAG, "Byte[%d] = 0x%02X",i, (unsigned int)rx_buffer[i]);
        }
    }
    else
    {
        ESP_LOGW(TAG, "No Modbus request received");
    }

    if (rx_length == MODBUS_REQUEST_LENGTH)
    {
        if (modbus_validate_crc(rx_buffer, (size_t)rx_length))
        {
            ESP_LOGI(TAG, "Modbus CRC validation: PASSED");
            process_modbus_request(rx_buffer, (size_t)rx_length);
        }
        else
        {
            ESP_LOGE(TAG, "Modbus CRC validation: FAILED");
        }
    }
    else
    {
        ESP_LOGW(TAG, "Incomplete Modbus frame: %d bytes", rx_length);
    }

        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}