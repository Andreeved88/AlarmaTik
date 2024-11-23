#include "alarmatik_icons.h"
#include "lang.h"
#pragma once
#define SAVING_DIRECTORY      STORAGE_APP_DATA_PATH_PREFIX
#define SAVING_FILENAME       SAVING_DIRECTORY "/alarmatik.cfg"
#define CLOCK_RFC_DATE_FORMAT "%.2d.%.2d.%.4d"
#define CLOCK_TIME_FORMAT     "%.2d:%.2d:%.2d"
#define TIMER_TIME_FORMAT     "%02ld:%02ld:%02ld"
#define MMSS_TIME_FORMAT      "%02ld:%02ld"
#define TIME_STR_SIZE         30

#define TIME_POS_X 10
#define TIME_POS_Y 47

#define SCREEN_ID_TIME      0
#define SCREEN_ID_ALARM     1
#define SCREEN_ID_TIMER     2
#define SCREEN_ID_STOPWATCH 3
#define SCREEN_ID_CONFIG    4

#define FONT_ID_INT 0
#define FONT_ID_EXT 1

typedef struct {
    int selectedScreen;
    int prevScreen;
    int brightness;
    int dspBrightnessBarFrames;
    int dspBrightnessBarDisplayFrames;
    bool led;
    bool ringing;
    bool irRxOn;
    bool irRecieved;
    bool show_time_only;
    int lang;
    int font;
    int tntMode;
    int ir_detection;
} App_Global_Data;

#define APP_TIMER_STATE_OFF   0
#define APP_TIMER_STATE_ON    1
#define APP_TIMER_STATE_PAUSE 2
#define APP_TIMER_STATE_BZZZ  3

typedef struct {
    int selected;
    int32_t count;
    int32_t expected_count;
    int state;
} App_Timer_Data;

#define APP_ALARM_STATE_OFF   0
#define APP_ALARM_STATE_ON    1
#define APP_ALARM_STATE_SLEEP 2
#define APP_ALARM_STATE_BZZZ  3

typedef struct {
    int selected;
} App_Config_Data;

typedef struct {
    int selected;
    int sH;
    int sM;
    int sH_old;
    int sM_old;
    int state;
    bool system_state;
} App_Alarm_Data;

typedef struct {
    bool running;
    uint32_t start_timestamp;
    uint32_t stopped_seconds;
} App_Stopwatch_Data;

static const NotificationSequence sequence_beep = {
    &message_blue_255,
    &message_note_c8,
    &message_delay_100,
    &message_sound_off,
    NULL,
};

static const NotificationSequence sequence_timeup = {
    &message_force_display_brightness_setting_1f,
    &message_display_backlight_on,
    &message_vibro_on,
    &message_note_c8,
    &message_delay_50,
    &message_sound_off,
    &message_delay_50,
    &message_delay_25,
    &message_note_c8,
    &message_delay_50,
    &message_sound_off,
    &message_delay_50,
    &message_delay_25,
    &message_note_c8,
    &message_delay_50,
    &message_sound_off,
    &message_delay_50,
    &message_delay_25,
    &message_note_c8,
    &message_delay_50,
    &message_sound_off,
    &message_delay_50,
    &message_delay_25,
    &message_vibro_off,
    &message_display_backlight_off,
    &message_delay_500,
    NULL,
};

int AppInit();
int AppDeinit();
void Draw(Canvas* c, void* ctx);
int KeyProc(int type, int key);
void notification_beep_once();
void notification_off();
void notification_timeup();
void showScreen(int id);
void ApplyFont(Canvas* c);
char* getStr(int id);
void AppTimeKeyUp();
void AppTimeKeyDown();
void AppStopwatchToggle();
void AppStopwatchReset();
void AppTimerKeyUp();
void AppTimerKeyDown();
void AppTimerKeyOk();
void AppTimerKeyBack();
void AppTimerKeyLeft();
void AppTimerKeyRight();
void AppTimerReset();
void AppAlarmKeyUp();
void AppAlarmKeyDown();
void AppAlarmKeyOk();
void AppAlarmKeyBack();
void AppConfigKeyUp();
void AppConfigKeyDown();
void AppConfigKeyLeft();
void AppConfigKeyRight();
void OnTimerTick();
void SetTNT(int mode);
void SetIrBlink(bool state);
void SetIR_rx(bool state);
void SetRing(bool state);
void LoadParams();
void SaveParams();
