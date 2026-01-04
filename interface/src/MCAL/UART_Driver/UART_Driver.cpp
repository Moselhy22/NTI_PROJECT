#include "UART_Driver.h"

bool UART_Driver::begin(uint8_t uart_num, uint32_t baud_rate, uint8_t rx_pin, uint8_t tx_pin, uint32_t config) {
    HardwareSerial* serial = getSerial(uart_num);
    if (serial) {
        serial->begin(baud_rate, config, rx_pin, tx_pin);
        return true;
    }
    return false;
}

int UART_Driver::available(uint8_t uart_num) {
    HardwareSerial* serial = getSerial(uart_num);
    return serial ? serial->available() : 0;
}

int UART_Driver::read(uint8_t uart_num) {
    HardwareSerial* serial = getSerial(uart_num);
    return serial ? serial->read() : -1;
}

size_t UART_Driver::write(uint8_t uart_num, uint8_t data) {
    HardwareSerial* serial = getSerial(uart_num);
    return serial ? serial->write(data) : 0;
}

size_t UART_Driver::writeBytes(uint8_t uart_num, const uint8_t* buffer, size_t size) {
    HardwareSerial* serial = getSerial(uart_num);
    return serial ? serial->write(buffer, size) : 0;
}

size_t UART_Driver::readBytes(uint8_t uart_num, uint8_t* buffer, size_t size, uint32_t timeout) {
    HardwareSerial* serial = getSerial(uart_num);
    return serial ? serial->readBytes(buffer, size) : 0;
}

HardwareSerial* UART_Driver::getSerial(uint8_t uart_num) {
    switch(uart_num) {
        case 0: return &Serial;   // Usually USB
        case 1: return &Serial1;  // GPIO9 (RX), GPIO10 (TX)
        case 2: return &Serial2;  // GPIO16 (RX), GPIO17 (TX) - For GPS
        default: return nullptr;
    }
}