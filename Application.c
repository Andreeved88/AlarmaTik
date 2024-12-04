#include "F0App.h"
#include "F0CustomFonts.h"
extern F0App* FApp;

#define DEVB 0
char* CaptionsRus[] = {"[0]",     "Выкл",        "Вкл",   "Сброс",     "Пуск",
                       "Стоп",    "Lang / Язык", "Рус",   "Eng",       "Пакет рун",
                       "База",    "Система",     "БЗЗЗТ", "C3+ B2-",   "ИК приёмник",
                       "Буд",     "Сек",         "Тмр",   "Спать",     "Режим",
                       "Настр",   "Пнд",         "Втр",   "Срд",       "Чтр",
                       "Птн",     "Сбт",         "Вск",   "Приоритет", "Встроенный",
                       "Внешний", "Равный",      "Вибро", "Мигалка",   "Пикалка"};

char* CaptionsEng[] = {"[0]",         "Off",     "On",          "Reset", "Start",     "Stop",
                       "Lang / Язык", "Рус",     "Eng",         "Font",  "Internal",  "System",
                       "BZZZT",       "C3+ B2-", "IR reciever", "Alarm", "SW",        "Timer",
                       "Sleep",       "Action",  "Set",         "Mon",   "Tue",       "Wed",
                       "Thr",         "Fri",     "Sat",         "Sun",   "Приоритет", "Встроенный",
                       "Внешний",     "Равный",  "Vibro",       "Blink", "Sounds"};

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
    .selected = 1,
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
    .selected = 2,
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
App_Stopwatch_Data AppStopwatch = {.running = 0, .start_timestamp = 0, .stopped_seconds = 0};

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
    &message_delay_100,
    &message_sound_off,
    NULL,
};

static NotificationSequence sequence_bzzzt = {
    &message_vibro_on,
    &message_force_display_brightness_setting_1f,
    &message_display_backlight_on,
    &message_note_c8,
    &message_delay_50,
    &message_sound_off,
    &message_delay_100,
    &message_note_c8,
    &message_delay_50,
    &message_sound_off,
    &message_delay_100,
    &message_note_c8,
    &message_delay_50,
    &message_sound_off,
    &message_delay_100,
    &message_vibro_off,
    &message_display_backlight_off,
    &message_delay_500,
    NULL,
};

void notification_beep_once() {
    notification_message(FApp->Notificator, &sequence_beep);
}

void notification_BZZZT(int params) {
    bool v = params & BZZZT_FLAG_VIBRO;
    bool b = params & BZZZT_FLAG_BLINK;
    if(!v)
        sequence_bzzzt[0] = &message_vibro_off;
    else
        sequence_bzzzt[0] = &message_vibro_on;

    if(!b) {
        sequence_bzzzt[1] = &message_delay_1;
        sequence_bzzzt[2] = &message_delay_1;
        sequence_bzzzt[16] = &message_delay_1;
    } else {
        sequence_bzzzt[1] = &message_force_display_brightness_setting_1f;
        sequence_bzzzt[2] = &message_display_backlight_on;
        sequence_bzzzt[16] = &message_display_backlight_off;
    };
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
    SetScreenBacklightMode(3); //auto
    SetIR_rx(0);
    furi_hal_gpio_init(&gpio_ext_pc3, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_init(&gpio_ext_pb2, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(&gpio_ext_pc3, 0);
    furi_hal_gpio_write(&gpio_ext_pb2, 0);
    SaveParams();
    infrared_worker_free(worker);
    return 0;
}

void elements_progress_bar_vertical(
    Canvas* canvas,
    uint8_t x,
    uint8_t y,
    uint8_t height,
    float progress) {
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
    uint32_t curr_ts = datetime_datetime_to_timestamp(&curr_dt);
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

    if(AppGlobal.selectedScreen == SCREEN_ID_TIME) { //MAIN SCREEN
        canvas_set_custom_u8g2_font(canvas, TechnoDigits15);
        canvas_draw_str(canvas, TIME_POS_X, TIME_POS_Y, time_string);
        if(AppGlobal.dspBrightnessBarFrames) {
            elements_progress_bar_vertical(
                canvas, 121, 0, 64, (float)(AppGlobal.brightness / 100.f));
            return;
        }
        if(!AppGlobal.show_time_only) {
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
        }
    }

    if(AppGlobal.selectedScreen == SCREEN_ID_STOPWATCH) { //СЕКУНДОМЕР
        int32_t elapsed_secs = AppStopwatch.running ? (curr_ts - AppStopwatch.start_timestamp) :
                                                      AppStopwatch.stopped_seconds;
        snprintf(
            stopwatch_string,
            TIME_STR_SIZE,
            "%.2ld:%.2ld:%.2ld",
            elapsed_secs / 3600,
            elapsed_secs / 60,
            elapsed_secs % 60);

        canvas_set_custom_u8g2_font(canvas, TechnoDigits15);
        canvas_draw_str(canvas, TIME_POS_X, TIME_POS_Y, stopwatch_string);
        ApplyFont(canvas);
        canvas_draw_str_aligned(canvas, 0, 0, AlignLeft, AlignTop, time_string);
        canvas_draw_str_aligned(canvas, 128, 0, AlignRight, AlignTop, date_string);
        elements_button_left(canvas, getStr(STR_GLOBAL_RESET));
        if(AppStopwatch.running)
            elements_button_center(canvas, getStr(STR_GLOBAL_STOP));
        else
            elements_button_center(canvas, getStr(STR_GLOBAL_START));
    }

    if(AppGlobal.selectedScreen == SCREEN_ID_TIMER) { //ТАЙМЕР
        canvas_set_custom_u8g2_font(canvas, TechnoDigits15);
        canvas_draw_str(canvas, TIME_POS_X, TIME_POS_Y, timer_string);
        if(AppTimer.count && AppTimer.state != APP_TIMER_STATE_OFF)
            elements_progress_bar(
                canvas, 0, 0, 128, (1.0 * AppTimer.count / AppTimer.expected_count));
        ApplyFont(canvas);

        if(AppTimer.state == APP_TIMER_STATE_OFF) {
            int shiftX = 0;
            switch(AppTimer.selected) {
            case 1:
                shiftX = 10;
                break;
            case 2:
                shiftX = 50;
                break;
            case 3:
                shiftX = 90;
                break;
            }
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
        elements_button_right(canvas, getStr(STR_TNT_SETTINGS));
        return;
    }

    if(AppGlobal.selectedScreen == SCREEN_ID_ALARM) { //БУДИЛЬНИК
        int shiftX = TIME_POS_X + 20;
        if(AppAlarm.selected == 2) shiftX = TIME_POS_X + 60;
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
        elements_button_right(canvas, getStr(STR_TNT_SETTINGS));
        return;
    }

    if(AppGlobal.selectedScreen == SCREEN_ID_CONFIG) {
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
        int x = 0, y = 0, w = 128, h = 64;
        canvas_draw_rframe(canvas, x, y, w, h, 1);
        canvas_draw_rframe(canvas, x + 1, (AppAlarm.selectedExt * 13) + y + 1, w - 2, 13, 2);
        ApplyFont(canvas);
        x += 2;
        y += 4;
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_CONF_TNT_BZZZT));
        canvas_draw_str_aligned(canvas, x, y + 13, AlignLeft, AlignTop, getStr(STR_CONF_TNT_PC3));
        canvas_draw_str_aligned(canvas, x, y + 26, AlignLeft, AlignTop, getStr(STR_ALARM_PRIOR));
        int str_id1 = 0, str_id2 = 0, str_id3 = 0;
        if(AppAlarm.tntMode1 == 0)
            str_id1 = STR_GLOBAL_OFF;
        else
            str_id1 = STR_GLOBAL_ON;
        if(AppAlarm.tntMode2 == 0)
            str_id2 = STR_GLOBAL_OFF;
        else
            str_id2 = STR_GLOBAL_ON;
        str_id3 = STR_ALARM_PRIOR_INT + AppAlarm.prior;
        canvas_draw_str_aligned(canvas, x + w - 5, y, AlignRight, AlignTop, getStr(str_id1));
        canvas_draw_str_aligned(canvas, x + w - 5, y + 13, AlignRight, AlignTop, getStr(str_id2));
        canvas_draw_str_aligned(canvas, x + w - 5, y + 26, AlignRight, AlignTop, getStr(str_id3));
        if(AppAlarm.selectedExt == 0) elements_button_center(canvas, getStr(STR_TNT_SETTINGS));
    }

    if(AppGlobal.selectedScreen == SCREEN_ID_TIMER_EXT) {
        int x = 0, y = 0, w = 128, h = 64;
        canvas_draw_rframe(canvas, x, y, w, h, 1);
        canvas_draw_rframe(canvas, x + 1, (AppTimer.selectedExt * 13) + y + 1, w - 2, 13, 2);
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
        int x = 0, y = 0, w = 128, h = 64;
        canvas_draw_rframe(canvas, x, y, w, h, 1);
        canvas_draw_rframe(canvas, x + 1, (AppBzzzt.selected * 13) + y + 1, w - 2, 13, 2);
        ApplyFont(canvas);
        x += 2;
        y += 4;
        int str_id1 = STR_GLOBAL_OFF, str_id2 = STR_GLOBAL_OFF;
        if(AppBzzzt.v == 1) str_id1 = STR_GLOBAL_ON;
        if(AppBzzzt.b == 1) str_id2 = STR_GLOBAL_ON;
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_BZZZT_VIBRO));
        canvas_draw_str_aligned(canvas, x + w - 5, y, AlignRight, AlignTop, getStr(str_id1));
        y += 13;
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, getStr(STR_BZZZT_BLINK));
        canvas_draw_str_aligned(canvas, x + w - 5, y, AlignRight, AlignTop, getStr(str_id2));
    }
}

int KeyProc(int type, int key) {
    if(type == InputTypeMAX) {
        if(key == 255) OnTimerTick();
        return 0;
    }
    int ss = AppGlobal.selectedScreen;
    if(type == InputTypeLong) {
        if(key == InputKeyBack) {
            if(ss == SCREEN_ID_TIME) return 255; //exit
            return 0;
        }
        if(key == InputKeyRight) {
            if(ss == SCREEN_ID_TIMER) showScreen(SCREEN_ID_TIMER_EXT);
            if(ss == SCREEN_ID_ALARM) showScreen(SCREEN_ID_ALARM_EXT);
            return 0;
        }
        if(key == InputKeyOk) {
            if(ss == SCREEN_ID_TIME) AppGlobal.show_time_only = !AppGlobal.show_time_only;
            return 0;
        }
    }
    if(type == InputTypeRepeat) {
        if(key == InputKeyUp) {
            if(ss == SCREEN_ID_TIME) {
                AppTimeKeyUp();
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKeyUp();
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarmKeyUp();
                return 0;
            }
            return 0;
        }
        if(key == InputKeyDown) {
            if(ss == SCREEN_ID_TIME) {
                AppTimeKeyDown();
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKeyDown();
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarmKeyDown();
                return 0;
            }
            return 0;
        }
        return 0;
    }
    if(type == InputTypeShort) {
        if(ss == SCREEN_ID_ALARM_EXT) {
            AppAlarmExtKey(key);
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
        if(key == InputKeyBack) {
            if(ss == SCREEN_ID_STOPWATCH) {
                showScreen(SCREEN_ID_TIME);
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKeyBack();
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarmKeyBack();
                return 0;
            }
            if(ss == SCREEN_ID_TIME) {
                showScreen(SCREEN_ID_CONFIG);
                return 0;
            }
            return 0;
        }
        if(key == InputKeyOk) {
            if(ss == SCREEN_ID_STOPWATCH) {
                AppStopwatchToggle();
                notification_beep_once();
                return 0;
            }
            if(ss == SCREEN_ID_TIME) {
                showScreen(SCREEN_ID_STOPWATCH);
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKeyOk();
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarmKeyOk();
                return 0;
            }
            return 0;
        }
        if(key == InputKeyUp) {
            if(ss == SCREEN_ID_TIME) {
                AppTimeKeyUp();
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKeyUp();
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarmKeyUp();
                return 0;
            }
            return 0;
        }
        if(key == InputKeyDown) {
            if(ss == SCREEN_ID_TIME) {
                AppTimeKeyDown();
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKeyDown();
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarmKeyDown();
                return 0;
            }
            return 0;
        }
        if(key == InputKeyLeft) {
            if(ss == SCREEN_ID_STOPWATCH) {
                AppStopwatchReset();
                return 0;
            }
            if(ss == SCREEN_ID_TIME) {
                showScreen(SCREEN_ID_ALARM);
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKeyLeft();
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarmKeyLeft();
                return 0;
            }
            return 0;
        }
        if(key == InputKeyRight) {
            if(ss == SCREEN_ID_TIME) {
                showScreen(SCREEN_ID_TIMER);
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKeyRight();
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarmKeyRight();
                return 0;
            }
            return 0;
        }
        return 0;
    }
    return 0;
}

void AppTimeKeyUp() {
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
}
void AppTimeKeyDown() {
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
}

void AppStopwatchToggle() {
    uint32_t curr_ts = furi_hal_rtc_get_timestamp();
    if(AppStopwatch.running)
        AppStopwatch.stopped_seconds = curr_ts - AppStopwatch.start_timestamp;
    else {
        if(!AppStopwatch.start_timestamp)
            AppStopwatch.start_timestamp = curr_ts;
        else
            AppStopwatch.start_timestamp = curr_ts - AppStopwatch.stopped_seconds;
    }
    AppStopwatch.running = !AppStopwatch.running;
}
void AppStopwatchReset() {
    if(AppStopwatch.start_timestamp) {
        AppStopwatch.running = 0;
        AppStopwatch.start_timestamp = 0;
        AppStopwatch.stopped_seconds = 0;
    }
}

void AppTimerKeyUp() {
    if(AppTimer.state != APP_TIMER_STATE_OFF) return;
    switch(AppTimer.selected) {
    case 1:
        AppTimer.count += 60 * 60;
        break;
    case 2:
        AppTimer.count += 60;
        break;
    case 3:
        AppTimer.count += 1;
        break;
    default:
        break;
    }
    if(AppTimer.count > 100 * 60 * 60 - 1) AppTimer.count = 100 * 60 * 60 - 1;
    AppTimer.expected_count = AppTimer.count;
}
void AppTimerKeyDown() {
    if(AppTimer.state != APP_TIMER_STATE_OFF) return;
    switch(AppTimer.selected) {
    case 1:
        AppTimer.count -= 60 * 60;
        break;
    case 2:
        AppTimer.count -= 60;
        break;
    case 3:
        AppTimer.count -= 1;
        break;
    default:
        break;
    }
    if(AppTimer.count < 0) AppTimer.count = 0;
    AppTimer.expected_count = AppTimer.count;
}
void AppTimerKeyOk() {
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
}
void AppTimerKeyBack() {
    if(AppTimer.state == APP_TIMER_STATE_BZZZ) {
        AppTimerReset();
        if(AppTimer.tntMode1 == 1) SetRing(0);
        if(AppTimer.tntMode2 == 1) SetTNTmode2(0);
    }
    AppGlobal.selectedScreen = SCREEN_ID_TIME;
}
void AppTimerKeyLeft() { //RESET
    if(AppTimer.state == APP_TIMER_STATE_OFF) {
        AppTimer.selected--;
        if(!AppTimer.selected) AppTimer.selected = 3;
        return;
    }

    if(AppTimer.state == APP_TIMER_STATE_PAUSE) {
        AppTimerReset();
        return;
    }
}
void AppTimerKeyRight() {
    if(AppTimer.state == APP_TIMER_STATE_OFF) {
        AppTimer.selected++;
        if(AppTimer.selected == 4) AppTimer.selected = 1;
    }
}
void AppTimerReset() {
    AppTimer.state = APP_TIMER_STATE_OFF;
    AppTimer.count = AppTimer.expected_count;
}

void AppAlarmKeyUp() {
    switch(AppAlarm.selected) {
    case 1:
        ++AppAlarm.sH;
        if(AppAlarm.sH == 24) AppAlarm.sH = 0;
        break;
    case 2:
        ++AppAlarm.sM;
        if(AppAlarm.sM == 60) AppAlarm.sM = 0;
        break;
    default:
        break;
    }
    AppAlarm.sH_old = AppAlarm.sH;
    AppAlarm.sM_old = AppAlarm.sM;
}
void AppAlarmKeyDown() {
    switch(AppAlarm.selected) {
    case 1:
        if(AppAlarm.sH == 0)
            AppAlarm.sH = 23;
        else
            AppAlarm.sH--;
        break;
    case 2:
        if(AppAlarm.sM == 0)
            AppAlarm.sM = 59;
        else
            AppAlarm.sM--;
        break;
    default:
        break;
    }
    AppAlarm.sH_old = AppAlarm.sH;
    AppAlarm.sM_old = AppAlarm.sM;
}
void AppAlarmKeyLeft() {
    AppAlarm.selected = 1;
    return;
}
void AppAlarmKeyRight() {
    AppAlarm.selected = 2;
    return;
}
void AppAlarmKeyOk() {
    if(AppAlarm.state == APP_ALARM_STATE_OFF) {
        AppAlarm.state = APP_ALARM_STATE_ON;
        if(AppAlarm.prior == 1) {
            DateTime dt;
            dt.hour = AppAlarm.sH;
            dt.minute = AppAlarm.sM;
            furi_hal_rtc_set_alarm(&dt, 1);
        }
        return;
    };

    if(AppAlarm.state == APP_ALARM_STATE_ON || AppAlarm.state == APP_ALARM_STATE_SLEEP) {
        AppAlarm.state = APP_ALARM_STATE_OFF;
        AppAlarm.sH = AppAlarm.sH_old;
        AppAlarm.sM = AppAlarm.sM_old;
        if(AppAlarm.prior == 1) {
            DateTime dt;
            dt.hour = AppAlarm.sH;
            dt.minute = AppAlarm.sM;
            furi_hal_rtc_set_alarm(&dt, 0);
        }
        return;
    };

    if(AppAlarm.state == APP_ALARM_STATE_BZZZ) {
        AppAlarm.state = APP_ALARM_STATE_SLEEP;
        AppAlarm.sM += 5;
        if(AppAlarm.sM > 59) {
            AppAlarm.sM = AppAlarm.sM - 60;
            AppAlarm.sH++;
            if(AppAlarm.sH > 23) AppAlarm.sH = 0;
            if(AppAlarm.prior == 1) {
                DateTime dt;
                dt.hour = AppAlarm.sH;
                dt.minute = AppAlarm.sM;
                furi_hal_rtc_set_alarm(&dt, 1);
            };
            if(AppAlarm.tntMode1 == 1) SetRing(0);
            if(AppAlarm.tntMode2 == 1) SetTNTmode2(0);
            showScreen(SCREEN_ID_TIME);
            return;
        };
    }
}
void AppAlarmKeyBack() {
    if(AppAlarm.state == APP_ALARM_STATE_BZZZ) AppAlarm.state = APP_ALARM_STATE_SLEEP;
    if(AppAlarm.tntMode1 == 1) SetRing(0);
    if(AppAlarm.tntMode2 == 1) SetTNTmode2(0);
    showScreen(SCREEN_ID_TIME);
    AppAlarm.sH = AppAlarm.sH_old;
    AppAlarm.sM = AppAlarm.sM_old;
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
        if(AppConfig.selected == 0) {
            AppGlobal.lang = 0;
        }
        if(AppConfig.selected == 1) {
            AppGlobal.font = 0;
        }
        if(AppConfig.selected == 2) {
            AppGlobal.ir_detection = 0;
        }
        return;
    }
    if(key == InputKeyRight) {
        if(AppConfig.selected == 0) {
            AppGlobal.lang = 1;
        }
        if(AppConfig.selected == 1) {
            AppGlobal.font = 1;
        }
        if(AppConfig.selected == 2) {
            AppGlobal.ir_detection = 1;
        }
        return;
    }
    if(key == InputKeyBack) {
        showScreen(SCREEN_ID_TIME);
        return;
    }
}

void AppBzzztLoadParam(int p) {
    if(p & BZZZT_FLAG_BLINK)
        AppBzzzt.b = 1;
    else
        AppBzzzt.b = 0;
    if(p & BZZZT_FLAG_VIBRO)
        AppBzzzt.v = 1;
    else
        AppBzzzt.v = 0;
    if(p & BZZZT_FLAG_SOUND)
        AppBzzzt.s = 1;
    else
        AppBzzzt.s = 0;
}
void AppBzzztKey(int key) {
    if(key == InputKeyLeft) {
        if(AppBzzzt.selected == 0) {
            AppBzzzt.v = 0;
        }
        if(AppBzzzt.selected == 1) {
            AppBzzzt.b = 0;
        }
        return;
    }
    if(key == InputKeyRight) {
        if(AppBzzzt.selected == 0) {
            AppBzzzt.v = 1;
        }
        if(AppBzzzt.selected == 1) {
            AppBzzzt.b = 1;
        }
        return;
    }

    if(key == InputKeyUp) {
        if(AppBzzzt.selected > 0) {
            AppBzzzt.selected--;
        }
        return;
    }
    if(key == InputKeyDown) {
        if(AppBzzzt.selected < 1) {
            AppBzzzt.selected++;
        }

        return;
    }

    if(key == InputKeyOk) {
        if(AppBzzzt.selected == 0) AppBzzzt.v = !AppBzzzt.v;
        if(AppBzzzt.selected == 1) AppBzzzt.b = !AppBzzzt.b;
        if(AppBzzzt.selected == 2) AppBzzzt.s = !AppBzzzt.s;

        return;
    }

    if(key == InputKeyBack) {
        int p = BZZZT_FLAG_SOUND;
        if(AppBzzzt.b) p |= BZZZT_FLAG_BLINK;
        if(AppBzzzt.v) p |= BZZZT_FLAG_VIBRO;
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
    SetScreenBacklightBrightness(AppGlobal.brightness);
    if(AppGlobal.dspBrightnessBarFrames > 0) AppGlobal.dspBrightnessBarFrames--;
    if(AppGlobal.ringing) {
        if(AppGlobal.irRecieved) {
            SetRing(0);
            AppGlobal.irRecieved = 0;
            if(AppGlobal.selectedScreen == SCREEN_ID_TIMER) AppTimerKeyBack();
            if(AppGlobal.selectedScreen == SCREEN_ID_ALARM) AppAlarmKeyBack();
        }
    }

    if(AppAlarm.state != APP_ALARM_STATE_OFF) {
        if(AppAlarm.sH == curr_dt.hour && AppAlarm.sM == curr_dt.minute && !curr_dt.second) {
            AppAlarm.state = APP_ALARM_STATE_BZZZ;
            if(AppAlarm.tntMode1 == 1 && AppAlarm.prior != 1) SetRing(1);
            if(AppAlarm.tntMode2 == 1) SetTNTmode2(1);
        }
    }

    if(AppAlarm.state == APP_ALARM_STATE_BZZZ) {
        showScreen(SCREEN_ID_ALARM);
        if(AppAlarm.tntMode1 == 1 && AppAlarm.prior != 1)
            notification_BZZZT(AppAlarm.tntMode1_param);
    }

    int ats = AppTimer.state;
    if(ats == APP_TIMER_STATE_ON) {
        if(AppTimer.count)
            AppTimer.count--;
        else {
            AppTimer.state = APP_TIMER_STATE_BZZZ;
            showScreen(SCREEN_ID_TIMER);
            if(AppTimer.tntMode1 == 1) SetRing(1);
            if(AppTimer.tntMode2 == 1) SetTNTmode2(1);
        }
    }
    if(ats == APP_TIMER_STATE_BZZZ) {
        if(AppTimer.tntMode1 == 1) notification_BZZZT(AppTimer.tntMode1_param);
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
    if(!state) SetScreenBacklightMode(2);
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

    DateTime dt;
    AppAlarm.system_state = furi_hal_rtc_get_alarm(&dt);

    if(AppAlarm.prior == 0) furi_hal_rtc_set_alarm(NULL, false);
    AppAlarm.sH_old = AppAlarm.sH;
    AppAlarm.sM_old = AppAlarm.sM;
    AppTimer.count = AppTimer.expected_count;
    UpdateView();
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

    DateTime dt;
    if(AppAlarm.prior == 0) {
        if(AppAlarm.state != APP_ALARM_STATE_OFF) {
            dt.hour = AppAlarm.sH;
            dt.minute = AppAlarm.sM;
            furi_hal_rtc_set_alarm(&dt, 1);
        } else
            furi_hal_rtc_set_alarm(NULL, AppAlarm.system_state);
    }
}
