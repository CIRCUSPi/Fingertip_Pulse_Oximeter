#ifndef _CONFIG_H
#define _CONFIG_H

/* #region  Device Info */
#define DEVICE_NAME "Pulse Oxi"
#define Version     "1.0.0"
#define LOGO_PATH   "/CIRCUSPI_logo.jpg"
/* #endregion */

/* #region  Development Settings */
#define DEBUG_MODE false
#if DEBUG_MODE
#define debugSerial      Serial
#define DEBUG_PRINT(x)   debugSerial.print(x)
#define DEBUG_PRINTLN(x) debugSerial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif
/* #endregion */

/* #region  TTGO Button GPIO */
#define BUTTON_LEFT  0
#define BUTTON_RIGHT 35
/* #endregion */

/* #region  Timer */
#define SENSOR_GET_TIMER_MS       800
#define PULSE_GET_TIMER_MS        20
#define CHART_CLEAR_TIMER_MS      2000
#define CHRG_BAT_ICON_TIMER_MS    500
#define DISCHRG_BAT_ICON_TIMER_MS 10000
#define SHOW_SENSOR_DATA_TIMER_MS 1000
#define FINGER_OUT_TIMEOUT_MS     7000
/* #endregion */

/* #region  Buffer Size */
#define HEART_RATE_BUFF_SIZE 10
#define PULSE_BUFF_SIZE      50
#define CHART_BUFF_SIZE      240
/* #endregion */

/* #region  Utility */
#define GET_MIN(a, b) ((a) > (b) ? (b) : (a))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
/* #endregion */

/* #region  Battery Config */
#define ICON_WIDTH        70
#define ICON_HEIGHT       36
#define STATUS_HEIGHT_BAR ICON_HEIGHT
#define ICON_POS_X        (tft.width() - ICON_WIDTH)
#define MIN_USB_VOL       4.7
#define ADC_PIN           34
#define CONV_FACTOR       1.8
#define READS             20
/* #endregion */

/* #region  Other */
#define FINGER_ON_VALUE 7000
/* #endregion */

/* #region  Battery State */
typedef enum
{
    BAT_CHRG,
    BAT_DISCHRG,
    BAT_NONE,
} BAT_STATE_E;
/* #endregion */

typedef enum
{
    CHK_FINGER_STATE,
    FINGER_OUT_START,
    FINGER_OUT_KEEP,
} CHK_SLEEP_STATE_E;

#endif