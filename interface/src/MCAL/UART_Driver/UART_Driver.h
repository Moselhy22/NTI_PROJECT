#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include "../../CFG.h"
#include "../MCAL_Common.h"

class UART_Driver {
public:
    static bool begin(uint8_t uart_num, uint32_t baud_rate, uint8_t rx_pin, uint8_t tx_pin, uint32_t config = SERIAL_8N1);
    static int available(uint8_t uart_num);
    static int read(uint8_t uart_num);
    static size_t write(uint8_t uart_num, uint8_t data);
    static size_t writeBytes(uint8_t uart_num, const uint8_t* buffer, size_t size);
    static size_t readBytes(uint8_t uart_num, uint8_t* buffer, size_t size, uint32_t timeout = 1000);
    
private:
    static HardwareSerial* getSerial(uint8_t uart_num);
};

#endif