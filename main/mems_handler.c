#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/i2c.h"

#include "mems_handler.h"
#include "ui_handler.h"
#include "led_handler.h"

static const char *TAG = "GuitarOS(MEMS)";

// I2C common protocol defines
#define WRITE_BIT                          I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                           I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */

#define MEMS_ADDR 0x6B


static esp_err_t i2c_master_read_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_rd, size_t size)
{
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // first, send device address (indicating write) & register to be read
    i2c_master_write_byte(cmd, ( i2c_addr << 1 ), ACK_CHECK_EN);
    // send register we want
    i2c_master_write_byte(cmd, i2c_reg, ACK_CHECK_EN);
    // Send repeated start
    i2c_master_start(cmd);
    // now send device address (indicating read) & read data
    i2c_master_write_byte(cmd, ( i2c_addr << 1 ) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t i2c_master_write_slave_reg(i2c_port_t i2c_num, uint8_t i2c_addr, uint8_t i2c_reg, uint8_t* data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // first, send device address (indicating write) & register to be written
    i2c_master_write_byte(cmd, ( i2c_addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    // send register we want
    i2c_master_write_byte(cmd, i2c_reg, ACK_CHECK_EN);
    // write the data
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static void mems_task(void *arg)
{
    static int16_t xx = 100, yy = 800;

    ESP_LOGI(TAG, "Starting MEMS task");
    while (1) {
        uint8_t axl,axh,ayl,ayh,azl,azh;
        i2c_master_read_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x35, &axl, 1);
        i2c_master_read_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x36, &axh, 1);
        i2c_master_read_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x37, &ayl, 1);
        i2c_master_read_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x38, &ayh, 1);
        i2c_master_read_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x39, &azl, 1);
        i2c_master_read_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x3a, &azh, 1);
        int16_t ax = ((int16_t)axh << 8) | axl;
        int16_t ay = ((int16_t)ayh << 8) | ayl;
        int16_t az = ((int16_t)azh << 8) | azl;
        //ESP_LOGI(TAG, "ax : %d, ay : %d, az : %d", ax,ay,az);
#if 0
        ax -= xx;
        ay -= yy;
        
        uint8_t s = 2;
        while ((abs(ax >> s) >= 120) || (abs(ay >> s) >= 120))
        {
            s += 3;
        }

        show_xy(-(ay >> s), (ax >> s), 5 + (s * 5));
#else
        static double last_x = 0;
        static double last_y = 0;
        static double last_z = 0;
        
        double x = (double)((int32_t)ax + 0x4000) / (double)0x8000;
        double y = (double)((int32_t)ay + 0x4000) / (double)0x8000;
        double z = (double)((int32_t)az + 0x4000) / (double)0x8000;
        //ESP_LOGI(TAG, "x : %f, y : %f, z : %f", x,y,z);
        //add_point(0, 1, 3, rgb_to_col(x,y,z), 255);

        set_col(rgb_to_col(last_x-x, last_y-y, last_z-z));

        last_x = x;
        last_y = y;
        last_z = z;
#endif        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void configure_mems(void)
{
    ESP_LOGI(TAG, "Configuring MEMS!");

    uint8_t DevID, RevID;
    i2c_master_read_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x00, &DevID, 1);
    i2c_master_read_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x01, &RevID, 1);
    ESP_LOGI(TAG, "Dev ID : %02x, Rev ID : %02x", DevID,RevID);

    uint8_t ctrl7 = 0x03;
    i2c_master_write_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x08, &ctrl7, 1);

    uint8_t ctrl2;
    i2c_master_read_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x03, &ctrl2, 1);
    ctrl2 = (ctrl2 & 0xf0) | 0x08;
    i2c_master_write_slave_reg((i2c_port_t)0, MEMS_ADDR, 0x03, &ctrl2, 1);

    xTaskCreate(mems_task, "MEMS", (4 * 1024), NULL, 2, NULL);
}