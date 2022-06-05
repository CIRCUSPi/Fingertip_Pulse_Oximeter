/*
 * @Author : Zack Huang
 * @Link :
 * @Date : 6/5/2022, 12:29:47 PM
 *
 * Hardware Used
 * Development board: TTGO T-Display >> https://www.icshop.com.tw/product-page.php?28572
 * Pulse Oximeter Sensor: MAX30100 >> https://www.icshop.com.tw/product-page.php?28034
 * LiPo Battery: Lipo 500mAh 3.7V 602535 >> https://www.icshop.com.tw/product-page.php?14047
 *
 * Board Config: ESP32 Dev Module
 *
 * Librarys
 *
 * Arduino Library Manager:
 * LittleFS_esp32(V1.0.6)       >> https://github.com/lorol/LITTLEFS
 * MAX30100lib(V1.2.1)          >> https://github.com/oxullo/Arduino-MAX30100
 * TJpg_Decoder(V1.0.5)         >> https://github.com/Bodmer/TJpg_Decoder
 *
 * Github
 * 18650CL(e1be2aa)             >> https://github.com/pangodream/18650CL
 * TFT_eSPI(4ced37a)            >> https://github.com/Bodmer/TFT_eSPI
 * U8g2_for_TFT_eSPI(a170ef8)   >> https://github.com/Bodmer/U8g2_for_TFT_eSPI
 *
 * Notice: The MAX30100 uses high-speed(400k) I2C,
 * and the distance of the transmission line
 * should be kept within 10 cm to avoid transmission failure.
 *
 * TODO: 無使用自動進入省電模式，節省電力
 * TODO: 透過按鈕選擇顯示方向
 * TODO: 顯示心律單直條圖
 * TODO: 顯示開機頁面版本資訊等等
 * TODO: 顯示電池電壓
 * TODO: 驗證省電模式續航力
 * TODO: 結合APP紀錄？
 */

#include "config.h"

#include <SPI.h>
#include <Wire.h>

#include <LITTLEFS.h>
#include <MAX30100_PulseOximeter.h>
#include <Pangodream_18650_CL.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <U8g2_for_TFT_eSPI.h>

/* #region Battery object and global var */
Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);
String              batteryImages[] = {"/hori_battery_01.jpg", "/hori_battery_02.jpg", "/hori_battery_03.jpg", "/hori_battery_04.jpg", "/hori_battery_05.jpg"};
/* #endregion */

/* #region  Max30100 object and global var */
PulseOximeter pox;
uint8_t       heart_rate_buff[HEART_RATE_BUFF_SIZE] = {0};
uint8_t       spo2                                  = 0;
uint8_t       heart_rate                            = 0;
bool          detect_beat                           = false;
bool          show_chart                            = false;
uint32_t      chart_clear_timer                     = 0;
/* #endregion */

/* #region  TFT object and global var */
TFT_eSPI          tft   = TFT_eSPI();
TFT_eSprite       graph = TFT_eSprite(&tft);
U8g2_for_TFT_eSPI u8g2;
int32_t           chartNumList[CHART_BUFF_SIZE] = {-1};
/* #endregion */

/* #region  Function Declaration */
void SPIFFSInit(void);
void TFTLibsInit(void);
void MAX30100Init(void);
void GPIOInit(void);
void TaskMax30100(void);
void ShowVersion(void);
void TaskDisplay(void);
void drawBatIcon(String filePath);
bool onTJpgDecoded(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
void drawTFTchart(int32_t *myNumList, byte chartType, bool dirType, uint16_t myColor);
void clearTFTchartBuff();
void onBeatDetected();
/* #endregion */

/* #region  Arduino Setup */
void setup()
{
#if DEBUG_MODE
    debugSerial.begin(115200);
#endif
    GPIOInit();
    TFTLibsInit();
    SPIFFSInit();
    ShowVersion();
    delay(5000);
    MAX30100Init();
}
/* #endregion */

/* #region  SPIFFS Initialization */
void SPIFFSInit(void)
{
    if (!SPIFFS.begin()) {
        DEBUG_PRINTLN(F("SPIFFS initialisation failed!"));
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.drawString(String("SPIFFS FAILED"), 30, 55, 4);
        while (1)
            yield();
    }
    DEBUG_PRINTLN(F("\r\nSPIFFS Initialisation done."));
}
/* #endregion */

/* #region  TFT Librars Initialization */
void TFTLibsInit(void)
{
    // TFT
    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(3);
    tft.setSwapBytes(true);
    // u8g2
    u8g2.begin(tft);
    u8g2.setForegroundColor(TFT_WHITE);
    // TJpgDec
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(onTJpgDecoded);
    // other
    clearTFTchartBuff();
}
/* #endregion */

/* #region  Max30100 Initialization */
void MAX30100Init(void)
{
    tft.fillScreen(TFT_BLACK);
    if (!pox.begin()) {
        DEBUG_PRINTLN(F("MAX30100 Initialization Failed !"));
        tft.setTextColor(TFT_RED);
        tft.drawString(String("MAX30100 FAILED"), 13, 55, 4);
        while (true)
            yield();
    } else {
        DEBUG_PRINTLN(F("MAX30100 Initialization Success !"));
    }
    // LED Configuration
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    // Register a callback for the beat detection
    pox.setOnBeatDetectedCallback(onBeatDetected);

    // show initialization surface
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 3, 4);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.print("%SpO2");
    tft.setCursor(153, 38, 4);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("PRbpm");
}
/* #endregion */

/* #region  GPIO Initialization */
void GPIOInit(void)
{
    // pinMode(14, OUTPUT);
    // digitalWrite(14, HIGH);
}
/* #endregion */

/* #region  Arduino Loop */
void loop()
{
    TaskMax30100();
    TaskDisplay();
}
/* #endregion */

/* #region  Show Device Name and Version */
void ShowVersion(void)
{
    tft.fillScreen(TFT_BLACK);
    TJpgDec.drawFsJpg(0, 20, LOGO_PATH);
    tft.setTextColor(TFT_GREEN);
    tft.drawString(String(DEVICE_NAME), 125, 40, 4);
    tft.setTextColor(TFT_ORANGE);
    tft.drawString(String("Version: "), 135, 70, 4);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString(String(Version), 140, 100, 4);
}
/* #endregion */

/* #region  Task Max30100 */
void TaskMax30100(void)
{
    static uint32_t sensor_get_timer            = 0;
    static uint32_t pulse_get_timer             = 0;
    static uint8_t  hr_buf_idx                  = 0;
    static float    pulse_buff[PULSE_BUFF_SIZE] = {0.0f};
    static uint8_t  pls_buf_idx                 = 0;

    // update pulse oximeter
    pox.update();

    if (millis() > sensor_get_timer) {
        // Get HeartRate and SpO2 Data
        sensor_get_timer              = millis() + SENSOR_GET_TIMER_MS;
        heart_rate_buff[hr_buf_idx++] = (uint8_t)pox.getHeartRate();
        spo2                          = pox.getSpO2();
        // Check buff index
        if (hr_buf_idx >= HEART_RATE_BUFF_SIZE)
            hr_buf_idx = 0;
        // Calc avg heart rate
        uint16_t avg_hr = 0;
        for (uint8_t i = 0; i < HEART_RATE_BUFF_SIZE; i++)
            avg_hr += heart_rate_buff[i];
        heart_rate = (uint8_t)(avg_hr / HEART_RATE_BUFF_SIZE);
    }

    if (millis() > pulse_get_timer) {
        pulse_get_timer = millis() + PULSE_GET_TIMER_MS;
        // Get sensor filtered Pulse Value
        pulse_buff[pls_buf_idx++] = pox.ftPlsVal;
        // Check buff index
        if (pls_buf_idx >= PULSE_BUFF_SIZE)
            pls_buf_idx = 0;

        float   min_avg = 0.0f, max_avg = 0.0f;
        uint8_t min_num = 0, max_num = 0;
        for (int i = 0; i < PULSE_BUFF_SIZE; i++) {
            // total min pulse value
            if (pulse_buff[i] < 0) {
                min_avg += pulse_buff[i];
                min_num++;
            }
            // total max pulse value
            if (pulse_buff[i] > 0) {
                max_avg += pulse_buff[i];
                max_num++;
            }
        }
        // Calc average
        min_avg /= min_num;
        max_avg /= max_num;
        // Generate chart values
        if (millis() > chart_clear_timer)
            clearTFTchartBuff();
        else
            // FIXME: 重新校正?
            chartNumList[CHART_BUFF_SIZE - 1] = (int32_t)(map(pox.ftPlsVal, min_avg, max_avg, 10, 0) + 10);
        show_chart = true;
    }
}
/* #endregion */

/* #region  Task TFT Display */
void TaskDisplay(void)
{
    static uint32_t show_bat_icon_timer  = 0;
    static uint32_t show_beat_spo2_timer = 0;
    static uint8_t  icon_idx             = 0;
    static uint8_t  bat_state = BAT_DISCHRG, pre_bat_state = BAT_DISCHRG;

    char str_buff[8] = {'\0'};
    int  batteryLevel;

    // Display heartbeat and SpO2 at fixed time or when a heartbeat is detected
    if (detect_beat || millis() > show_beat_spo2_timer) {
        show_beat_spo2_timer = millis() + SHOW_SENSOR_DATA_TIMER_MS;
        detect_beat          = false;
        // Show HeartRate
        sprintf(str_buff, "%03d", GET_MIN(heart_rate, 199));
        tft.setCursor(153, 67);
        tft.setTextSize(1);
        tft.setTextFont(6);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.print(str_buff);
        // Show SPO2
        sprintf(str_buff, "%02d", GET_MIN(spo2, 99));
        tft.setCursor(0, 30);
        tft.setTextSize(1);
        tft.setTextFont(8);
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        tft.print(str_buff);
    }

    // show battery icon
    if (BL.getBatteryVolts() >= MIN_USB_VOL) {
        bat_state = BAT_CHRG;
        // Charge
        if (millis() > show_bat_icon_timer) {
            show_bat_icon_timer = millis() + CHRG_BAT_ICON_TIMER_MS;
            // Draw Bat Icon
            drawBatIcon(batteryImages[icon_idx++]);
            // check bat icon idx
            if (icon_idx >= ARRAY_SIZE(batteryImages))
                icon_idx = 0;
        }
    } else {
        bat_state = BAT_DISCHRG;
        // Discharge
        if (millis() > show_bat_icon_timer) {
            show_bat_icon_timer = millis() + DISCHRG_BAT_ICON_TIMER_MS;
            int imgNum          = 0;
            batteryLevel        = BL.getBatteryChargeLevel();
            if (batteryLevel >= 80)
                imgNum = 3;
            else if (batteryLevel < 80 && batteryLevel >= 50)
                imgNum = 2;
            else if (batteryLevel < 50 && batteryLevel >= 20)
                imgNum = 1;
            else if (batteryLevel < 20)
                imgNum = 0;
            drawBatIcon(batteryImages[imgNum]);
        }
    }

    // show battery State/Level
    if (bat_state != pre_bat_state) {
        pre_bat_state = bat_state;
        memset(str_buff, '\0', sizeof(str_buff));
        tft.fillRect(105, 7, 63, 25, TFT_BLACK);
        tft.setTextSize(1);
        tft.setTextFont(4);
        switch (bat_state) {
        case BAT_CHRG:
            tft.setCursor(110, 7);
            tft.setTextColor(TFT_RED, TFT_BLACK);
            strcpy(str_buff, "Chrg");
            break;
        case BAT_DISCHRG:
            sprintf(str_buff, "%3d%%", GET_MIN(batteryLevel, 100));
            tft.setCursor(105, 7);
            tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
            // Show battery Level
            break;
        default:
            break;
        }
        tft.print(str_buff);
    }

    // Show heart rate bar graph
    if (show_chart) {
        show_chart = false;
        graph.createSprite(tft.width(), 30);
        drawTFTchart(chartNumList, 1, false, tft.color565(0, 0, 255));
        graph.pushSprite(0, 105);
        graph.deleteSprite();
    }

    DEBUG_PRINT(F("HR:   "));
    DEBUG_PRINT(heart_rate);
    DEBUG_PRINT(F(" , SPO2: "));
    DEBUG_PRINTLN(spo2);
}
/* #endregion */

/* #region  Draw Battery Icon */
void drawBatIcon(String filePath)
{
    TJpgDec.drawFsJpg(ICON_POS_X, 0, filePath);
}
/* #endregion */

/* #region  Tjpg decode callback  */
bool onTJpgDecoded(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    if (y >= tft.height())
        return 0;
    // FIXME: Use DMA ?
    tft.pushImage(x, y, w, h, bitmap);
    return 1;
}
/* #endregion */

/* #region  Draw TFT chart */
/*
 * myNumList >> 顯示buff，此方法將會移動buff內容
 * chartType >> 顯示類型，0=折線圖，1=長條圖
 * dirType   >> 刷新方向，true=左到右，false=右到左
 * myColor   >> 顯示顏色
 */
void drawTFTchart(int32_t *myNumList, byte chartType, bool dirType, uint16_t myColor)
{
    byte myWidth = tft.width(), myHeight = tft.height();
    graph.fillSprite(TFT_BLACK);
    for (int i = 0; i < ((chartType == 0) ? (myWidth - 1) : myWidth); i++) {
        if ((myNumList[i]) > -1 && (myNumList[i + 1]) > -1) {
            switch (chartType) {
            case 0:
                if (!dirType)
                    graph.drawLine(i, myNumList[i], i + 1, myNumList[i + 1], myColor);
                else
                    graph.drawLine(myWidth - 1 - i, myNumList[i], myWidth - 2 - i, myNumList[i + 1], myColor);
                break;
            case 1:
                if (!dirType)
                    graph.drawLine(i, myHeight - 1, i, myNumList[i], myColor);
                else
                    graph.drawLine(myWidth - 1 - i, myHeight - 1, myWidth - 1 - i, myNumList[i], myColor);
                break;
            }
        }
    }
    for (int i = 0; i < (myWidth - 1); i++) {
        myNumList[i] = (myNumList[i + 1]);
    }
}
/* #endregion */

/* #region  Clear heart rate graph buffer */
void clearTFTchartBuff()
{
    for (int i = 0; i < CHART_BUFF_SIZE; i++)
        chartNumList[i] = -1;
}
/* #endregion */

/* #region  Callback fired when a pulse is detected */
void onBeatDetected()
{
    chart_clear_timer = millis() + CHART_CLEAR_TIMER_MS;
    detect_beat       = true;
}
/* #endregion */
