
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_err.h"
#include "esp_log.h"

#define UART_PORT       UART_NUM_1
#define UART_TX_PIN     GPIO_NUM_17
#define UART_RX_PIN     GPIO_NUM_16
#define RS485_DIR_GPIO  GPIO_NUM_18

#define UART_BAUD_RATE  9600U

static const char *TAG = "DEVICE_2";

void app_main(void)
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
        gpio_set_direction(
            RS485_DIR_GPIO,
            GPIO_MODE_OUTPUT
        )
    );

    /* Keep the MAX485 in receive mode */
    ESP_ERROR_CHECK(
        gpio_set_level(RS485_DIR_GPIO, 0)
    );

    uint8_t rx_buffer[128];

    ESP_LOGI(TAG, "RS485 receiver ready");

    while (1)
    {
        int bytes_received = uart_read_bytes(
            UART_PORT,
            rx_buffer,
            sizeof(rx_buffer) - 1U,
            pdMS_TO_TICKS(1000U)
        );

        if (bytes_received > 0)
        {
            rx_buffer[bytes_received] = '\0';

            ESP_LOGI(
                TAG,
                "Received: %s",
                (char *)rx_buffer
            );
        }
    }
}