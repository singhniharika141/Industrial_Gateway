#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"

static const char *TAG = "DEVICE_1";
#define UART_PORT       UART_NUM_1
#define UART_TX_PIN    GPIO_NUM_17
#define UART_RX_PIN    GPIO_NUM_16
#define RS485_DIR_GPIO GPIO_NUM_4
#define UART_BAUD_RATE  9600U

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
    while (1) {
        ESP_LOGI(TAG, "Sending message...");

        rs485_send("Hello from Gateway\r\n");

        vTaskDelay(pdMS_TO_TICKS(1000U)); // Delay for 2000 milliseconds (2 seconds)
    }
}