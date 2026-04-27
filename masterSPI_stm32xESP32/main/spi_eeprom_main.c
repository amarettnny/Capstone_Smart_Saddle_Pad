// This SPI Master program for ESP32 is designed to receive one character

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "soc/rtc_periph.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "spi_flash_mmap.h"

#include "driver/gpio.h"
#include "esp_intr_alloc.h"

// Pins in use
#define GPIO_MOSI 10
#define GPIO_MISO 9
#define GPIO_SCLK 8
#define GPIO_CS 1

#define TAG "SPI"

// Main application
void app_main(void)
{
    esp_err_t ret;

    // Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .max_transfer_sz = 8,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1};

    // Configuration for the SPI device on the other side of the bus
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .clock_speed_hz = 1000000,  // 1 MHz
        .duty_cycle_pos = 128,      // 50% duty cycle
        .mode = 0,                  // SPI mode 0
        .spics_io_num = GPIO_CS,    // CS pin
        .cs_ena_posttrans = 3,      // Keep the CS low 3 cycles after transaction
        .queue_size = 1};

    // Initialize the SPI bus
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    printf("spi_bus_initialize %d\n", ret);     // ESP_OK = 0

    spi_device_handle_t spi_handle;
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
    printf("spi_bus_add_device %d\n", ret);     // ESP_OK = 0

    uint8_t tx_data = 0;
    uint8_t rx_data = 0;

spi_transaction_t t = {
    .length = 8,        // 8 bits = 1 byte
    .rxlength = 8,
    .tx_buffer = &tx_data,
    .rx_buffer = &rx_data
};

printf("Master starting transmission 1-10:\n");

while (1)
{
    for (uint8_t i = 1; i <= 10; i++)
    {
        tx_data = i;

        ret = spi_device_transmit(spi_handle, &t);

        printf("Transmitted: %d\n", tx_data);
        printf("Received: %d\n", rx_data);
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // wait 5 seconds before repeating
}
}