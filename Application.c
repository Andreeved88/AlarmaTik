#include "F0App.h"
#include "F0CustomFonts.h"
extern F0App* FApp;

App_Global_Data AppGlobal = {SCREEN_ID_TIME, 5, 0, 2, 0, 0, 0, 0, 0};
App_Alarm_Data AppAlarm = {1, 8, 0, 8, 0, 0, 0};
App_Timer_Data AppTimer = {2, 300, 300, APP_TIMER_STATE_OFF};
App_Stopwatch_Data AppStopwatch = {0, 0, 0};

char time_string[TIME_STR_SIZE];
char date_string[TIME_STR_SIZE];
char timer_string[TIME_STR_SIZE];
char timer_string_trim[TIME_STR_SIZE];
char stopwatch_string[TIME_STR_SIZE];
char alarm_string[TIME_STR_SIZE];
DateTime curr_dt;
//InfraredWorker* worker;

void notification_beep_once() {
    notification_message(furi_record_open(RECORD_NOTIFICATION), &sequence_beep);
    notification_off();
}

void notification_off() {
    furi_record_close(RECORD_NOTIFICATION);
}

void notification_timeup() {
    SetScreenBacklightMode(3);
    notification_message(furi_record_open(RECORD_NOTIFICATION), &sequence_timeup);
    SetScreenBacklightMode(1);
}
/*
static void ir_received_callback(void* ctx, InfraredWorkerSignal* signal) {
    UNUSED(ctx);
    UNUSED(signal);
//    AppGlobal.irRecieved = 1;
}
*/
int AppInit() {
    /*
    worker = infrared_worker_alloc();
    infrared_worker_rx_enable_signal_decoding(worker, false);
    infrared_worker_rx_enable_blink_on_receiving(worker, true);    
    infrared_worker_rx_set_received_signal_callback(worker, ir_received_callback, 0);
    furi_hal_gpio_init(&gpio_infrared_tx, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);
    */
    LoadParams();
    SetScreenBacklightBrightness(AppGlobal.brightness);
    return 0;
}

int AppDeinit() {
    ResetLED();
    SetScreenBacklightBrightness((int)(FApp->SystemScreenBrightness * 100));
    SetScreenBacklightMode(3); //auto
    SetIR_rx(0);
    SaveParams();
    //    infrared_worker_free(worker);
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
    //uint8_t weekday; /**< Current weekday: 1-7 */
    snprintf(
        date_string,
        TIME_STR_SIZE,
        CLOCK_RFC_DATE_FORMAT,
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

    if(AppGlobal.selectedScreen == SCREEN_ID_TIME) { // ЧАСЫ С КУКУХОЙ
        if(AppGlobal.dspBrightnessBarFrames)
            elements_progress_bar_vertical(
                canvas, 121, 10, 40, (float)(AppGlobal.brightness / 100.f));
        canvas_set_custom_u8g2_font(canvas, TechnoDigits15);
        canvas_draw_str(canvas, TIME_POS_X, TIME_POS_Y, time_string);

        if(!AppGlobal.show_time_only) {
            //          canvas_set_font(canvas, FontSecondary);
            canvas_set_custom_u8g2_font(canvas, quadro7);
            elements_button_left(canvas, "Буд");
            elements_button_center(canvas, "Сек");
            elements_button_right(canvas, "Тмр");
            //            if (AppGlobal.irRxOn) canvas_draw_icon(canvas, 0, 22, &I_IR_On);
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
        //      canvas_set_font(canvas, FontSecondary);
        canvas_set_custom_u8g2_font(canvas, quadro7);
        canvas_draw_str_aligned(canvas, 0, 0, AlignLeft, AlignTop, time_string);
        canvas_draw_str_aligned(canvas, 128, 0, AlignRight, AlignTop, date_string);
        elements_button_left(canvas, "Сброс");
        if(AppStopwatch.running)
            elements_button_center(canvas, "Стоп");
        else
            elements_button_center(canvas, "Пуск");
    }

    if(AppGlobal.selectedScreen == SCREEN_ID_TIMER) { //ТАЙМЕР
        canvas_set_custom_u8g2_font(canvas, TechnoDigits15);
        canvas_draw_str(canvas, TIME_POS_X, TIME_POS_Y, timer_string);
        if(AppTimer.count)
            elements_progress_bar(
                canvas, 0, 0, 128, (1.0 * AppTimer.count / AppTimer.expected_count));
        //      canvas_set_font(canvas, FontSecondary);
        canvas_set_custom_u8g2_font(canvas, quadro7);

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
            elements_slightly_rounded_box(canvas, shiftX, TIME_POS_Y - 5, 30, 2);
            elements_button_center(canvas, "Пуск");
            return;
        }

        if(AppTimer.state == APP_TIMER_STATE_ON) { //запущен
            elements_button_center(canvas, "Стоп");
            return;
        }

        if(AppTimer.state == APP_TIMER_STATE_PAUSE) {
            elements_button_left(canvas, "Сброс");
            elements_button_center(canvas, "Пуск");
            return;
        }

        if(AppTimer.state == APP_TIMER_STATE_BZZZ) {
            elements_button_center(canvas, "Выкл");
            return;
        }
    }

    if(AppGlobal.selectedScreen == SCREEN_ID_ALARM) { //БУДИЛЬНИК
        int shiftX = TIME_POS_X + 20;
        if(AppAlarm.selected == 2) shiftX = TIME_POS_X + 60;
        canvas_set_custom_u8g2_font(canvas, TechnoDigits15);
        canvas_draw_str(canvas, TIME_POS_X + 20, TIME_POS_Y, alarm_string);
        elements_slightly_rounded_box(canvas, shiftX, TIME_POS_Y - 5, 30, 2);
        // canvas_set_font(canvas, FontSecondary);
        canvas_set_custom_u8g2_font(canvas, quadro7);
        canvas_draw_str_aligned(canvas, 0, 0, AlignLeft, AlignTop, time_string);
        canvas_draw_str_aligned(canvas, 128, 0, AlignRight, AlignTop, date_string);
        if(AppAlarm.state) {
            if(AppAlarm.state == APP_ALARM_STATE_BZZZ)
                elements_button_center(canvas, "Спать");
            else
                elements_button_center(canvas, "Выкл");
        } else
            elements_button_center(canvas, "Вкл");
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
        }
        if(key == InputKeyOk) {
            if(ss == SCREEN_ID_TIME) { /*
                if(AppGlobal.irRxOn)
                    SetIR_rx(0);
                else
                    SetIR_rx(1);*/
                AppGlobal.show_time_only = !AppGlobal.show_time_only;
            }
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
    }

    if(type == InputTypeShort) {
        if(key == InputKeyBack) {
            if(ss == SCREEN_ID_STOPWATCH) {
                AppGlobal.selectedScreen = SCREEN_ID_TIME;
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
            return 0;
        }
        if(key == InputKeyOk) {
            if(ss == SCREEN_ID_STOPWATCH) {
                AppStopwatchToggle();
                notification_beep_once();
                return 0;
            }
            if(ss == SCREEN_ID_TIME) {
                AppGlobal.selectedScreen = SCREEN_ID_STOPWATCH;
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
                AppGlobal.selectedScreen = SCREEN_ID_ALARM;
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKeyLeft();
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarm.selected--;
                if(!AppAlarm.selected) AppAlarm.selected = 2;
                return 0;
            }
            return 0;
        }

        if(key == InputKeyRight) {
            if(ss == SCREEN_ID_TIME) {
                AppGlobal.selectedScreen = SCREEN_ID_TIMER;
                return 0;
            }
            if(ss == SCREEN_ID_TIMER) {
                AppTimerKeyRight();
                return 0;
            }
            if(ss == SCREEN_ID_ALARM) {
                AppAlarm.selected++;
                if(AppAlarm.selected > 2) AppAlarm.selected = 1;
                return 0;
            }
            return 0;
        }
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
    } else if(AppGlobal.brightness == 0) { //trigger on every down press afterwards
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
        SetRing(0);
        return;
    }
}

void AppTimerKeyBack() {
    if(AppTimer.state == APP_TIMER_STATE_BZZZ) {
        AppTimerReset();
        SetRing(0);
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
    if(AppTimer.state != APP_TIMER_STATE_OFF) return;
    AppTimer.selected++;
    if(AppTimer.selected > 3) AppTimer.selected = 1;
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

void AppAlarmKeyOk() {
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
        };
        SetRing(0);
        AppGlobal.selectedScreen = SCREEN_ID_TIME;
        return;
    };
}

void AppAlarmKeyBack() {
    if(AppAlarm.state == APP_ALARM_STATE_BZZZ) AppAlarm.state = APP_ALARM_STATE_SLEEP;
    SetRing(0);
    AppGlobal.selectedScreen = SCREEN_ID_TIME;
    AppAlarm.sH = AppAlarm.sH_old;
    AppAlarm.sM = AppAlarm.sM_old;
}

void OnTimerTick() {
    SetScreenBacklightBrightness(AppGlobal.brightness);
    if(AppGlobal.dspBrightnessBarFrames > 0) AppGlobal.dspBrightnessBarFrames--;
    if(AppGlobal.ringing) { //если гудит
        SetIrBlink(1);
        if(AppGlobal.irRecieved) { //рукой махали - считай нажали взад
            SetRing(0);
            AppGlobal.irRecieved = 0;
            if(AppGlobal.selectedScreen == SCREEN_ID_TIMER) AppTimerKeyBack();
            if(AppGlobal.selectedScreen == SCREEN_ID_ALARM) AppAlarmKeyBack();
        }
    }

    if(AppAlarm.state != APP_ALARM_STATE_OFF) {
        if(AppAlarm.sH == curr_dt.hour && AppAlarm.sM == curr_dt.minute && !curr_dt.second) {
            AppAlarm.state = APP_ALARM_STATE_BZZZ;
            SetRing(1);
        }
    }

    if(AppAlarm.state == APP_ALARM_STATE_BZZZ) {
        AppGlobal.selectedScreen = SCREEN_ID_ALARM;
        notification_timeup();
    }

    int ats = AppTimer.state;
    if(ats == APP_TIMER_STATE_ON) {
        if(AppTimer.count)
            AppTimer.count--;
        else {
            AppTimer.state = APP_TIMER_STATE_BZZZ;
            AppGlobal.selectedScreen = SCREEN_ID_TIMER;
            SetRing(1);
        }
    }
    if(ats == APP_TIMER_STATE_BZZZ) notification_timeup();
}

void SetIrBlink(bool state) {
    UNUSED(state);
    //    furi_hal_gpio_write(&gpio_infrared_tx, state);
    return;
}

void SetIR_rx(bool state) {
    AppGlobal.irRecieved = 0;
    if(AppGlobal.irRxOn == state) return;
    AppGlobal.irRxOn = state;
    if(state) {
        AppGlobal.irRxOn = 1;
        //        infrared_worker_rx_start(worker);
    } else {
        AppGlobal.irRxOn = 0;
        //        infrared_worker_rx_stop(worker);
    }
    SetIrBlink(state);
}

void SetRing(bool state) {
    AppGlobal.ringing = state;
    if(!state) {
        notification_off();
        SetScreenBacklightMode(2);
    }
    //    SetIR_rx(state);
}

void LoadParams() {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_migrate(storage, EXT_PATH("apps/Tools/alarmatik.bin"), SAVING_FILENAME);

    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, SAVING_FILENAME, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_read(file, &AppAlarm.sH, sizeof(AppAlarm.sH));
        storage_file_read(file, &AppAlarm.sM, sizeof(AppAlarm.sM));
        storage_file_read(file, &AppAlarm.state, sizeof(AppAlarm.state));
        storage_file_read(file, &AppGlobal.brightness, sizeof(AppGlobal.brightness));
        storage_file_read(file, &AppTimer.expected_count, sizeof(AppTimer.expected_count));
    } else {
        AppAlarm.sH = 8;
        AppAlarm.sM = 0;
        AppAlarm.state = APP_ALARM_STATE_OFF;
        AppGlobal.brightness = 5;
        AppTimer.expected_count = 300;
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    DateTime dt;
    AppAlarm.system_state = furi_hal_rtc_get_alarm(&dt);

    furi_hal_rtc_set_alarm(NULL, false);
    AppAlarm.sH_old = AppAlarm.sH;
    AppAlarm.sM_old = AppAlarm.sM;
    AppTimer.count = AppTimer.expected_count;
    UpdateView();
}

void SaveParams() {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, SAVING_FILENAME, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, &AppAlarm.sH_old, sizeof(AppAlarm.sH_old));
        storage_file_write(file, &AppAlarm.sM_old, sizeof(AppAlarm.sM_old));
        storage_file_write(file, &AppAlarm.state, sizeof(AppAlarm.state));
        storage_file_write(file, &AppGlobal.brightness, sizeof(AppGlobal.brightness));
        storage_file_write(file, &AppTimer.expected_count, sizeof(AppTimer.expected_count));
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    if(AppAlarm.state != APP_ALARM_STATE_OFF) {
        DateTime dt;
        dt.hour = AppAlarm.sH;
        dt.minute = AppAlarm.sM;
        furi_hal_rtc_set_alarm(&dt, 1);
    } else
        furi_hal_rtc_set_alarm(NULL, AppAlarm.system_state);
}
