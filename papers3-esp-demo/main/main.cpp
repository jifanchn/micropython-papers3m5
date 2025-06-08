#define ARDUINOJSON_DECODE_UNICODE 1
#include <Arduino.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"

#include "epd_highlevel.h"
#include "epdiy.h"

#include "ISprite.h"
#include "ED047TC1Driver.h"
#include "binaryttf.h"

#include "GT911.h"
#include <Arduino_BMI270_BMM150.h>
#include <ReefwingAHRS.h>
#include <MadgwickAHRS.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <vector>

#include "LGFXPaperS3.hpp"
#include "PNGManager.hpp"
#include "I2C_BM8563.h"
#include "gfxfont.h"

#include <functional>

#define TAG "main"
bool need_update    = false;
bool wifi_scan_done = false;
int wifi_best_db    = -100;
String wifi_best_name;
std::vector<std::pair<int, String>> wifiNetworks;
ReefwingAHRS ahrs;
SensorData data;
unsigned long microsPerReading, microsPrevious;
Madgwick filter;
GT911 gt911;
ED047TC1Driver epd;
ISprite MainCanvas(&epd);
float roll, pitch, yaw;
I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire);
bool beep_state                    = false;
SemaphoreHandle_t wifi_scan_semphr = xSemaphoreCreateBinary();

extern const uint8_t image_logo_start[] asm("_binary_logo_start");
extern const uint8_t image_logo_end[] asm("_binary_logo_end");
extern const uint8_t image_ui_start[] asm("_binary_ui_start");
extern const uint8_t image_ui_end[] asm("_binary_ui_end");
extern const uint8_t image_charge_start[] asm("_binary_charge_start");
extern const uint8_t image_charge_end[] asm("_binary_charge_end");
extern const uint8_t image_beep_on_start[] asm("_binary_buzzer_on_start");
extern const uint8_t image_beep_on_end[] asm("_binary_buzzer_on_end");
extern const uint8_t image_beep_off_start[] asm("_binary_buzzer_off_start");
extern const uint8_t image_beep_off_end[] asm("_binary_buzzer_off_end");

enum {
    SD_POSX      = 240,
    SD_POSY      = 190,
    RTC_POSX     = 30,
    RTC_POSY     = 180,
    IMU_POSX     = 20,
    IMU_POSY     = 400,
    BEEP_POSX    = 235,
    BEEP_POSY    = 317,
    BATTERY_POSX = 805,
    BATTERY_POSY = 22,
    CHARGE_POSX  = 730,
    CHARGE_POSY  = 10,
    WIFI_POSY    = 120,
    WIFI_POSX    = 480,
};

void imu_ahrs_task(void *args)
{
    uint32_t t = 0;
    uint32_t diff;

    while (1) {
        // Read data from gyroscope, accelerometer, and magnetometer
        if (IMU.gyroscopeAvailable()) {
            IMU.readGyroscope(data.gx, data.gy, data.gz);
        }
        if (IMU.accelerationAvailable()) {
            IMU.readAcceleration(data.ax, data.ay, data.az);
        }

        // Update the AHRS filter and compute orientation
        if (micros() - microsPrevious < microsPerReading) {
            vTaskDelay(1);
            continue;
        }

        filter.updateIMU(data.gx, data.gy, data.gz, data.ax, data.ay, data.az);
        roll  = filter.getRoll();
        pitch = filter.getPitch();
        yaw   = filter.getYaw();

        diff           = micros() - microsPrevious;
        microsPrevious = micros();
    }
}

#define PIN_ADC_BATTERY GPIO_NUM_3
float getBatteryVoltage()
{
    int x = analogRead(PIN_ADC_BATTERY);
    return x * 3.5 / 4096 * 2;
}

void shutdown(bool lowbat = false)
{
    vTaskDelay(2000);
    MainCanvas.fillSprite(15);
    MainCanvas.pushSprite(MODE_GC16);
    MainCanvas.fillSprite(0);
    MainCanvas.pushSprite(MODE_GC16);
    vTaskDelay(2000);
    MainCanvas.fillSprite(15);
    MainCanvas.pushSprite(MODE_GC16);
    vTaskDelay(2000);
    memcpy(MainCanvas.frameBuffer(0), image_logo_start + 4, image_logo_end - image_logo_start);
    MainCanvas.pushSprite(MODE_GC16);
    ledcWrite(1, 128);
    vTaskDelay(500);
    pinMode(44, OUTPUT);
    digitalWrite(44, HIGH);
    delay(200);
    digitalWrite(44, LOW);
    esp_deep_sleep_start();
}

struct Button {
    int x, y, w, h;
    bool is_pressed;
    std::function<void(struct Button &)> on_pressed;
    std::function<void(struct Button &)> on_draw;
    uint32_t last_press_time;
};

Button buttons[] = {
    {BEEP_POSX, BEEP_POSY, 214, 214, false, [](struct Button &btn) { beep_state = btn.is_pressed; },
     [](struct Button &btn) {
         if (btn.is_pressed) {
             MainCanvas.pushImage(BEEP_POSX + 1, BEEP_POSY, 214, 214, (uint16_t *)(image_beep_on_start + 4), 4);
         } else {
             MainCanvas.pushImage(BEEP_POSX + 1, BEEP_POSY, 214, 214, (uint16_t *)(image_beep_off_start + 4), 4);
         }
         need_update = true;
     },
     0},

    {840, 96, 110, 160, false,
     [](struct Button &btn) {
         if (!btn.is_pressed) {
             return;
         }
         shutdown();
     },
     [](struct Button &btn) {

     },
     0},

    {840, 268, 110, 160, false,
     [](struct Button &btn) {
         if (!btn.is_pressed) {
             return;
         }
         rtc.clearIRQ();
         rtc.SetAlarmIRQ(21);
         shutdown();
     },
     [](struct Button &btn) {

     },
     0},

    {455, 100, 382, 420, false,
     [](struct Button &btn) {
         if (!btn.is_pressed) {
             return;
         }
         xSemaphoreGive(wifi_scan_semphr);
     },
     [](struct Button &btn) {
         if (btn.is_pressed) {
             MainCanvas.setTextDatum(TC_DATUM);
             MainCanvas.drawString("Scanning...", 655, 372);
             need_update = true;
         }
     },
     0},
};

void updateButton(Button &btn)
{
    btn.on_draw(btn);
    btn.on_pressed(btn);
    btn.last_press_time = millis();
}

void sd_test()
{
    MainCanvas.setTextDatum(TL_DATUM);

    char buf[64];
    if (!SD.begin(47, SPI, 20000000)) {
        sprintf(buf, "Not Found");
        goto end;
    }

    if (!SD.open("/test.txt", FILE_WRITE)) {
        sprintf(buf, "Not Found");
        goto end;
    }

    sprintf(buf, "%.1f GiB", SD.cardSize() / 1024.0 / 1024.0 / 1024.0);

end:;
    printf("SD: %s\n", buf);
    MainCanvas.setTextColor(0, 15);
    MainCanvas.fillRect(SD_POSX, SD_POSY, 190, 40, 15);
    MainCanvas.drawString(buf, SD_POSX, SD_POSY);
    need_update = true;
}

void wifiScanTask(void *args)
{
    WiFi.mode(WIFI_STA);  // 设置 Wi-Fi 工作模式为 STA
    WiFi.disconnect();    // 确保断开所有已连接的网络
    delay(100);

    while (1) {
        xSemaphoreTake(wifi_scan_semphr, portMAX_DELAY);
        int numNetworks = WiFi.scanNetworks();  // 开始扫描 Wi-Fi
        wifiNetworks.clear();

        for (int i = 0; i < numNetworks; i++) {
            String ssid = WiFi.SSID(i);
            int rssi    = WiFi.RSSI(i);
            wifiNetworks.push_back({rssi, ssid});  // 保存信号强度和 SSID
        }

        // 按 RSSI 从强到弱排序
        std::sort(wifiNetworks.begin(), wifiNetworks.end(),
                  [](const std::pair<int, String> &a, const std::pair<int, String> &b) {
                      return a.first > b.first;  // 按信号强度排序
                  });

        wifi_best_db   = WiFi.RSSI(0);
        wifi_best_name = WiFi.SSID(0);

        wifi_scan_done = true;  // 标记扫描完成
    }

    vTaskDelete(NULL);  // 删除任务
}

extern "C" void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    if (gt911.begin(41, 42, 48) != ESP_OK) {
        ESP_LOGE(TAG, "GT911 init failed");
    }
    gt911.flush();

    ahrs.begin();
    ahrs.setImuType(ImuType::BMI270_BMM150);
    ahrs.setDOF(DOF::DOF_6);
    ahrs.setFusionAlgorithm(SensorFusion::MADGWICK);
    ahrs.setDeclination(-3.22);  // Set magnetic declination for Shenzhen, China

    filter.begin(100);

    microsPerReading = 1000000 / 100;
    microsPrevious   = micros();

    IMU.begin();

    rtc.begin();

    ledcSetup(1, 4000, 8);
    ledcAttachPin(21, 1);

    pinMode(0, OUTPUT);  // LED
    pinMode(1, OUTPUT);  // EXT GPIO
    pinMode(2, OUTPUT);  // EXT GPIO
    pinMode(4, INPUT);   // CHG_STATE 0: charge, 1: full
    pinMode(5, INPUT);   // USB DET 1: USB-IN

    xTaskCreatePinnedToCore(
        [](void *args) {
            for (int i = 0; i < 5; i++) {
                ledcWrite(1, 128);
                delay(100);
                ledcWrite(1, 0);
                delay(100);
            }

            while (1) {
                digitalWrite(0, HIGH);
                digitalWrite(1, HIGH);
                digitalWrite(2, HIGH);
                if (beep_state) {
                    for (int i = 0; i < 5; i++) {
                        ledcWrite(1, 128);
                        delay(100);
                        ledcWrite(1, 0);
                        delay(100);
                    }
                }
                digitalWrite(0, LOW);
                digitalWrite(1, LOW);
                digitalWrite(2, LOW);
                vTaskDelay(1000);
            }
        },
        "4Pin", 4096, NULL, 5, NULL, 1);

    epd.init();
    MainCanvas.setColorDepth(4);
    MainCanvas.createSprite(960, 540, 1, epd.getFrameBuffer());
    MainCanvas.fillSprite(15);
    MainCanvas.setTextColor(0);
    MainCanvas.setTextSize(2);
    epd.clear();

    for (int gray = 0; gray < 15; gray++) {
        MainCanvas.fillRect(gray * 64, 0, 64, 540, gray);
    }
    MainCanvas.pushSprite(MODE_GC16);

    vTaskDelay(1000);

    memcpy(MainCanvas.frameBuffer(0), image_ui_start + 4, image_ui_end - image_ui_start);
    MainCanvas.pushSprite(MODE_GC16);

    xTaskCreatePinnedToCore(&imu_ahrs_task, "imu_ahrs_task", 8 * 1024, NULL, 10, NULL, 1);
    xTaskCreatePinnedToCore(&wifiScanTask, "wifiScanTask", 8 * 1024, NULL, 10, NULL, 1);

    I2C_BM8563_DateTypeDef dateStruct;
    I2C_BM8563_TimeTypeDef timeStruct;
    dateStruct.year    = 2077;
    dateStruct.month   = 1;
    dateStruct.date    = 1;
    dateStruct.weekDay = 1;
    rtc.setDate(&dateStruct);
    timeStruct.hours   = 12;
    timeStruct.minutes = 0;
    timeStruct.seconds = 0;
    rtc.setTime(&timeStruct);

    roll = pitch = yaw = 0;

    // init display
    MainCanvas.loadFont(binaryttf, sizeof(binaryttf));
    MainCanvas.createRender(28, 32);
    MainCanvas.createRender(24, 32);
    MainCanvas.createRender(32, 32);
    MainCanvas.createRender(40, 32);
    MainCanvas.setTextSize(40);
    MainCanvas.setTextColor(0, 15);
    char buf[64];

    for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
        Button &btn = buttons[i];
        updateButton(btn);
    }
    // need_update = true;

    // 3. SD卡
    SPI.begin(39, 40, 38);
    sd_test();
    MainCanvas.pushSprite(MODE_GC16);
    uint32_t update_time         = 0;
    uint32_t ahrs_update_time    = 0;
    uint32_t gc16_update_time    = 0;
    uint32_t battery_update_time = 0;
    uint32_t wifi_update_time    = 0;
    uint32_t rtc_update_time     = 0;
    uint32_t chg_update_time     = 0;
    uint32_t sd_update_time      = 0;
    int rtc_last_sec             = -1;
    gt911.flush();
    int last_x = -1, last_y = -1;
    while (1) {
        if (gt911.available()) {
            gt911.update();
            int x = gt911.readFingerX(0);
            int y = gt911.readFingerY(0);
            if (x == last_x && y == last_y) {
                gt911.flush();
                vTaskDelay(10);
                goto tpend;
            }
            printf("x %d y %d\n", x, y);
            last_x = x;
            last_y = y;

            for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
                Button &btn = buttons[i];
                if (x > btn.x && x < btn.x + btn.w && y > btn.y && y < btn.y + btn.h) {
                    // 按钮按下，更新状态
                    if (millis() - btn.last_press_time > 200) {
                        btn.is_pressed      = !btn.is_pressed;
                        btn.last_press_time = millis();
                        ledcWrite(1, 128);
                        delay(100);
                        ledcWrite(1, 0);
                    } else {
                        continue;
                    }
                    updateButton(btn);
                    need_update = true;
                    break;
                }
            }

            // wait release
            while (1) {
                if (!gt911.available()) {
                    vTaskDelay(100);
                    if (!gt911.available()) {
                        break;
                    }
                }
                gt911.flush();
                vTaskDelay(10);
            }

            need_update = true;
            gt911.flush();
        }

    tpend:;
        if (millis() - sd_update_time > 500) {
            sd_update_time = millis();
            sd_test();
        }

        // IMU
        if (millis() - ahrs_update_time > 500) {
            ahrs_update_time = millis();
            if (roll != 0 && pitch != 0 && yaw != 0) {
                MainCanvas.setTextColor(0, 15);
                MainCanvas.setTextDatum(TL_DATUM);
                MainCanvas.setTextSize(24);
                MainCanvas.fillRect(15, 390, 207, 132, 15);
                char buf[64];
                sprintf(buf, "AX %04.1f GX %04.1f", data.ax, data.gx);
                MainCanvas.drawString(buf, IMU_POSX, IMU_POSY);
                sprintf(buf, "AY %04.1f GY %04.1f", data.ay, data.gy);
                MainCanvas.drawString(buf, IMU_POSX, IMU_POSY + 40);
                sprintf(buf, "AZ %04.1f GZ %04.1f", data.az, data.gz);
                MainCanvas.drawString(buf, IMU_POSX, IMU_POSY + 80);
                MainCanvas.setTextSize(40);

                need_update = true;
            }
        }

        // Battery
        if (millis() - battery_update_time > 1000) {
            battery_update_time = millis();
            float bat           = getBatteryVoltage();
            MainCanvas.setTextSize(32);
            MainCanvas.setTextDatum(TC_DATUM);
            char buf[64];
            sprintf(buf, "%1.2fV", getBatteryVoltage());
            MainCanvas.drawString(buf, BATTERY_POSX + 50, BATTERY_POSY);
            MainCanvas.setTextSize(40);

            int usb = digitalRead(5);
            if (!usb && (bat < 3.8)) {
                shutdown(true);
            }

            need_update = true;
        }

        // RTC
        if (millis() - rtc_update_time > 1000) {
            rtc_update_time = millis();
            I2C_BM8563_DateTypeDef dateStruct;
            I2C_BM8563_TimeTypeDef timeStruct;
            rtc.getTime(&timeStruct);
            rtc.getDate(&dateStruct);
            if (timeStruct.seconds != rtc_last_sec) {
                if (rtc_last_sec != -1) {
                    MainCanvas.setTextColor(0, 15);
                    char buf[64];
                    MainCanvas.setTextDatum(TC_DATUM);
                    MainCanvas.setTextSize(32);
                    sprintf(buf, "%04d/%02d/%02d", dateStruct.year, dateStruct.month, dateStruct.date);
                    MainCanvas.drawString(buf, RTC_POSX + 90, RTC_POSY);

                    MainCanvas.setTextSize(40);
                    sprintf(buf, "%02d:%02d:%02d", timeStruct.hours, timeStruct.minutes, timeStruct.seconds);
                    MainCanvas.drawString(buf, RTC_POSX + 90, RTC_POSY + 50);
                    need_update = true;
                }
                rtc_last_sec = timeStruct.seconds;
            }
        }

        // Charge and USB
        if (millis() - chg_update_time > 100) {
            chg_update_time = millis();
            int chg         = digitalRead(4);
            int usb         = digitalRead(5);
            char buf[64];
            if ((usb == 1)) {
                MainCanvas.pushImage(CHARGE_POSX, CHARGE_POSY, 64, 64, (uint16_t *)(image_charge_start + 4), 4);
            } else {
                MainCanvas.fillRect(CHARGE_POSX, CHARGE_POSY, 64, 70, 15);
            }
            need_update = true;
        }

        // Wifi List with db
        if (millis() - wifi_update_time > 1000) {
            if (wifi_scan_done) {
                MainCanvas.useFreetypeFont(false);
                MainCanvas.setTextFont(1);
                MainCanvas.setTextSize(2);
                MainCanvas.setTextDatum(TL_DATUM);
                wifi_scan_done   = false;
                wifi_update_time = millis();

                if (wifiNetworks.size() > 0) {
                    MainCanvas.fillRect(WIFI_POSX, WIFI_POSY, 340, 390, 15);
                    char buf[64];
                    int n = wifiNetworks.size() > 15 ? 15 : wifiNetworks.size();
                    if (wifi_best_db > -45) {
                        snprintf(buf, 32, "Best: %s (%d dBm)", wifi_best_name.c_str(), wifi_best_db);
                        MainCanvas.drawString(buf, WIFI_POSX, WIFI_POSY);
                    } else {
                        snprintf(buf, 32, "Best: %s (%d dBm)", wifi_best_name.c_str(), wifi_best_db);
                        MainCanvas.setTextColor(15, 0);
                        MainCanvas.drawString(buf, WIFI_POSX, WIFI_POSY);
                        MainCanvas.setTextColor(0, 15);
                    }
                    for (int i = 1; i < n; i++) {
                        snprintf(buf, 32, "%s (%d dBm)", wifiNetworks[i].second.c_str(), wifiNetworks[i].first);
                        MainCanvas.drawString(buf, WIFI_POSX, WIFI_POSY + i * 30);
                    }
                    need_update = true;
                }

                MainCanvas.useFreetypeFont(true);

                xSemaphoreGive(wifi_scan_semphr);
            }
        }

        if (need_update && millis() - update_time > 100) {
            if (millis() - gc16_update_time > 10000) {
                gc16_update_time = millis();
                MainCanvas.pushSprite(MODE_GC16);
            } else {
                MainCanvas.pushSprite(MODE_DU);
            }
            update_time = millis();
            need_update = false;
        }

        vTaskDelay(1);
    }
}
