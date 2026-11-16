#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <stdint.h>
#include <stddef.h>

static const char *TAG = "DEVICE_1";
#define UART_PORT       UART_NUM_1
#define UART_TX_PIN    GPIO_NUM_17
#define UART_RX_PIN    GPIO_NUM_16
#define RS485_DIR_GPIO GPIO_NUM_4
#define UART_BAUD_RATE  9600U

//CRC16 calculation for Modbus RTU
#define MODBUS_REQUEST_LENGTH    8U
#define MODBUS_DATA_LENGTH       6U

//Initializes the UART
static void uart_init(void)
{
    const uart_config_t uart_config =
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
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        )
    );
//why we gave 1024 size for rx_buffer_size and tx_buffer_size in uart_driver_install function?
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
}

// Initializes the RS485 direction control GPIO
static void rs485_init(void)
{
    ESP_ERROR_CHECK(
        gpio_set_direction(
            RS485_DIR_GPIO,
            GPIO_MODE_OUTPUT
        )
    );

    /* Start in receive mode */
    ESP_ERROR_CHECK(
        gpio_set_level(RS485_DIR_GPIO, 0)
    );
}

static void rs485_send(const char *message)
{
    /* Enable RS485 transmitter */
    ESP_ERROR_CHECK(
        gpio_set_level(RS485_DIR_GPIO, 1)
    );

    int bytes_written = uart_write_bytes(
        UART_PORT,
        message,
        strlen(message)
    );

    if (bytes_written < 0)
    {
        ESP_LOGE(TAG, "UART transmission failed");
    }

    /*
     * Wait until the UART has physically finished
     * transmitting all bytes.
     */
    ESP_ERROR_CHECK(
        uart_wait_tx_done(
            UART_PORT,
            pdMS_TO_TICKS(1000U)
        )
    );

    /* Return MAX485 to receive mode */
    ESP_ERROR_CHECK(
        gpio_set_level(RS485_DIR_GPIO, 0)
    );
}


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

static void send_modbus_request(void)
{
    uint8_t request_frame[MODBUS_REQUEST_LENGTH] =
    {
        0x01U, //Device Address
        0x03U, //Function Code: Read Holding Registers
        0x00U, //Starting Address High Byte
        0x10U, //Starting Address Low Byte
        0x00U, //Quantity of Registers High Byte
        0x02U, //Quantity of Registers Low Byte
        0x00U, //CRC Low Byte
        0x00U  //CRC High Byte
    };

    /* Calculate CRC for the first six bytes */
    uint16_t crc = modbus_crc16(
        request_frame,
        MODBUS_DATA_LENGTH
    );

    /* Append CRC: low byte first */
    request_frame[6] = (uint8_t)(crc & 0x00FFU);
    request_frame[7] = (uint8_t)((crc >> 8U) & 0x00FFU);

    /* Enable RS485 transmission */
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_4, 1));

    /* Send the complete Modbus frame */
    int bytes_written = uart_write_bytes(
        UART_PORT,
        (const char *)request_frame,
        sizeof(request_frame)
    );

    if (bytes_written == (int)sizeof(request_frame))
    {
        ESP_LOGI(TAG, "Modbus request transmitted");

        /* Wait until transmission is physically complete */
        ESP_ERROR_CHECK(
            uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100))
        );
    }
    else
    {
        ESP_LOGE(TAG, "Modbus transmission failed");
    }

    ESP_LOGI(TAG, "Modbus request sent: %02X %02X %02X %02X %02X %02X %02X %02X",
             request_frame[0], request_frame[1], request_frame[2], request_frame[3],
             request_frame[4], request_frame[5], request_frame[6], request_frame[7]);
    /* Return RS485 transceiver to receive mode */
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_4, 0));
}

void app_main(void)
{
    //Step 1: First firmware
    /*
    ESP_LOGI(TAG, "Industrial Gateway started");
    ESP_LOGI(TAG, "Gateway ID: GW001");
    ESP_LOGI(TAG, "Firmware Version: 1.0.0");
    ESP_LOGI(TAG, "System Status: RUNNING");
    */
    //initialize the UART
    uart_init();
    //initialize the RS485 direction control GPIO
    rs485_init();
    //send a Modbus request to the slave device
    

    while (1) {
        // ESP_LOGI(TAG, "Sending message...");

        //rs485_send("Hello from Gateway\r\n");
        send_modbus_request();
        vTaskDelay(pdMS_TO_TICKS(1000U)); // Delay for 2000 milliseconds (2 seconds)
    }
}