set(srcs "spi_eeprom_main.c")

idf_component_register(
    SRCS "spi_eeprom_main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_event esp_driver_spi esp_driver_gpio nvs_flash spi_flash esp_hw_support
)
