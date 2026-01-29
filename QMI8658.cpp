#include "QMI8658.h"
#include "I2C_Driver.h"
#include <Arduino.h>

IMUdata Accel;
IMUdata Gyro;

// 辅助函数：写一个字节
void QMI8658_transmit(uint8_t reg, uint8_t data) {
    I2C_Write(QMI8658_L_SLAVE_ADDRESS, reg, &data, 1);
}

// 辅助函数：读一个字节
uint8_t QMI8658_receive(uint8_t reg) {
    uint8_t data = 0;
    I2C_Read(QMI8658_L_SLAVE_ADDRESS, reg, &data, 1);
    return data;
}

void QMI8658_Init(void) {
    // 1. 初始化底层 I2C (引脚 47, 48)
    I2C_Init();
    delay(10);

    // 2. 检查 ID
    uint8_t id = QMI8658_receive(QMI8658_WHO_AM_I);
    Serial.printf("QMI8658 ID: 0x%02X\n", id); // 应该是 0x05

    // 3. 软复位
    QMI8658_transmit(0x60, 0xB6);
    delay(20);

    // 4. 开启 ACC 和 GYR
    QMI8658_transmit(QMI8658_CTRL1, 0x40); // Address auto increment
    QMI8658_transmit(QMI8658_CTRL7, 0x03); // Enable Acc & Gyr

    // 5. 配置 ACC (4g, 1000Hz) -> 0x13
    QMI8658_transmit(QMI8658_CTRL2, 0x13);

    // 6. 配置 GYR (512dps, 250Hz) -> 0x55
    QMI8658_transmit(QMI8658_CTRL3, 0x55);
}

void QMI8658_Loop(void) {
    uint8_t buf[12];
    // 从 0x35 (AX_L) 开始连续读 12 个字节
    if (I2C_Read(QMI8658_L_SLAVE_ADDRESS, QMI8658_AX_L, buf, 12)) {
        int16_t ax = buf[0] | (buf[1] << 8);
        int16_t ay = buf[2] | (buf[3] << 8);
        int16_t az = buf[4] | (buf[5] << 8);
        int16_t gx = buf[6] | (buf[7] << 8);
        int16_t gy = buf[8] | (buf[9] << 8);
        int16_t gz = buf[10] | (buf[11] << 8);

        // 转换
        Accel.x = (float)ax / 8192.0f;
        Accel.y = (float)ay / 8192.0f;
        Accel.z = (float)az / 8192.0f;
        
        Gyro.x = (float)gx / 64.0f;
        Gyro.y = (float)gy / 64.0f;
        Gyro.z = (float)gz / 64.0f;
    }
}
