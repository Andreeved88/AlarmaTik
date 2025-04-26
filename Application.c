#include "F0App.h"
#include "alarmatik_icons.h"
#include "F0BigData.h"
extern F0App* FApp;

char* CaptionsRus[] = {
    "[0]",   "Выкл",      "Вкл",     "Сброс",   "Пуск",      "Стоп",       "Lang / Язык", "Рус",
    "Eng",   "Пакет рун", "База",    "Система", "БЗЗЗТ",     "C3+ B2-",    "ИК приёмник", "Буд",
    "Хрон",  "Тмр",       "Спать",   "Режим",   "Настройка", "Пнд",        "Втр",         "Срд",
    "Чтр",   "Птн",       "Сбт",     "Вск",     "Вибро", "Мигалка",   "Пикалка", "Зап"};

char* CaptionsEng[] = {"[0]",         "Off",     "On",          "Reset", "Start",    "Stop",
                       "Lang / Язык", "Рус",     "Eng",         "Font",  "Internal", "System",
                       "BZZZT",       "C3+ B2-", "IR reciever", "Alarm", "Chron",    "Timer",
                       "Sleep",       "Action",  "Set",         "Mon",   "Tue",      "Wed",
                       "Thr",         "Fri",     "Sat",         "Sun",   "Vibro",       "Blink", "Sounds",   "Write"};

App_Global_Data AppGlobal = {
    .selectedScreen = SCREEN_ID_TIME,
    .prevScreen = SCREEN_ID_TIME,
    .brightness = 10,
    .dspBrightnessBarDisplayFrames = 2,
    .dspBrightnessBarFrames = 0,
    .led = 0,
    .ringing = 0,
    .irRxOn = 0,
    .irRecieved = 0,
    .show_time_only = 0,
    .lang = 0,
    .font = 0,
    .ir_detection = 0};

App_Alarm_Data AppAlarm = {
    .selected = 0,
    .selectedExt = 0,
    .sH = 6,
    .sH_old = 0,
    .sM = 30,
    .sM_old = 0,
    .state = APP_ALARM_STATE_OFF,
    .system_state = 0,
    .tntMode1 = 1,
    .tntMode1_param = 7,
    .tntMode2 = 0,
    .tntMode2_param = 0,
    .prior = 0};

App_Timer_Data AppTimer = {
    .selected = 1,
    .selectedExt = 0,
    .count = 300,
    .expected_count = 300,
    .state = APP_TIMER_STATE_OFF,
    .tntMode1 = 1,
    .tntMode1_param = 7,
    .tntMode2 = 0,
    .tntMode2_param = 0};

App_Config_Data AppConfig = {.selected = 0};
App_Bzzzt_Data AppBzzzt = {.selected = 0, .v = 1, .s = 1, .b = 1};
App_Stopwatch_Data AppStopwatch =
    {.running = 0, .start_tick = 0, .stopped_tick = 0, .slot_id = 0, .curr_slot = 0};
char time_string[TIME_STR_SIZE];
char date_string[TIME_STR_SIZE];
char timer_string[TIME_STR_SIZE];
char timer_string_trim[TIME_STR_SIZE];
char stopwatch_string[TIME_STR_SIZE];
char alarm_string[TIME_STR_SIZE];
DateTime curr_dt;
InfraredWorker* worker;

static const NotificationSequence sequence_beep = {
    &message_blue_255,
    &message_note_c8,
    &message_delay_50,
    &message_sound_off,
    NULL,
};
static NotificationSequence sequence_bzzzt = {
    &message_force_display_brightness_setting_1f,
    &message_display_backlight_on,
    &message_vibro_on,
    &message_note_c5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_100,
    &message_note_c6,
    &message_delay_50,
    &message_sound_off,
    &message_delay_100,
    &message_note_c5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_100,
    &message_note_c6,
    &message_delay_50,
    &message_sound_off,
    &message_vibro_off,
    &message_display_backlight_off,
    &message_delay_250,
    NULL,
};

void notification_beep_once() {
    notification_message(FApp->Notificator, &sequence_beep);
}
void notification_BZZZT(int params) {
    bool v = params & BZZZT_FLAG_VIBRO;
    bool b = params & BZZZT_FLAG_BLINK;
    bool s = params & BZZZT_FLAG_SOUND;

    if(!b) {
        sequence_bzzzt[0] = &message_delay_10;
        sequence_bzzzt[1] = &message_delay_10;
        sequence_bzzzt[19] = &message_delay_10;
    } else {
        sequence_bzzzt[0] = &message_force_display_brightness_setting_1f;
        sequence_bzzzt[1] = &message_display_backlight_on;
        sequence_bzzzt[19] = &message_display_backlight_off;
    }    
    
    if(!v){
        sequence_bzzzt[2] = &message_delay_10;
        sequence_bzzzt[18] = &message_delay_10;
    }
    else {
        sequence_bzzzt[2] = &message_vibro_on;
        sequence_bzzzt[18] = &message_vibro_off;
    }

    if(!s) {
        sequence_bzzzt[3] = &message_delay_500;
        if(v) sequence_bzzzt[4] = &message_vibro_off;
        else sequence_bzzzt[4] = &message_delay_10;
        if(b) sequence_bzzzt[5] = &message_display_backlight_off;
        else sequence_bzzzt[5] = &message_delay_10;
        sequence_bzzzt[6] =  &message_delay_250;
        sequence_bzzzt[7] = NULL;
    } else {
        sequence_bzzzt[3] = &message_note_c5;
        sequence_bzzzt[4] = &message_delay_50;
        sequence_bzzzt[5] = &message_sound_off;
        sequence_bzzzt[6] = &message_delay_100;
        sequence_bzzzt[7] = &message_note_c6;
    }
    notification_message(FApp->Notificator, &sequence_bzzzt);
}

void showScreen(int id) {
    AppGlobal.prevScreen = AppGlobal.selectedScreen;
    AppGlobal.selectedScreen = id;
}
void ApplyFont(Canvas* c) {
    if(AppGlobal.font == FONT_ID_INT) canvas_set_custom_u8g2_font(c, quadro7);
    if(AppGlobal.font == FONT_ID_EXT) canvas_set_font(c, FontSecondary);
}
char* getStr(int id) {
    if(AppGlobal.lang == 0)
        return CaptionsEng[id];
    else
        return CaptionsRus[id];
}

static void ir_received_callback(void* ctx, InfraredWorkerSignal* signal) {
    UNUSED(ctx);
    UNUSED(signal);
    AppGlobal.irRecieved = 1;
}
void SetIR_rx(bool state) {
    if(AppGlobal.ir_detection == 0) return;
    AppGlobal.irRecieved = 0;
    if(AppGlobal.irRxOn == state) return;
    AppGlobal.irRxOn = state;
    if(state) {
        AppGlobal.irRxOn = 1;
        infrared_worker_rx_start(worker);
    } else {
        AppGlobal.irRxOn = 0;
        infrared_worker_rx_stop(worker);
    }
}

int AppInit() {
    worker = infrared_worker_alloc();
    infrared_worker_rx_enable_signal_decoding(worker, false);
    infrared_worker_rx_enable_blink_on_receiving(worker, true);
    infrared_worker_rx_set_received_signal_callback(worker, ir_received_callback, 0);
    furi_hal_gpio_init(&gpio_ext_pc3, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_init(&gpio_ext_pb2, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(&gpio_ext_pc3, 0);
    furi_hal_gpio_write(&gpio_ext_pb2, 1);
    LoadParams();
    UpdateView();
    SetScreenBacklightBrightness(AppGlobal.brightness);
    return 0;
}
int AppDeinit() {
    ResetLED();
    SetScreenBacklightBrightness((int)(FApp->SystemScreenBrightness * 100));
    SetIR_rx(0);
    furi_hal_gpio_init(&gpio_ext_pc3, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_init(&gpio_ext_pb2, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(&gpio_ext_pc3, 0);
    furi_hal_gpio_write(&gpio_ext_pb2, 0);
    SaveParams();
    infrared_worker_free(worker);
    return 0;
}

void elements_progress_bar_vertical(Canvas* canvas, int x, int y, int height, float progress) {
    furi_assert(canvas);
    furi_assert((progress >= 0) && (progress <= 1.0));
    uint8_t width = 7;
    uint8_t progress_length = roundf((1.f - progress) * (height - 2));
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_box(canvas, x + 1, y + 1, width - 2, height - 2);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, x + 1, y + 1, width - 2, progress_length);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_rframe(canvas, x, y, width, height, 2);
}

void Draw(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    furi_hal_rtc_get_datetime(&curr_dt);
    snprintf(
        time_string,
        TIME_STR_SIZE,
        CLOCK_TIME_FORMAT,
        curr_dt.hour,
        curr_dt.minute,
        curr_dt.second);

    snprintf(
        date_string,
        TIME_STR_SIZE,
        CLOCK_RFC_DATE_FORMAT,
        getStr(curr_dt.weekday + 20),
        curr_dt.day,
        curr_dt.month,
        curr_dt.year);

    snprintf(
        timer_string,
        TIME_STR_SIZE,
        TIMER_TIME_FORMAT,
        (AppTimer.count % (60 * 60 * 100)) / (60 * 60),
        (AppTimer.count % (60 * 60)) / 60,
        AppTimer.count % 60);
    snprintf(alarm_string, TIME_STR_SIZE, "%.2d:%.2d", AppAlarm.sH, AppAlarm.sM);

    if(AppGlobal.selectedScreen == SCREEN_ID_TIME) { //ЧАСЫ

        if(AppGlobal.dspBrightnessBarFrames) {
            elements_progress_bar_vertical(
                canvas, 121, 0, 64, (float)(AppGlobal.brightness / 100.f));
            return;
        }
        if(!AppGlobal.show_time_only) {
            canvas_set_custom_u8g2_font(canvas, TechnoDigits15);
            canvas_draw_str(canvas, TIME_POS_X, TIME_POS_Y, time_string);
            ApplyFont(canvas);
            elements_button_left(canvas, getStr(STR_TIME_ALARM));
            elements_button_center(canvas, getStr(STR_TIME_STOPWATCH));
            elements_button_right(canvas, getStr(STR_TIME_TIMER));
            if(AppGlobal.ir_detection) canvas_draw_icon(canvas, 0, 0, &I_IR_On);
            canvas_draw_str_aligned(canvas, 128, 0, AlignRight, AlignTop, date_string);
            if(AppAlarm.state)
                canvas_draw_str_aligned(canvas, 0, 44, AlignLeft, AlignTop, alarm_string);
            if(AppTimer.state == APP_TIMER_STATE_ON) {
                if(AppTimer.count > 3599)
                    canvas_draw_str_aligned(canvas, 128, 44, AlignRight, AlignTop, timer_string);
                else {
                    snprintf(
                        timer_string_trim,
                        TIME_STR_SIZE,
                        MMSS_TIME_FORMAT,
                        (AppTimer.count % (60 * 60)) / 60,
                        AppTimer.count % 60);
                    canvas_draw_str_aligned(
                        canvas, 128, 44, AlignRight, AlignTop, timer_string_trim);
                }
            }
        } else {
            canvas_set_custom_u8g2_font(canvas, bigidig);
            char out[5];
            snprintf(out, 5, "%.2d", curr_dt.hour);
            canvas_draw_str_aligned(canvas, 0, 0, AlignLeft, AlignTop, out);
            snprintf(out, 5, "%.2d", curr_dt.minute);
            canvas_draw_str_aligned(canvas, 128, 73, AlignRight, AlignBottom, out);
        }
    }
    if(AppGlobal.selectedScreen == SCREEN_ID_STOPWATCH) { //СЕКУНДОМЕР

        uint32_t elapsed_msecs = AppStopwatch.stopped_tick;
        if(AppStopwatch.running) elapsed_msecs = furi_get_tick() - AppStopwatch.start_tick;
        int32_t h = elapsed_msecs / 3600000, m = (elapsed_msecs / 60000) % 60,
                s = (elapsed_msecs / 1000) % 60, ms = elapsed_msecs % 1000;
        snprintf(stopwatch_string, 21, "%.2ld:%.2ld:%.2ld", h, m, s);
        canvas_set_custom_u8g2_font(canvas, TechnoDigits15);
        canvas_draw_str(canvas, 0, TIME_POS_Y, stopwatch_string);
        ApplyFont(canvas);
        canvas_draw_str_aligned(canvas, 0, 0, AlignLeft, AlignTop, time_string);
        canvas_draw_str_aligned(canvas, 128, 0, AlignRight, AlignTop, date_string);
        elements_button_left(canvas, getStr(STR_GLOBAL_RESET));
        if(AppStopwatch.running) {
            elements_button_center(canvas, getStr(STR_GLOBAL_STOP));
            elements_button_right(canvas, getStr(STR_STOPWATCH_WRITE));
        } else {
            elements_button_center(canvas, getStr(STR_GLOBAL_START));
            if(AppStopwatch.stopped_tick) {
                char msec[7];
                snprintf(msec, TIME_STR_SIZE, "%.2ld", ms);
                canvas_draw_str_aligned(canvas, 128, TIME_POS_Y - 15, AlignRight, AlignTop, msec);
            }
        }
        if(AppStopwatch.curr_slot) {
            char str_res[40];
            int32_t slot_ms = AppStopwatch.slot[AppStopwatch.curr_slot];
            h = slot_ms / 3600000;
            m = (slot_ms / 60000) % 60;
            s = (slot_ms / 1000) % 60;
            ms = slot_ms % 1000;
            snprintf(
                str_res, 40, "#%.d: %.2ld:%.2ld:%.2ld.%2ld", AppStopwatch.curr_slot, h, m, s, ms);
            canvas_draw_str_aligned(canvas, 0, TIME_POS_Y - 5, AlignLeft, AlignTop, str_res);
        }
    }
    if(AppGlobal.selectedScreen == SCREEN_ID_TIMER) { //ТАЙМЕР
        canvas_set_custom_u8g2_font(canvas, TechnoDigits15);
        canvas_draw_str(canvas, TIME_POS_X, TIME_POS_Y, timer_string);
        if(AppTimer.count && AppTimer.state != APP_TIMER_STATE_OFF)
            elements_progress_bar(
                canvas, 0, 10, 128, (1.0 * AppTimer.count / AppTimer.expected_count));
        ApplyFont(canvas);
        canvas_draw_str_aligned(canvas, 0, 0, AlignLeft, AlignTop, time_string);
        canvas_draw_str_aligned(canvas, 128, 0, AlignRight, AlignTop, date_string);
        if(AppTimer.state == APP_TIMER_STATE_OFF) {
            int shiftX = 10 + AppTimer.selected * 40;
            canvas_draw_rframe(canvas, shiftX - 1, TIME_POS_Y - 25, 32, 19, 2);
            canvas_draw_rframe(canvas, shiftX - 2, TIME_POS_Y - 26, 34, 21, 2);
            elements_button_center(canvas, getStr(STR_GLOBAL_START));
        }

        if(AppTimer.state == APP_TIMER_STATE_ON) { //запущен
            elements_button_center(canvas, getStr(STR_GLOBAL_STOP));
        }

        if(AppTimer.state == APP_TIMER_STATE_PAUSE) {
            elements_button_left(canvas, getStr(STR_GLOBAL_RESET));
            elements_button_center(canvas, getStr(STR_GLOBAL_START));
        }

        if(AppTimer.state == APP_TIMER_STATE_BZZZ) {
            elements_button_center(canvas, getStr(STR_GLOBAL_OFF));
        }
        return;
    }
    if(AppGlobal.selectedScreen == SCREEN_ID_ALARM) { //БУДИЛЬНИК
        int shiftX = TIME_POS_X + 20;
        if(AppAlarm.selected == 1) shiftX = TIME_POS_X + 60;
        canvas_set_custom_u8g2_font(canvas, TechnoDigits15);
        canvas_draw_str(canvas, TIME_POS_X + 20, TIME_POS_Y, alarm_string);

        canvas_draw_rframe(canvas, shiftX - 1, TIME_POS_Y - 25, 32, 19, 2);
        canvas_draw_rframe(canvas, shiftX - 2, TIME_POS_Y - 26, 34, 21, 2);

        ApplyFont(canvas);
        canvas_draw_str_aligned(canvas, 0, 0, AlignLeft, AlignTop, time_string);
        canvas_draw_str_aligned(canvas, 128, 0, AlignRight, AlignTop, date_string);
        if(AppAlarm.state) {
            if(AppAlarm.state == APP_ALARM_STATE_BZZZ)
                elements_button_center(canvas, getStr(STR_ALARM_SLEEP));
            else
                elements_button_center(canvas, getStr(STR_GLOBAL_OFF));
        } else
            elements_button_center(canvas, getStr(STR_GLOBAL_ON));
        return;
    }
    if(AppGlobal.selectedScreen == SCREEN_ID_CONFIG) { //НАСТРОЙКИ
        int x = 3, y = 3;

        canvas_draw_rframe(canvas, 0, (AppConfig.selected * 13), 128, 13, 2);

        ApplyFont(canvas);
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_CONF_LANG));
        if(AppGlobal.lang == 0)
            canvas_draw_str_aligned(canvas, 125, y, AlignRight, AlignTop, getStr(STR_LANG_ENG));
        else
            canvas_draw_str_aligned(canvas, 125, y, AlignRight, AlignTop, getStr(STR_LANG_RUS));
        y += 13;
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_CONF_FONT));
        if(AppGlobal.font == 0)
            canvas_draw_str_aligned(
                canvas, 125, y, AlignRight, AlignTop, getStr(STR_CONF_FONT_INT));
        else
            canvas_draw_str_aligned(
                canvas, 125, y, AlignRight, AlignTop, getStr(STR_CONF_FONT_EXT));
        y += 13;
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_CONF_IRMOTDET));

        if(AppGlobal.ir_detection == 0)
            canvas_draw_str_aligned(canvas, 125, y, AlignRight, AlignTop, getStr(STR_GLOBAL_OFF));
        else
            canvas_draw_str_aligned(canvas, 125, y, AlignRight, AlignTop, getStr(STR_GLOBAL_ON));
    }
    if(AppGlobal.selectedScreen == SCREEN_ID_ALARM_EXT) {
        int x = 0, y = 0, w = 128;
        canvas_draw_rframe(canvas, x, (AppAlarm.selectedExt * 13) + y + 1, w, 13, 2);
        ApplyFont(canvas);
        x += 2;
        y += 4;
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_CONF_TNT_BZZZT));
        canvas_draw_str_aligned(canvas, x, y + 13, AlignLeft, AlignTop, getStr(STR_CONF_TNT_PC3));
        int str_id1 = 0, str_id2 = 0;
        if(AppAlarm.tntMode1 == 0)
            str_id1 = STR_GLOBAL_OFF;
        else
            str_id1 = STR_GLOBAL_ON;
        if(AppAlarm.tntMode2 == 0)
            str_id2 = STR_GLOBAL_OFF;
        else
            str_id2 = STR_GLOBAL_ON;
        canvas_draw_str_aligned(canvas, x + w - 5, y, AlignRight, AlignTop, getStr(str_id1));
        canvas_draw_str_aligned(canvas, x + w - 5, y + 13, AlignRight, AlignTop, getStr(str_id2));
        if(AppAlarm.selectedExt == 0) elements_button_center(canvas, getStr(STR_TNT_SETTINGS));
    }
    if(AppGlobal.selectedScreen == SCREEN_ID_TIMER_EXT) {
        int x = 0, y = 0, w = 128;
        canvas_draw_rframe(canvas, x, (AppTimer.selectedExt * 13) + y + 1, w, 13, 2);
        ApplyFont(canvas);
        x += 2;
        y += 4;
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_CONF_TNT_BZZZT));
        canvas_draw_str_aligned(canvas, x, y + 13, AlignLeft, AlignTop, getStr(STR_CONF_TNT_PC3));
        int str_id1 = 0, str_id2 = 0;
        if(AppTimer.tntMode1 == 0)
            str_id1 = STR_GLOBAL_OFF;
        else
            str_id1 = STR_GLOBAL_ON;
        if(AppTimer.tntMode2 == 0)
            str_id2 = STR_GLOBAL_OFF;
        else
            str_id2 = STR_GLOBAL_ON;
        canvas_draw_str_aligned(canvas, x + w - 5, y, AlignRight, AlignTop, getStr(str_id1));
        canvas_draw_str_aligned(canvas, x + w - 5, y + 13, AlignRight, AlignTop, getStr(str_id2));
        if(AppTimer.selectedExt == 0) elements_button_center(canvas, getStr(STR_TNT_SETTINGS));
    }
    if(AppGlobal.selectedScreen == SCREEN_ID_BZZZT_SET) {
        int x = 0, y = 0, w = 128;
        canvas_draw_rframe(canvas, x, (AppBzzzt.selected * 13) + y + 1, w, 13, 2);
        ApplyFont(canvas);
        x += 2;
        y += 4;
        int str_id1 = STR_GLOBAL_OFF, str_id2 = STR_GLOBAL_OFF, str_id3 = STR_GLOBAL_OFF;
        if(AppBzzzt.v == 1) str_id1 = STR_GLOBAL_ON;
        if(AppBzzzt.b == 1) str_id2 = STR_GLOBAL_ON;
        if(AppBzzzt.s == 1) str_id3 = STR_GLOBAL_ON;
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_BZZZT_VIBRO));
        canvas_draw_str_aligned(canvas, x + w - 5, y, AlignRight, AlignTop, getStr(str_id1));
        y += 13;
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_BZZZT_BLINK));
        canvas_draw_str_aligned(canvas, x + w - 5, y, AlignRight, AlignTop, getStr(str_id2));
        y += 13;
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_BZZZT_SOUND));
        canvas_draw_str_aligned(canvas, x + w - 5, y, AlignRight, AlignTop, getStr(str_id3));
    }
}

int KeyProc(int type, int key) {
    if(type == InputTypeMAX) {
        if(key == 255) OnTimerTick();
        return 0;
    }
    int ss = AppGlobal.selectedScreen;
    if(type == InputTypeLong) {
        if(key == InputKeyUp) {
            if(ss == SCREEN_ID_STOPWATCH) {
                if(AppStopwatch.slot_id)
                    AppStopwatch.curr_slot = 1;
                else
                    AppStopwatch.curr_slot = 0;
            }
            return 0;
        }
        if(key == InputKeyDown) {
            if(ss == SCREEN_ID_STOPWATCH) AppStopwatch.curr_slot = AppStopwatch.slot_id;
            return 0;
        }

        if(key == InputKeyBack) {
            if(ss == SCREEN_ID_TIME) return 255; //exit
            return 0;
        }
        if(key == InputKeyOk) {
            if(ss == SCREEN_ID_TIME) {
                showScreen(SCREEN_ID_CONFIG);
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                if(AppTimer.state != APP_TIMER_STATE_BZZZ) showScreen(SCREEN_ID_TIMER_EXT);
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                if(AppAlarm.state != APP_ALARM_STATE_BZZZ) showScreen(SCREEN_ID_ALARM_EXT);
                return 0;
            }
            return 0;
        }
    }
    if(type == InputTypeRepeat) {
        if(key == InputKeyUp) {
            if(ss == SCREEN_ID_TIME) {
                AppTimeKey(InputKeyUp);
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKey(InputKeyUp);
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarmKey(InputKeyUp);
                return 0;
            }
            return 0;
        }
        if(key == InputKeyDown) {
            if(ss == SCREEN_ID_TIME) {
                AppTimeKey(InputKeyDown);
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKey(InputKeyDown);
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarmKey(InputKeyDown);
                return 0;
            }
            return 0;
        }
        return 0;
    }
    if(type == InputTypeShort) {
        if(ss == SCREEN_ID_TIME) {
            AppTimeKey(key);
            return 0;
        }
        if(ss == SCREEN_ID_STOPWATCH) {
            AppStopwatchKey(key);
            return 0;
        }
        if(ss == SCREEN_ID_ALARM) {
            AppAlarmKey(key);
            return 0;
        }
        if(ss == SCREEN_ID_ALARM_EXT) {
            AppAlarmExtKey(key);
            return 0;
        }
        if(ss == SCREEN_ID_TIMER) {
            AppTimerKey(key);
            return 0;
        }
        if(ss == SCREEN_ID_TIMER_EXT) {
            AppTimerExtKey(key);
            return 0;
        }
        if(ss == SCREEN_ID_BZZZT_SET) {
            AppBzzztKey(key);
            return 0;
        }
        if(ss == SCREEN_ID_CONFIG) {
            AppConfigKey(key);
            return 0;
        }
        return 0;
    }
    return 0;
}

void AppStopwatchKey(int key) {
    if(key == InputKeyLeft) {
        if(AppStopwatch.start_tick) {
            AppStopwatch.running = 0;
            AppStopwatch.start_tick = 0;
            AppStopwatch.stopped_tick = 0;
            AppStopwatch.slot_id = 0;
            AppStopwatch.slot[0] = 0;
            AppStopwatch.curr_slot = 0;
        }
        return;
    }
    if(key == InputKeyOk) {
        uint32_t curr_tick = furi_get_tick();
        if(AppStopwatch.running)
            AppStopwatch.stopped_tick = curr_tick - AppStopwatch.start_tick;
        else {
            if(!AppStopwatch.start_tick) {
                AppStopwatch.start_tick = curr_tick;
                AppStopwatch.slot[0] = curr_tick;
            } else
                AppStopwatch.start_tick = curr_tick - AppStopwatch.stopped_tick;
        }
        AppStopwatch.running = !AppStopwatch.running;
        notification_beep_once();
        return;
    }
    if(key == InputKeyBack) {
        showScreen(SCREEN_ID_TIME);
        return;
    }
    if(key == InputKeyUp) {
        if(AppStopwatch.curr_slot > 1) AppStopwatch.curr_slot--;
        return;
    }
    if(key == InputKeyDown) {
        if(AppStopwatch.curr_slot < AppStopwatch.slot_id) AppStopwatch.curr_slot++;
        return;
    }
    if(key == InputKeyRight) {
        if(!AppStopwatch.running) return;
        if(AppStopwatch.slot_id < 146) {
            if(AppStopwatch.curr_slot == AppStopwatch.slot_id) AppStopwatch.curr_slot++;
            AppStopwatch.slot_id++;
            AppStopwatch.slot[AppStopwatch.slot_id] = furi_get_tick() - AppStopwatch.start_tick;
        }
        notification_beep_once();
        return;
    }
}

void AppTimeKey(int key) {
    if(key == InputKeyUp) {
        AppGlobal.dspBrightnessBarFrames = AppGlobal.dspBrightnessBarDisplayFrames;
        if(AppGlobal.brightness < 100) {
            if(AppGlobal.led) {
                AppGlobal.led = 0;
                ResetLED();
            }
            AppGlobal.brightness += 10;
        }
        if(AppGlobal.brightness > 100) AppGlobal.brightness = 100;
        SetScreenBacklightBrightness(AppGlobal.brightness);
        return;
    }
    if(key == InputKeyDown) {
        AppGlobal.dspBrightnessBarFrames = AppGlobal.dspBrightnessBarDisplayFrames;
        if(AppGlobal.brightness > 0) {
            AppGlobal.brightness -= 10;
            if(AppGlobal.brightness == 0) {
                AppGlobal.led = true;
                SetLED(255, 0, 0, 0.1);
            }
        } else if(AppGlobal.brightness == 0) {
            AppGlobal.led = !AppGlobal.led;
            if(AppGlobal.led)
                SetLED(255, 0, 0, 0.1);
            else
                ResetLED();
        }
        SetScreenBacklightBrightness(AppGlobal.brightness);
        return;
    }
    if(key == InputKeyLeft) {
        showScreen(SCREEN_ID_ALARM);
        return;
    }
    if(key == InputKeyRight) {
        showScreen(SCREEN_ID_TIMER);
        return;
    }
    if(key == InputKeyOk) {
        showScreen(SCREEN_ID_STOPWATCH);
        return;
    }
    if(key == InputKeyBack) {
        AppGlobal.show_time_only = !AppGlobal.show_time_only;
        return;
    }
}

void AppTimerKey(int key) {
    if(key == InputKeyUp) {
        if(AppTimer.state != APP_TIMER_STATE_OFF) return;
        if(AppTimer.selected == 0) AppTimer.count += 60 * 60;
        if(AppTimer.selected == 1) AppTimer.count += 60;
        if(AppTimer.selected == 2) AppTimer.count += 1;
        if(AppTimer.count > 100 * 60 * 60 - 1) AppTimer.count = 100 * 60 * 60 - 1;
        AppTimer.expected_count = AppTimer.count;
        return;
    }
    if(key == InputKeyDown) {
        if(AppTimer.state != APP_TIMER_STATE_OFF) return;
        if(AppTimer.selected == 0) AppTimer.count -= 60 * 60;
        if(AppTimer.selected == 1) AppTimer.count -= 60;
        if(AppTimer.selected == 2) AppTimer.count -= 1;

        if(AppTimer.count < 0) AppTimer.count = 0;
        AppTimer.expected_count = AppTimer.count;
        return;
    }
    if(key == InputKeyLeft) {
        if(AppTimer.state == APP_TIMER_STATE_OFF) {
            if(!AppTimer.selected) AppTimer.selected = 3;
            AppTimer.selected--;
            return;
        }

        if(AppTimer.state == APP_TIMER_STATE_PAUSE) {
            AppTimerReset();
            return;
        }
        return;
    }
    if(key == InputKeyRight) {
        if(AppTimer.state == APP_TIMER_STATE_OFF) {
            AppTimer.selected++;
            if(AppTimer.selected > 2) AppTimer.selected = 0;
        }
        return;
    }
    if(key == InputKeyOk) {
        int ats = AppTimer.state;
        if(ats == APP_TIMER_STATE_OFF || ats == APP_TIMER_STATE_PAUSE) { //ON
            if(!AppTimer.count) return;
            AppTimer.state = APP_TIMER_STATE_ON;
            notification_beep_once();
            return;
        }

        if(ats == APP_TIMER_STATE_ON) { //PAUSE
            AppTimer.state = APP_TIMER_STATE_PAUSE;
            return;
        }

        if(AppTimer.state == APP_TIMER_STATE_BZZZ) { //RESET
            AppTimerReset();
            if(AppTimer.tntMode1 == 1) SetRing(0);
            if(AppTimer.tntMode2 == 1) SetTNTmode2(0);
            return;
        }
        return;
    }
    if(key == InputKeyBack) {
        if(AppTimer.state == APP_TIMER_STATE_BZZZ) {
            AppTimerReset();
            if(AppTimer.tntMode1 == 1) SetRing(0);
            if(AppTimer.tntMode2 == 1) SetTNTmode2(0);
        }
        AppGlobal.selectedScreen = SCREEN_ID_TIME;
        return;
    }
}

void AppTimerReset() {
    AppTimer.state = APP_TIMER_STATE_OFF;
    AppTimer.count = AppTimer.expected_count;
}

void AppAlarmKey(int key) {
    if(key == InputKeyUp) {
        switch(AppAlarm.selected) {
        case 0:
            ++AppAlarm.sH;
            if(AppAlarm.sH == 24) AppAlarm.sH = 0;
            break;
        case 1:
            ++AppAlarm.sM;
            if(AppAlarm.sM == 60) AppAlarm.sM = 0;
            break;
        }
        AppAlarm.sH_old = AppAlarm.sH;
        AppAlarm.sM_old = AppAlarm.sM;
        return;
    }
    if(key == InputKeyDown) {
        switch(AppAlarm.selected) {
        case 0:
            if(AppAlarm.sH == 0)
                AppAlarm.sH = 23;
            else
                AppAlarm.sH--;
            break;
        case 1:
            if(AppAlarm.sM == 0)
                AppAlarm.sM = 59;
            else
                AppAlarm.sM--;
            break;
        }
        AppAlarm.sH_old = AppAlarm.sH;
        AppAlarm.sM_old = AppAlarm.sM;
        return;
    }
    if(key == InputKeyLeft || key == InputKeyRight) {
        AppAlarm.selected = !AppAlarm.selected;
        return;
    }
    if(key == InputKeyOk) {
        if(AppAlarm.state == APP_ALARM_STATE_OFF) {
            AppAlarm.state = APP_ALARM_STATE_ON;
            return;
        };

        if(AppAlarm.state == APP_ALARM_STATE_ON || AppAlarm.state == APP_ALARM_STATE_SLEEP) {
            AppAlarm.state = APP_ALARM_STATE_OFF;
            AppAlarm.sH = AppAlarm.sH_old;
            AppAlarm.sM = AppAlarm.sM_old;
            return;
        };

        if(AppAlarm.state == APP_ALARM_STATE_BZZZ) {
            AppAlarm.state = APP_ALARM_STATE_SLEEP;
            AppAlarm.sM += 5;
            if(AppAlarm.sM > 59) {
                AppAlarm.sM = AppAlarm.sM - 60;
                AppAlarm.sH++;
                if(AppAlarm.sH > 23) AppAlarm.sH = 0;
                if(AppAlarm.tntMode1 == 1) SetRing(0);
                if(AppAlarm.tntMode2 == 1) SetTNTmode2(0);
                showScreen(SCREEN_ID_TIME);
                return;
            };
        }
        return;
    }
    if(key == InputKeyBack) {
        if(AppAlarm.state == APP_ALARM_STATE_BZZZ) AppAlarm.state = APP_ALARM_STATE_SLEEP;
        if(AppAlarm.tntMode1 == 1) SetRing(0);
        if(AppAlarm.tntMode2 == 1) SetTNTmode2(0);
        showScreen(SCREEN_ID_TIME);
        AppAlarm.sH = AppAlarm.sH_old;
        AppAlarm.sM = AppAlarm.sM_old;
        return;
    }
}

void AppAlarmExtKey(int key) {
    if(key == InputKeyBack) {
        showScreen(SCREEN_ID_ALARM);
        return;
    }
    if(key == InputKeyUp) {
        if(!AppAlarm.selectedExt)
            AppAlarm.selectedExt = APP_ALARM_EXT_LINES - 1;
        else
            AppAlarm.selectedExt--;
        return;
    }
    if(key == InputKeyDown) {
        AppAlarm.selectedExt++;
        if(AppAlarm.selectedExt == APP_ALARM_EXT_LINES) AppAlarm.selectedExt = 0;
        return;
    }
    if(key == InputKeyLeft) {
        if(AppAlarm.selectedExt == 0) AppAlarm.tntMode1 = 0; //bzzzt
        if(AppAlarm.selectedExt == 1) AppAlarm.tntMode2 = 0; //pin7
        if(AppAlarm.selectedExt == 2) {
            if(AppAlarm.prior > 0) AppAlarm.prior--;
        }
        return;
    }
    if(key == InputKeyRight) {
        if(AppAlarm.selectedExt == 0) AppAlarm.tntMode1 = 1; //bzzzt
        if(AppAlarm.selectedExt == 1) AppAlarm.tntMode2 = 1; //pin7
        if(AppAlarm.selectedExt == 2) {
            if(AppAlarm.prior < 2) AppAlarm.prior++;
        }
        return;
    }
    if(key == InputKeyOk) {
        if(AppAlarm.selectedExt == 0) {
            AppBzzztLoadParam(AppAlarm.tntMode1_param);
            showScreen(SCREEN_ID_BZZZT_SET);
        }
    }

    if(key == InputKeyBack) {
        showScreen(SCREEN_ID_ALARM_EXT);
        return;
    }

    return;
}

void AppTimerExtKey(int key) {
    if(key == InputKeyBack) {
        showScreen(SCREEN_ID_TIMER);
        return;
    }

    if(key == InputKeyUp) {
        if(!AppTimer.selectedExt)
            AppTimer.selectedExt = APP_TIMER_EXT_LINES - 1;
        else
            AppTimer.selectedExt--;
        return;
    }
    if(key == InputKeyDown) {
        AppTimer.selectedExt++;
        if(AppTimer.selectedExt == APP_TIMER_EXT_LINES) AppTimer.selectedExt = 0;
        return;
    }
    if(key == InputKeyLeft) {
        if(AppTimer.selectedExt == 0) AppTimer.tntMode1 = 0;
        if(AppTimer.selectedExt == 1) AppTimer.tntMode2 = 0;
        return;
    }
    if(key == InputKeyRight) {
        if(AppTimer.selectedExt == 0) AppTimer.tntMode1 = 1;
        if(AppTimer.selectedExt == 1) AppTimer.tntMode2 = 1;
        return;
    }

    if(key == InputKeyOk) {
        if(AppTimer.selectedExt == 0) {
            AppBzzztLoadParam(AppTimer.tntMode1_param);
            showScreen(SCREEN_ID_BZZZT_SET);
        }
    }
    if(key == InputKeyBack) {
        showScreen(SCREEN_ID_TIMER_EXT);
        return;
    }
}

void AppConfigKey(int key) {
    if(key == InputKeyUp) {
        if(AppConfig.selected == 0)
            AppConfig.selected = APP_CONFIG_LINES - 1;
        else
            AppConfig.selected--;
        return;
    }
    if(key == InputKeyDown) {
        AppConfig.selected++;
        if(AppConfig.selected == APP_CONFIG_LINES) AppConfig.selected = 0;
        return;
    }
    if(key == InputKeyLeft) {
        if(AppConfig.selected == 0) AppGlobal.lang = 0;
        if(AppConfig.selected == 1) AppGlobal.font = 0;
        if(AppConfig.selected == 2) AppGlobal.ir_detection = 0;
        return;
    }
    if(key == InputKeyRight) {
        if(AppConfig.selected == 0) AppGlobal.lang = 1;
        if(AppConfig.selected == 1) AppGlobal.font = 1;
        if(AppConfig.selected == 2) AppGlobal.ir_detection = 1;
        return;
    }
    if(key == InputKeyOk) {
        if(AppConfig.selected == 0) AppGlobal.lang = !AppGlobal.lang;
        if(AppConfig.selected == 1) AppGlobal.font = !AppGlobal.font;
        if(AppConfig.selected == 2) AppGlobal.ir_detection = !AppGlobal.ir_detection;
        return;
    }
    if(key == InputKeyBack) {
        showScreen(SCREEN_ID_TIME);
        return;
    }
}

void AppBzzztLoadParam(int p) {
    AppBzzzt.b = 0;
    AppBzzzt.v = 0;
    AppBzzzt.s = 0;
    if(p & BZZZT_FLAG_BLINK) AppBzzzt.b = 1;
    if(p & BZZZT_FLAG_VIBRO) AppBzzzt.v = 1;
    if(p & BZZZT_FLAG_SOUND) AppBzzzt.s = 1;
}
void AppBzzztKey(int key) {
    if(key == InputKeyLeft) {
        if(AppBzzzt.selected == 0) AppBzzzt.v = 0;
        if(AppBzzzt.selected == 1) AppBzzzt.b = 0;
        if(AppBzzzt.selected == 2) AppBzzzt.s = 0;
        return;
    }
    if(key == InputKeyRight) {
        if(AppBzzzt.selected == 0) AppBzzzt.v = 1;
        if(AppBzzzt.selected == 1) AppBzzzt.b = 1;
        if(AppBzzzt.selected == 2) AppBzzzt.s = 1;
        return;
    }
    if(key == InputKeyUp) {
        if(AppBzzzt.selected > 0)
            AppBzzzt.selected--;
        else
            AppBzzzt.selected = 2;
        return;
    }
    if(key == InputKeyDown) {
        AppBzzzt.selected++;
        if(AppBzzzt.selected > 2) AppBzzzt.selected = 0;
        return;
    }
    if(key == InputKeyOk) {
        if(AppBzzzt.selected == 0) AppBzzzt.v = !AppBzzzt.v;
        if(AppBzzzt.selected == 1) AppBzzzt.b = !AppBzzzt.b;
        if(AppBzzzt.selected == 2) AppBzzzt.s = !AppBzzzt.s;
        return;
    }
    if(key == InputKeyBack) {
        int p = 0;
        if(AppBzzzt.b) p |= BZZZT_FLAG_BLINK;
        if(AppBzzzt.v) p |= BZZZT_FLAG_VIBRO;
        if(AppBzzzt.s) p |= BZZZT_FLAG_SOUND;
        if(AppGlobal.prevScreen == SCREEN_ID_ALARM_EXT) {
            AppAlarm.tntMode1_param = p;
            showScreen(SCREEN_ID_ALARM_EXT);
        }
        if(AppGlobal.prevScreen == SCREEN_ID_TIMER_EXT) {
            AppTimer.tntMode1_param = p;
            showScreen(SCREEN_ID_TIMER_EXT);
        }
        return;
    }
}

void OnTimerTick() {
    bool bzzzt_busy = 0;
    SetScreenBacklightBrightness(AppGlobal.brightness);
    if(AppGlobal.dspBrightnessBarFrames > 0) AppGlobal.dspBrightnessBarFrames--;
    if(AppGlobal.ringing) {
        if(AppGlobal.irRecieved) {
            SetRing(0);
            if(AppGlobal.selectedScreen == SCREEN_ID_TIMER) AppTimerKey(InputKeyBack);
            if(AppGlobal.selectedScreen == SCREEN_ID_ALARM) AppAlarmKey(InputKeyBack);
        }
    }

    if(AppAlarm.state != APP_ALARM_STATE_OFF) {
        if(AppAlarm.sH == curr_dt.hour && AppAlarm.sM == curr_dt.minute && !curr_dt.second) {
            AppAlarm.state = APP_ALARM_STATE_BZZZ;
            if(AppAlarm.tntMode1 == 1 && AppAlarm.prior != 1) {
                SetRing(1);
                bzzzt_busy = 1;
                notification_BZZZT(AppAlarm.tntMode1_param);
            }
            if(AppAlarm.tntMode2 == 1) SetTNTmode2(1);
        }
    }

    if(AppAlarm.state == APP_ALARM_STATE_BZZZ) {
        showScreen(SCREEN_ID_ALARM);
        if(AppAlarm.tntMode1 == 1 && AppAlarm.prior != 1) {
            bzzzt_busy = 1;
            notification_BZZZT(AppAlarm.tntMode1_param);
        }
    }

    int ats = AppTimer.state;
    if(ats == APP_TIMER_STATE_ON) {
        if(AppTimer.count)
            AppTimer.count--;
        else {
            AppTimer.state = APP_TIMER_STATE_BZZZ;
            showScreen(SCREEN_ID_TIMER);
            if(AppTimer.tntMode1 == 1) {
                SetRing(1);
                notification_BZZZT(AppTimer.tntMode1_param);
            }
            if(AppTimer.tntMode2 == 1) SetTNTmode2(1);
        }
    }
    if(ats == APP_TIMER_STATE_BZZZ) {
        showScreen(SCREEN_ID_TIMER);
        if(AppTimer.tntMode1 == 1 && !bzzzt_busy) notification_BZZZT(AppTimer.tntMode1_param);
    }
}

void SetTNTmode2(int state) {
    if(state == 1) {
        furi_hal_gpio_write(&gpio_ext_pc3, 1);
        furi_hal_gpio_write(&gpio_ext_pb2, 0);
        SetLED(0, 160, 160, 1);
    } else {
        furi_hal_gpio_write(&gpio_ext_pc3, 0);
        furi_hal_gpio_write(&gpio_ext_pb2, 1);
        ResetLED();
    }
}

void SetRing(bool state) {
    AppGlobal.ringing = state;
    SetIR_rx(state);
}

void LoadParams() {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_migrate(storage, EXT_PATH("apps/Tools/alarmatik.bin"), SAVING_FILENAME);

    File* file = storage_file_alloc(storage);
    char ver = 0;
    if(storage_file_open(file, SAVING_FILENAME, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_read(file, &ver, sizeof(ver));
        if(ver == CFG_VERSION) {
            storage_file_read(file, &AppGlobal.lang, sizeof(AppGlobal.lang));
            storage_file_read(file, &AppGlobal.font, sizeof(AppGlobal.font));
            storage_file_read(file, &AppGlobal.ir_detection, sizeof(AppGlobal.ir_detection));
            storage_file_read(file, &AppGlobal.brightness, sizeof(AppGlobal.brightness));
            storage_file_read(file, &AppAlarm.sH, sizeof(AppAlarm.sH));
            storage_file_read(file, &AppAlarm.sM, sizeof(AppAlarm.sM));
            storage_file_read(file, &AppAlarm.state, sizeof(AppAlarm.state));
            storage_file_read(file, &AppAlarm.prior, sizeof(AppAlarm.prior));
            storage_file_read(file, &AppAlarm.tntMode1, sizeof(AppAlarm.tntMode1));
            storage_file_read(file, &AppAlarm.tntMode1_param, sizeof(AppAlarm.tntMode1_param));
            storage_file_read(file, &AppAlarm.tntMode2, sizeof(AppAlarm.tntMode2));
            storage_file_read(file, &AppAlarm.tntMode2_param, sizeof(AppAlarm.tntMode2_param));
            storage_file_read(file, &AppTimer.expected_count, sizeof(AppTimer.expected_count));
            storage_file_read(file, &AppTimer.tntMode1, sizeof(AppTimer.tntMode1));
            storage_file_read(file, &AppTimer.tntMode1_param, sizeof(AppTimer.tntMode1_param));
            storage_file_read(file, &AppTimer.tntMode2, sizeof(AppTimer.tntMode2));
            storage_file_read(file, &AppTimer.tntMode2_param, sizeof(AppTimer.tntMode2_param));
        } else
            showScreen(SCREEN_ID_CONFIG);
    } else
        showScreen(SCREEN_ID_CONFIG);
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    AppAlarm.sH_old = AppAlarm.sH;
    AppAlarm.sM_old = AppAlarm.sM;
    AppTimer.count = AppTimer.expected_count;
}

void SaveParams() {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    char ver = CFG_VERSION;
    if(storage_file_open(file, SAVING_FILENAME, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, &ver, sizeof(ver));
        storage_file_write(file, &AppGlobal.lang, sizeof(AppGlobal.lang));
        storage_file_write(file, &AppGlobal.font, sizeof(AppGlobal.font));
        storage_file_write(file, &AppGlobal.ir_detection, sizeof(AppGlobal.ir_detection));
        storage_file_write(file, &AppGlobal.brightness, sizeof(AppGlobal.brightness));
        storage_file_write(file, &AppAlarm.sH, sizeof(AppAlarm.sH));
        storage_file_write(file, &AppAlarm.sM, sizeof(AppAlarm.sM));
        storage_file_write(file, &AppAlarm.state, sizeof(AppAlarm.state));
        storage_file_write(file, &AppAlarm.prior, sizeof(AppAlarm.prior));
        storage_file_write(file, &AppAlarm.tntMode1, sizeof(AppAlarm.tntMode1));
        storage_file_write(file, &AppAlarm.tntMode1_param, sizeof(AppAlarm.tntMode1_param));
        storage_file_write(file, &AppAlarm.tntMode2, sizeof(AppAlarm.tntMode2));
        storage_file_write(file, &AppAlarm.tntMode2_param, sizeof(AppAlarm.tntMode2_param));
        storage_file_write(file, &AppTimer.expected_count, sizeof(AppTimer.expected_count));
        storage_file_write(file, &AppTimer.tntMode1, sizeof(AppTimer.tntMode1));
        storage_file_write(file, &AppTimer.tntMode1_param, sizeof(AppTimer.tntMode1_param));
        storage_file_write(file, &AppTimer.tntMode2, sizeof(AppTimer.tntMode2));
        storage_file_write(file, &AppTimer.tntMode2_param, sizeof(AppTimer.tntMode2_param));
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}
