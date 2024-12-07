#pragma once
#define SAVING_DIRECTORY      STORAGE_APP_DATA_PATH_PREFIX
#define SAVING_FILENAME       SAVING_DIRECTORY "/alarmatik.cfg"
#define CFG_VERSION           7
#define CLOCK_RFC_DATE_FORMAT "%s %.2d.%.2d.%.4d"
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
#define SCREEN_ID_TIMER_EXT 5
#define SCREEN_ID_ALARM_EXT 6
#define SCREEN_ID_BZZZT_SET 7

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
    bool ir_detection;
} App_Global_Data;

#define APP_TIMER_STATE_OFF   0
#define APP_TIMER_STATE_ON    1
#define APP_TIMER_STATE_PAUSE 2
#define APP_TIMER_STATE_BZZZ  3

typedef struct {
    int selected;
    int selectedExt;
    int32_t count;
    int32_t expected_count;
    int state;
    int tntMode1;
    int tntMode1_param;
    int tntMode2;
    int tntMode2_param;
} App_Timer_Data;

#define APP_ALARM_STATE_OFF      0
#define APP_ALARM_STATE_ON       1
#define APP_ALARM_STATE_SLEEP    2
#define APP_ALARM_STATE_BZZZ     3
#define APP_ALARM_PRIOR_INT      0
#define APP_ALARM_PRIOR_SYS      1
#define APP_ALARM_PRIOR_INDEPEND 2
#define BZZZT_FLAG_VIBRO         0x1
#define BZZZT_FLAG_SOUND         0x2
#define BZZZT_FLAG_BLINK         0x4

#define APP_CONFIG_LINES    3
#define APP_TIMER_EXT_LINES 2
#define APP_ALARM_EXT_LINES 3

typedef struct {
    int selected;
} App_Config_Data;

typedef struct {
    int selected;
    bool v;
    bool s;
    bool b;
} App_Bzzzt_Data;

typedef struct {
    int selected;
    int selectedExt;
    int sH;
    int sM;
    int sH_old;
    int sM_old;
    int state;
    bool system_state;
    int tntMode1;
    int tntMode1_param;
    int tntMode2;
    int tntMode2_param;
    int prior;
} App_Alarm_Data;

typedef struct {
    bool running;
    uint32_t start_timestamp;
    uint32_t stopped_seconds;
} App_Stopwatch_Data;

int AppInit();
int AppDeinit();
void Draw(Canvas* c, void* ctx);
int KeyProc(int type, int key);
void notification_beep_once();
void notification_BZZZT(int params);
void showScreen(int id);
void ApplyFont(Canvas* c);
char* getStr(int id);
void AppStopwatchKey(int key);
void AppTimeKey(int key);
void AppTimerReset();
void AppTimerKey(int key);
void AppAlarmKey(int key);
void AppAlarmExtKey(int key);
void AppTimerExtKey(int key);
void AppConfigKey(int key);
void AppBzzztLoadParam(int p);
void AppBzzztKey(int key);
void OnTimerTick();
void SetTNTmode2(int state);
void SetIR_rx(bool state);
void SetRing(bool state);
void LoadParams();
void SaveParams();
