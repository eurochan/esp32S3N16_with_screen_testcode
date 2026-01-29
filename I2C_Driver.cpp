#include "I2C_Driver.h"
#include <Wire.h>

void I2C_Init(void) {
    // 这里使用了你头文件里定义的 48(SDA) 和 47(SCL)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_MASTER_FREQ_HZ);
}

bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length) {
    Wire.beginTransmission(Driver_addr);
    Wire.write(Reg_addr);
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom(Driver_addr, (uint8_t)Length) == Length) {
        for (uint32_t i = 0; i < Length; i++) {
            Reg_data[i] = Wire.read();
        }
        return true;
    }
    return false;
}

bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length) {
    Wire.beginTransmission(Driver_addr);
    Wire.write(Reg_addr);
    for (uint32_t i = 0; i < Length; i++) {
        Wire.write(Reg_data[i]);
    }
    return (Wire.endTransmission() == 0);
}
