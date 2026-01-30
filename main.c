#include "bsp.h"
#include "eeprom_emulation_type_a.h"
#include "MusicPlayer.h"
#include "MusicScore.h"
#define MAIN_ADDRESS 0x10000
#define KICKTIME 32000

typedef enum {ALARM1 = 1, ALARM2 = 2, ALARM3 = 3} ALARM_NUMBER;
typedef enum {ALARM_MENU_STATE = 3, ALARM_SETTING} ALARM_SYSTEM_STATE;

typedef struct {
    volatile bool enabled;
    volatile uint16_t hour;
    volatile uint16_t minute;
    volatile uint16_t second;
    volatile uint16_t music_id;
} alarm_t;

volatile alarm_t alarms[3] = {
    {true, 7, 0, 0, 1},
    {true, 12, 30, 0, 2},
    {true, 22, 0, 0, 3}
};

const char* music_names[3] = {"HARU", "Music2", "Music3"};
typedef enum{_24h=0,_12h=1}TIME_FORM;

typedef enum{TIME_DISPLAY=0, MENU_PAGE1, MENU_PAGE2, SET_FORM, STOPWATCH, ALARM_MENU, ALARM_SETTING_STATE}SYSTEM_STATE;

typedef enum{SET_TIME=1,SET_DATE,SET_APM,SET_SAVE,SET_PAGE2}SETTING_CHOICE_PAGE1;
typedef enum{PAGE2_SET_STOPWATCH=1,PAGE2_SET_ALARM,PAGE2_SET_EXIT,PAGE2_BACK}SETTING_CHOICE_PAGE2;

typedef struct {
    volatile uint16_t hours;
    volatile uint16_t minutes;
    volatile uint16_t seconds;
    volatile uint16_t milliseconds;
    volatile bool is_running;
    volatile bool is_reset;
} stopwatch_t;

volatile stopwatch_t stopwatch = {0, 0, 0, 0, false, true};
volatile uint32_t stopwatch_ticks = 0;

volatile SYSTEM_STATE currentState=TIME_DISPLAY;
volatile SETTING_CHOICE_PAGE1 currentSettingPage1=SET_TIME;
volatile SETTING_CHOICE_PAGE2 currentSettingPage2=PAGE2_SET_STOPWATCH;
volatile int currentMenuPage = 1;
volatile TIME_FORM currentForm=_24h;
volatile int keyGet=0;
volatile DL_FLASHCTL_COMMAND_STATUS gCmdStatus;
volatile const int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
uint16_t default_year=0;
uint8_t default_month=0;
uint8_t default_day=0;

struct dateStruct{
	volatile int16_t year;
	volatile int16_t month;
	volatile int16_t day;
	volatile int16_t weekday;
}date;

struct timeStruct{
	volatile int16_t hour;
	volatile int16_t minute;
	volatile int16_t second;
}time;

inline void delay_ms(uint32_t ms){
    if(ms == 0) return;
    delay_cycles((CPUCLK_FREQ / 1000) * ms);
}

inline void delay_time(int t){
	if(t>0)delay_cycles(KICKTIME*t);
}

int calculateWeekday(int y, int m, int d) {
    if (m < 3) { m += 12; y -= 1; }
    int c = y / 100;
    int y2 = y % 100;
    int w = (y2 + y2/4 + c/4 - 2*c + 26*(m+1)/10 + d - 1) % 7;
    if (w < 0) w += 7;
    w = (w + 6) % 7;
    return w+1;
}

int keyToDigit(int key) {
    switch(key) {
        case 1: return 1;
        case 2: return 2;
        case 3: return 3;
        case 5: return 4;
        case 6: return 5;
        case 7: return 6;
        case 9: return 7;
        case 10: return 8;
        case 11: return 9;
        case 14: return 0;
        default: return -1;
    }
}

void updateDate(void){
	date.day++;
	date.weekday=(date.weekday+1)%7;
	uint8_t feb=28;
	if((date.year%4==0&&date.year%100!=0)||(date.year%400==0))feb=29;
	uint8_t daysInMonth[]={31,feb,31,30,31,30,31,31,30,31,30,31};
	if(date.day>daysInMonth[date.month-1]){
		date.day=1;
		date.month++;
		if(date.month>12){
			date.month=1;
			date.year++;
		}
	}
}

void updateTime(void){
	time.second++;
	if(time.second>=60){
		time.second=0;
		time.minute++;
		if(time.minute>=60){
			time.minute=0;
			time.hour++;
			if(time.hour>=24){
				time.hour=0;
				updateDate();
			}
		}
	}
}

void updateStopwatch(void) {
    if (!stopwatch.is_running) return;
    stopwatch_ticks++;
    stopwatch.milliseconds++;
    if (stopwatch.milliseconds >= 1000) {
        stopwatch.milliseconds = 0;
        stopwatch.seconds++;
        if (stopwatch.seconds >= 60) {
            stopwatch.seconds = 0;
            stopwatch.minutes++;
            if (stopwatch.minutes >= 60) {
                stopwatch.minutes = 0;
                stopwatch.hours++;
                if (stopwatch.hours >= 99) {
                    stopwatch.hours = 99;
                    stopwatch.is_running = false;
                }
            }
        }
    }
}

void check_alarms(void) {
    for(int i = 0; i < 3; i++) {
        alarm_t* alarm = &alarms[i];
        
        if(alarm->enabled &&
           alarm->hour == time.hour &&
           alarm->minute == time.minute &&
           alarm->second == time.second) {

            OLED_Clear();
            OLED_ShowString(40, 2, "ALARM!");
            OLED_ShowString(30, 3, "Alarm ");
            OLED_ShowChar(66, 3, '1' + i);
            OLED_ShowString(20, 4, "Press any key");
            OLED_ShowString(20, 5, "to stop");
            
            switch(alarm->music_id) {
							case 1:playMusic(Haruhikage,60);	break;
							case 2:playMusic(FunkyStar,75);	break;
							default:playMusic(Haruhikage,60);	break;
            }
            OLED_Clear();
            break;
        }
    }
}

void display_time(int h,int m,int s){
	if(currentForm == _12h) {
		int display_h = (h % 12 == 0) ? 12 : (h % 12);
		OLED_ShowChar(40,0,display_h/10+'0');
		OLED_ShowChar(46,0,display_h%10+'0');
		OLED_ShowChar(58,0,m/10+'0');
		OLED_ShowChar(64,0,m%10+'0');
		OLED_ShowChar(76,0,s/10+'0');
		OLED_ShowChar(82,0,s%10+'0');
		OLED_ShowChar(88,0,(h >= 12) ? 'P' : 'A');
		OLED_ShowChar(94,0,'M');
	} else {
		OLED_ShowChar(40,0,h/10+'0');
		OLED_ShowChar(46,0,h%10+'0');
		OLED_ShowChar(58,0,m/10+'0');
		OLED_ShowChar(64,0,m%10+'0');
		OLED_ShowChar(76,0,s/10+'0');
		OLED_ShowChar(82,0,s%10+'0');
	}
}

void display_day(int y,int m,int d){
	OLED_ShowChar(35,2,y/1000+'0');
	OLED_ShowChar(41,2,y/100%10+'0');
	OLED_ShowChar(47,2,y/10%10+'0');
	OLED_ShowChar(53,2,y%10+'0');
	OLED_ShowChar(65,2,m/10+'0');
	OLED_ShowChar(71,2,m%10+'0');
	OLED_ShowChar(83,2,d/10+'0');
	OLED_ShowChar(89,2,d%10+'0');
}

void display_weekday(int w){
	const char*wd[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
	OLED_ShowString(50,4,wd[w]);
}

void display_stopwatch(void) {
    OLED_Clear();
    OLED_ShowString(0,0,"STOPWATCH:");
    OLED_ShowChar(0,2,stopwatch.hours/10+'0');
    OLED_ShowChar(6,2,stopwatch.hours%10+'0');
    OLED_ShowChar(12,2,':');
    OLED_ShowChar(18,2,stopwatch.minutes/10+'0');
    OLED_ShowChar(24,2,stopwatch.minutes%10+'0');
    OLED_ShowChar(30,2,':');
    OLED_ShowChar(36,2,stopwatch.seconds/10+'0');
    OLED_ShowChar(42,2,stopwatch.seconds%10+'0');
    OLED_ShowChar(48,2,'.');
    OLED_ShowChar(54,2,stopwatch.milliseconds/100+'0');
    OLED_ShowChar(60,2,(stopwatch.milliseconds%100)/10+'0');
    OLED_ShowChar(66,2,stopwatch.milliseconds%10+'0');
    if (stopwatch.is_running) {
        OLED_ShowString(0,4,"STATUS: RUNNING");
        OLED_ShowString(0,6,"1:Start/Stop 2:Reset");
        OLED_ShowString(0,7,"#:Exit");
    } else if (stopwatch.is_reset) {
        OLED_ShowString(0,4,"STATUS: RESET");
        OLED_ShowString(0,6,"1:Start  #:Exit");
    } else {
        OLED_ShowString(0,4,"STATUS: STOPPED");
        OLED_ShowString(0,6,"1:Resume 2:Reset");
        OLED_ShowString(0,7,"#:Exit");
    }
}

void display_alarm_menu(ALARM_NUMBER alarm_num) {
    OLED_Clear();
    OLED_ShowString(0,0,"ALARM SETTING");
    OLED_ShowString(0,1,"Alarm ");
    OLED_ShowChar(38,1,'0' + alarm_num);
    
    alarm_t* alarm = &alarms[alarm_num-1];
    
    OLED_ShowString(0,2,"Time: ");
    OLED_ShowChar(36,2,alarm->hour/10+'0');
    OLED_ShowChar(42,2,alarm->hour%10+'0');
    OLED_ShowChar(48,2,':');
    OLED_ShowChar(54,2,alarm->minute/10+'0');
    OLED_ShowChar(60,2,alarm->minute%10+'0');
    OLED_ShowChar(66,2,':');
    OLED_ShowChar(72,2,alarm->second/10+'0');
    OLED_ShowChar(78,2,alarm->second%10+'0');
    
    OLED_ShowString(0,3,"Status: ");
    OLED_ShowString(42,3,alarm->enabled ? "ENABLED" : "DISABLED");
    
    OLED_ShowString(0,4,"1:Change Alarm");
    OLED_ShowString(0,5,"2:Set Time");
    OLED_ShowString(0,6,"3:Toggle Enable");
    OLED_ShowString(0,7,"#:Back  6:Save");
}

void display_select_alarm(ALARM_NUMBER selected) {
    OLED_Clear();
    OLED_ShowString(0,0,"SELECT ALARM");
    
    for(int i = 0; i < 3; i++) {
        alarm_t* alarm = &alarms[i];
        
        OLED_ShowChar(0, 2+i*2, selected == (i+1) ? '>' : ' ');
        OLED_ShowString(6, 2+i*2, "Alarm ");
        OLED_ShowChar(42, 2+i*2, '1' + i);
        OLED_ShowChar(54, 2+i*2, alarm->hour/10+'0');
        OLED_ShowChar(60, 2+i*2, alarm->hour%10+'0');
        OLED_ShowChar(66, 2+i*2, ':');
        OLED_ShowChar(72, 2+i*2, alarm->minute/10+'0');
        OLED_ShowChar(78, 2+i*2, alarm->minute%10+'0');
        OLED_ShowChar(84, 2+i*2, ':');
        OLED_ShowChar(90, 2+i*2, alarm->second/10+'0');
        OLED_ShowChar(96, 2+i*2, alarm->second%10+'0');
        OLED_ShowString(6, 3+i*2, alarm->enabled ? "[ON] " : "[OFF]");
        OLED_ShowString(36, 3+i*2, music_names[alarm->music_id-1]);
    }
    
    OLED_ShowString(0, 8, "#:Back  6:Select");
}

void set_alarm_time(ALARM_NUMBER alarm_num) {
    OLED_Clear();
    alarm_t* alarm = &alarms[alarm_num-1];
    
    int nums[6] = {0};
    int num_size = 0;
    int key;
    int key_past = 0;
    int temp_hour = 0, temp_min = 0, temp_sec = 0;
    
    OLED_ShowString(0, 1, "Set Alarm Time");
    OLED_ShowString(0, 2, "Format: HHMMSS");
    OLED_ShowString(1, 3, "\"#\" to exit B:DEL");
    delay_ms(200);
    while(num_size < 6) {
        OLED_ShowChar(40, 5, temp_hour/10+'0');
        OLED_ShowChar(46, 5, temp_hour%10+'0');
        OLED_ShowChar(52, 5, ':');
        OLED_ShowChar(58, 5, temp_min/10+'0');
        OLED_ShowChar(64, 5, temp_min%10+'0');
        OLED_ShowChar(70, 5, ':');
        OLED_ShowChar(76, 5, temp_sec/10+'0');
        OLED_ShowChar(82, 5, temp_sec%10+'0');
        
        key = KeySCInput();
        if(key == 15) {
            currentState = ALARM_MENU;
            return;
        }
        
        if(key != 0 && key != key_past) {
            if(key == 8) {
                if(num_size > 0) {
                    num_size--;
                    nums[num_size] = 0;
                }
            }
            else if(key_past == 0 && key != 8) {
                int digit = keyToDigit(key);
                if(digit != -1 && num_size < 6) {
                    nums[num_size] = digit;
                    num_size++;
                }
                key_past = key;
            }
        }
        
        temp_hour = nums[0]*10 + nums[1];
        temp_min = nums[2]*10 + nums[3];
        temp_sec = nums[4]*10 + nums[5];
        
        if(temp_hour > 23) temp_hour = 23;
        if(temp_min > 59) temp_min = 59;
        if(temp_sec > 59) temp_sec = 59;
        
        delay_ms(200);
        key_past = key;
    }
    
    alarm->hour = temp_hour;
    alarm->minute = temp_min;
    alarm->second = temp_sec;
    
    OLED_Clear();
    OLED_ShowString(0, 2, "TIME SET!");
    delay_ms(1000);
}

void set_alarm_music(ALARM_NUMBER alarm_num) {
    OLED_Clear();
    alarm_t* alarm = &alarms[alarm_num-1];
    int music_choice = alarm->music_id;
    
    OLED_ShowString(0, 0, "SELECT MUSIC");
    
    while(1) {
        for(int i = 0; i < 3; i++) {
            OLED_ShowChar(0, 2+i, music_choice == (i+1) ? '>' : ' ');
            OLED_ShowString(6, 2+i, music_names[i]);
        }
        
        int key = KeySCInput();
        if(key == 2 || key == 5) {
            music_choice = (music_choice > 1) ? music_choice-1 : 3;
            delay_ms(200);
        }
        else if(key == 7 || key == 10) {
            music_choice = (music_choice < 3) ? music_choice+1 : 1;
            delay_ms(200);
        }
        else if(key == 6) {
            alarm->music_id = music_choice;
            OLED_Clear();
            OLED_ShowString(0, 2, "MUSIC SET!");
            delay_ms(1000);
            break;
        }
        else if(key == 15) {
            break;
        }
    }
}

void displayMenuPage1(void){
	OLED_Clear();
	OLED_ShowString(0,0,"SETTING [1/2]");
	OLED_ShowString(0,1,currentSettingPage1==SET_TIME?">":" ");
	OLED_ShowString(8,1,"SET TIME");
	OLED_ShowString(0,2,currentSettingPage1==SET_DATE?">":" ");
	OLED_ShowString(8,2,"SET DATE");
	OLED_ShowString(0,3,currentSettingPage1==SET_APM?">":" ");
	OLED_ShowString(8,3,"SET APM");
	OLED_ShowString(0,4,currentSettingPage1==SET_SAVE?">":" ");
	OLED_ShowString(8,4,"SAVE");
	OLED_ShowString(0,5,currentSettingPage1==SET_PAGE2?">":" ");
	OLED_ShowString(8,5,"NEXT PAGE >>");
	OLED_ShowString(0,7,"B:EXIT MENU");
}

void displayMenuPage2(void){
	OLED_Clear();
	OLED_ShowString(0,0,"SETTING [2/2]");
	OLED_ShowString(0,1,currentSettingPage2==PAGE2_SET_STOPWATCH?">":" ");
	OLED_ShowString(8,1,"STOPWATCH");
	OLED_ShowString(0,2,currentSettingPage2==PAGE2_SET_ALARM?">":" ");
	OLED_ShowString(8,2,"ALARM SETTING");
	OLED_ShowString(0,3,currentSettingPage2==PAGE2_SET_EXIT?">":" ");
	OLED_ShowString(8,3,"EXIT");
	OLED_ShowString(0,4,currentSettingPage2==PAGE2_BACK?">":" ");
	OLED_ShowString(8,4,"<< PREV PAGE");
	OLED_ShowString(0,7,"B:EXIT MENU");
}

void displayTimeForm(void) {
	OLED_Clear();
	OLED_ShowString(0,0,"TIME FORMAT");
	OLED_ShowString(0,2,currentForm==_24h?">":" ");
	OLED_ShowString(8,2,"24-HOUR");
	OLED_ShowString(0,3,currentForm==_12h?">":" ");
	OLED_ShowString(8,3,"12-HOUR");
}

void switchMenuPage(void) {
    if (currentMenuPage == 1) {
        currentMenuPage = 2;
        currentSettingPage2 = PAGE2_SET_STOPWATCH;
        currentState = MENU_PAGE2;
        displayMenuPage2();
    } else {
        currentMenuPage = 1;
        currentSettingPage1 = SET_TIME;
        currentState = MENU_PAGE1;
        displayMenuPage1();
    }
}

void setTime(void){
	bool changed=true;
    OLED_Clear();
    int nums[6] = {0};
    int num_size = 0;
    int key;
    int key_past = 0;
    int temp_hour = 0, temp_min = 0, temp_sec = 0;
    OLED_ShowString(0, 1, "HHMMSS");
	OLED_ShowString(0, 2, "#:EXIT B:DEL 6:OK");
    while(num_size < 6){
        display_time(temp_hour, temp_min, temp_sec);
        
        for(int i = 0; i < 6; i++) {
            if(i < num_size) {
                OLED_ShowChar(40 + i*8, 3, nums[i] + '0');
            } else {
                OLED_ShowChar(40 + i*8, 3, '_');
            }
        }
        
        key = KeySCInput();
		if(key==15){
			currentState = currentMenuPage == 1 ? MENU_PAGE1 : MENU_PAGE2;
			changed=false;
			break;
		}
        if(key != 0 && key != key_past){
            if(key == 8){
                if(num_size > 0){
                    num_size--;
                    nums[num_size] = 0;
                }
            }
            else if(key_past == 0 && key != 8){
                int digit = keyToDigit(key);
                if(digit != -1 && num_size < 6){
                    nums[num_size] = digit;
                    num_size++;
                }
                key_past = key;
            }
        }
        temp_hour = nums[0]*10 + nums[1];
        temp_min = nums[2]*10 + nums[3];
        temp_sec = nums[4]*10 + nums[5];
        
        if(temp_hour >= 24) {
            OLED_ShowString(0, 5, "HOUR<24!   ");
            if(nums[0] >= 2) {
                nums[0] = 2;
                nums[1] = 3;
                temp_hour = 23;
            }
        }
        if(temp_min >= 60) {
            OLED_ShowString(0, 6, "MIN<60!    ");
            if(nums[2] >= 6) {
                nums[2] = 5;
                nums[3] = 9;
                temp_min = 59;
            }
        }
        if(temp_sec >= 60) {
            OLED_ShowString(0, 7, "SEC<60!    ");
            if(nums[4] >= 6) {
                nums[4] = 5;
                nums[5] = 9;
                temp_sec = 59;
            }
        }
        
        delay_ms(200);
        key_past = key;
    }
    
    if(temp_hour >= 24) temp_hour = 23;
    if(temp_min >= 60) temp_min = 59;
    if(temp_sec >= 60) temp_sec = 59;
    
    time.hour = temp_hour;
    time.minute = temp_min;
    time.second = temp_sec;
    
    OLED_Clear();
	if(changed){
        OLED_ShowString(0, 2, "SUCCESS");
        delay_ms(1000);
        OLED_Clear();
	}
    
    if(currentMenuPage == 1) {
        displayMenuPage1();
    } else {
        displayMenuPage2();
    }
}

void setDate(void){
    OLED_Clear();
	bool changed=true;
    int nums[8] = {0};
    int num_size = 0;
    int key;
    int key_past = 0;
    int temp_year = 0, temp_month = 0, temp_day = 0;
    OLED_ShowString(0, 2, "YYYYMMDD");
	OLED_ShowString(0, 3, "#:EXIT B:DEL 6:OK");
    while(num_size < 8){
        display_day(temp_year, temp_month, temp_day);
        
        for(int i = 0; i < 8; i++) {
            if(i < num_size) {
                OLED_ShowChar(40 + i*6, 4, nums[i] + '0');
            } else {
                OLED_ShowChar(40 + i*6, 4, '_');
            }
        }
        
        key = KeySCInput();
		if(key==15){
			currentState = currentMenuPage == 1 ? MENU_PAGE1 : MENU_PAGE2;
			changed=false;
			break;
		}
        if(key != 0 && key != key_past){
            if(key == 8){
                if(num_size > 0){
                    num_size--;
                    nums[num_size] = 0;
                }
            }
            else if(key_past == 0 && key != 8){
                int digit = keyToDigit(key);
                if(digit != -1 && num_size < 8){
                    nums[num_size] = digit;
                    num_size++;
                }
                key_past = key;
            }
        }
        temp_year = nums[0]*1000 + nums[1]*100 + nums[2]*10 + nums[3];
        temp_month = nums[4]*10 + nums[5];
        temp_day = nums[6]*10 + nums[7];
        
        if(temp_month > 12) {
            OLED_ShowString(0, 6, "MONTH<=12!");
            if(nums[4] > 1) {
                nums[4] = 1;
                nums[5] = 2;
                temp_month = 12;
            }
        }
        
        delay_ms(200);
        key_past = key;
    }
    
    if(temp_month < 1) temp_month = 1;
    if(temp_month > 12) temp_month = 12;
    
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if((temp_year % 4 == 0 && temp_year % 100 != 0) || (temp_year % 400 == 0)) {
        days_in_month[1] = 29;
    }
    
    if(temp_day < 1) temp_day = 1;
    if(temp_day > days_in_month[temp_month-1]) {
        temp_day = days_in_month[temp_month-1];
    }
    
    date.year = temp_year;
    date.month = temp_month;
    date.day = temp_day;
    date.weekday = calculateWeekday(date.year, date.month, date.day);
    
    OLED_Clear();
	if(changed){
        OLED_ShowString(0, 2, "SUCCESS");
        delay_ms(1000);
        OLED_Clear();
	}
    
    if(currentMenuPage == 1) {
        displayMenuPage1();
    } else {
        displayMenuPage2();
    }
}

void saveTime(void){
	OLED_Clear();
	OLED_ShowString(0,2,"SAVING...");
	delay_time(300);
	uint32_t t1 = time.hour << 16 | time.minute << 8 | time.second;
	uint32_t t2 = date.year << 16 | date.month << 8 | date.day;
	DL_FlashCTL_unprotectSector(
		FLASHCTL, MAIN_ADDRESS, DL_FLASHCTL_REGION_SELECT_MAIN
	);
	gCmdStatus = DL_FlashCTL_eraseMemoryFromRAM(
		FLASHCTL, MAIN_ADDRESS, DL_FLASHCTL_COMMAND_SIZE_SECTOR
	);
	DL_FlashCTL_unprotectSector(
		FLASHCTL, MAIN_ADDRESS, DL_FLASHCTL_REGION_SELECT_MAIN
	);
	DL_FlashCTL_programMemoryFromRAM32WithECCGenerated(
		FLASHCTL, MAIN_ADDRESS, &t1
	);
	DL_FlashCTL_unprotectSector(
		FLASHCTL, MAIN_ADDRESS + 0x1000, DL_FLASHCTL_REGION_SELECT_MAIN
	);
	gCmdStatus = DL_FlashCTL_eraseMemoryFromRAM(
		FLASHCTL, MAIN_ADDRESS + 0x1000, DL_FLASHCTL_COMMAND_SIZE_SECTOR
	);
	DL_FlashCTL_unprotectSector(
		FLASHCTL, MAIN_ADDRESS + 0x1000, DL_FLASHCTL_REGION_SELECT_MAIN
	);
	DL_FlashCTL_programMemoryFromRAM32WithECCGenerated(
		FLASHCTL, MAIN_ADDRESS + 0x1000, &t2
	);

	OLED_Clear();
	OLED_ShowString(0,2,"SUCCESS");
	delay_time(500);
	OLED_Clear();
	currentState=MENU_PAGE1;
	currentMenuPage = 1;
	displayMenuPage1();
}

void TIMER_1_INST_IRQHandler(void){
    if(DL_TimerG_getPendingInterrupt(TIMER_1_INST)==DL_TIMER_INTERRUPT_ZERO_EVENT){
        updateStopwatch();
        DL_TimerG_clearInterruptStatus(TIMER_1_INST,DL_TIMER_INTERRUPT_ZERO_EVENT);
    }
}

void autoSave(){
	uint32_t t1 = time.hour << 16 | time.minute << 8 | time.second;
	uint32_t t2 = date.year << 16 | date.month << 8 | date.day;
	DL_FlashCTL_unprotectSector(
		FLASHCTL, MAIN_ADDRESS, DL_FLASHCTL_REGION_SELECT_MAIN
	);
	gCmdStatus = DL_FlashCTL_eraseMemoryFromRAM(
		FLASHCTL, MAIN_ADDRESS, DL_FLASHCTL_COMMAND_SIZE_SECTOR
	);
	DL_FlashCTL_unprotectSector(
		FLASHCTL, MAIN_ADDRESS, DL_FLASHCTL_REGION_SELECT_MAIN
	);
	DL_FlashCTL_programMemoryFromRAM32WithECCGenerated(
		FLASHCTL, MAIN_ADDRESS, &t1
	);
	DL_FlashCTL_unprotectSector(
		FLASHCTL, MAIN_ADDRESS + 0x1000, DL_FLASHCTL_REGION_SELECT_MAIN
	);
	gCmdStatus = DL_FlashCTL_eraseMemoryFromRAM(
		FLASHCTL, MAIN_ADDRESS + 0x1000, DL_FLASHCTL_COMMAND_SIZE_SECTOR
	);
	DL_FlashCTL_unprotectSector(
		FLASHCTL, MAIN_ADDRESS + 0x1000, DL_FLASHCTL_REGION_SELECT_MAIN
	);
	DL_FlashCTL_programMemoryFromRAM32WithECCGenerated(
		FLASHCTL, MAIN_ADDRESS + 0x1000, &t2
	);
}

void TIMER_0_INST_IRQHandler(void){
	autoSave();
	if(DL_TimerG_getPendingInterrupt(TIMER_0_INST)==DL_TIMER_INTERRUPT_ZERO_EVENT){
		updateTime();
		
		if(currentState == TIME_DISPLAY) {
			check_alarms();
		}
		
		if(currentState==TIME_DISPLAY){
			display_day(date.year,date.month,date.day);
			display_time(time.hour,time.minute,time.second);
			display_weekday(date.weekday);
			OLED_ShowString(35,6,"[#]:MENU");
			DL_GPIO_togglePins(LED_L1_PORT,LED_L1_PIN);
		}
		DL_TimerG_clearInterruptStatus(TIMER_0_INST,DL_TIMER_INTERRUPT_ZERO_EVENT);
	}
}

int main(void){
    {
				SYSCFG_DL_init();
        Keyboard_init();
			  BuzzDisabled();
        OLED_Init();
        NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
        NVIC_EnableIRQ(TIMER_1_INST_INT_IRQN);
        time.hour = *(uint8_t *)(MAIN_ADDRESS + 2);
        time.minute = *(uint8_t *)(MAIN_ADDRESS + 1);
        time.second = *(uint8_t *)(MAIN_ADDRESS);
        date.day = *(uint8_t *)(MAIN_ADDRESS + 0x1000);
        date.month = *(uint8_t *)(MAIN_ADDRESS + 0x1001);
        uint8_t year_low = *(uint8_t *)(MAIN_ADDRESS + 0x1002);
        uint8_t year_high = *(uint8_t *)(MAIN_ADDRESS + 0x1003);
        date.year = year_high * 256 + year_low;
        if (date.year == 0xFFFF && date.month == 0xFF && date.day == 0xFF &&
            time.hour == 0xFF && time.minute == 0xFF && time.second == 0xFF) {
            date.year = 0;
            date.month = 0;
            date.day = 0;
            date.weekday = 0;
            time.hour = 0;
            time.minute = 0;
            time.second = 0;
        }
        if (time.hour >= 24) time.hour = 0;
        if (time.minute >= 60) time.minute = 0;
        if (time.second >= 60) time.second = 0;
        if (date.month > 12 || date.month == 0) date.month = 0;
        if (date.day > 31 || date.day == 0) date.day = 0;
        if (date.year > 9999) date.year = 0;
        date.weekday = calculateWeekday(date.year, date.month, date.day);
    
        OLED_Clear();
        display_day(date.year,date.month,date.day);
        display_time(time.hour,time.minute,time.second);
        display_weekday(date.weekday);
        OLED_ShowString(35,6,"[#]:MENU");
    
    }
    
    volatile ALARM_NUMBER selected_alarm = ALARM1;
    
	while(1){
        keyGet=KeySCInput();
        switch(currentState){
        case TIME_DISPLAY:
            if(keyGet==15){
                currentState=MENU_PAGE1;
                currentSettingPage1=SET_TIME;
                currentMenuPage = 1;
                displayMenuPage1();
            }
            break;
        case MENU_PAGE1:
            if(keyGet==2||keyGet==5){
                delay_ms(200);
                if(keyGet==2||keyGet==5){
                    if(currentSettingPage1 == SET_TIME){
                        currentState = MENU_PAGE2;
                        currentMenuPage = 2;
                        currentSettingPage2 = PAGE2_BACK;
                        displayMenuPage2();
                    }else{
                        currentSettingPage1 = (SETTING_CHOICE_PAGE1)(currentSettingPage1 - 1);
                        displayMenuPage1();
                    }
                }
            }else if(keyGet==7||keyGet==10){
                delay_ms(200);
                if(keyGet==7||keyGet==10){
                    if(currentSettingPage1 == SET_PAGE2){
                        currentState = MENU_PAGE2;
                        currentMenuPage = 2;
                        currentSettingPage2 = PAGE2_SET_STOPWATCH;
                        displayMenuPage2();
                    }else{
                        currentSettingPage1 = (SETTING_CHOICE_PAGE1)(currentSettingPage1 + 1);
                        displayMenuPage1();
                    }
                }
            }else if(keyGet==6){
                delay_ms(200);
                if(keyGet==6){
                    if(currentSettingPage1==SET_TIME){
                        setTime();
                    }
                    else if(currentSettingPage1==SET_DATE){
                        setDate();
                    }
                    else if(currentSettingPage1==SET_APM){
                        currentState=SET_FORM;
                        displayTimeForm();
                    }
                    else if(currentSettingPage1==SET_SAVE){
                        saveTime();
                    }
                    else if(currentSettingPage1==SET_PAGE2){
                        switchMenuPage();
                    }
                }
            }else if(keyGet==8){
                currentState=TIME_DISPLAY;
                OLED_Clear();
            }
            break;
        case MENU_PAGE2:
            if(keyGet==2||keyGet==5){
                delay_ms(200);
                if(keyGet==2||keyGet==5){
                    if(currentSettingPage2 == PAGE2_SET_STOPWATCH){
                        currentState = MENU_PAGE1;
                        currentMenuPage = 1;
                        currentSettingPage1 = SET_PAGE2;
                        displayMenuPage1();
                    }else{
                        currentSettingPage2 = (SETTING_CHOICE_PAGE2)(currentSettingPage2 - 1);
                        displayMenuPage2();
                    }
                }
            }else if(keyGet==7||keyGet==10){
                delay_ms(200);
                if(keyGet==7||keyGet==10){
                    if(currentSettingPage2 == PAGE2_BACK){
                        currentState = MENU_PAGE1;
                        currentMenuPage = 1;
                        currentSettingPage1 = SET_TIME;
                        displayMenuPage1();
                    }else{
                        currentSettingPage2 = (SETTING_CHOICE_PAGE2)(currentSettingPage2 + 1);
                        displayMenuPage2();
                    }
                }
            }else if(keyGet==6){
                delay_ms(200);
                if(keyGet==6){
                    if(currentSettingPage2==PAGE2_SET_STOPWATCH){
                        currentState = STOPWATCH;
                        display_stopwatch();
                    }
                    else if(currentSettingPage2==PAGE2_SET_ALARM){
                        currentState = ALARM_MENU;
                        display_alarm_menu(selected_alarm);
                    }
                    else if(currentSettingPage2==PAGE2_SET_EXIT){
                        currentState=TIME_DISPLAY;
                        OLED_Clear();
                    }
                    else if(currentSettingPage2==PAGE2_BACK){
                        switchMenuPage();
                    }
                }
            }else if(keyGet==15){
                currentState=TIME_DISPLAY;
                OLED_Clear();
            }
            break;
        case SET_FORM:
            if(keyGet==2||keyGet==5||keyGet==7||keyGet==10){
                delay_ms(200);
                if(keyGet==2||keyGet==5||keyGet==7||keyGet==10){
                    currentForm = (currentForm == _24h) ? _12h : _24h;
                    displayTimeForm();
                }
            }else if(keyGet==6){
                delay_ms(200);
                if(keyGet==6){
                    if(currentMenuPage == 1) {
                        currentState = MENU_PAGE1;
                        displayMenuPage1();
                    } else {
                        currentState = MENU_PAGE2;
                        displayMenuPage2();
                    }
                }
            }else if(keyGet==15){
                if(currentMenuPage == 1) {
                    currentState = MENU_PAGE1;
                    displayMenuPage1();
                } else {
                    currentState = MENU_PAGE2;
                    displayMenuPage2();
                }
            }
            break;
        case STOPWATCH:
            if(keyGet == 1){
                if(stopwatch.is_reset) {
                    stopwatch.is_running = true;
                    stopwatch.is_reset = false;
                    DL_TimerG_startCounter(TIMER_1_INST);
                } else {
                    stopwatch.is_running = !stopwatch.is_running;
                    if(stopwatch.is_running) {
                        DL_TimerG_startCounter(TIMER_1_INST);
                    } else {
                        DL_TimerG_stopCounter(TIMER_1_INST);
                    }
                }
                display_stopwatch();
                delay_ms(300);
            }
            else if(keyGet == 2){
                stopwatch.hours = 0;
                stopwatch.minutes = 0;
                stopwatch.seconds = 0;
                stopwatch.milliseconds = 0;
                stopwatch.is_running = false;
                stopwatch.is_reset = true;
                stopwatch_ticks = 0;
                DL_TimerG_stopCounter(TIMER_1_INST);
                DL_TimerG_setLoadValue(TIMER_1_INST, 32768);
                display_stopwatch();
                delay_ms(300);
            }
            else if(keyGet == 15){
                if(stopwatch.is_running) {
                    DL_TimerG_stopCounter(TIMER_1_INST);
                }
                currentState = MENU_PAGE2;
                displayMenuPage2();
            }
            
            if(currentState == STOPWATCH) {
                display_stopwatch();
            }
            break;
        case ALARM_MENU:
            if(keyGet == 1) {
                currentState = ALARM_SETTING_STATE;
                display_select_alarm(selected_alarm);
                delay_ms(200);
            }
            else if(keyGet == 2) {
                set_alarm_time(selected_alarm);
                display_alarm_menu(selected_alarm);
            }
            else if(keyGet == 3) {
                alarms[selected_alarm-1].enabled = !alarms[selected_alarm-1].enabled;
                display_alarm_menu(selected_alarm);
                delay_ms(300);
            }
            else if(keyGet == 4) {
                set_alarm_music(selected_alarm);
                display_alarm_menu(selected_alarm);
            }
            else if(keyGet == 15) {
                currentState = MENU_PAGE2;
                displayMenuPage2();
            }
            else if(keyGet == 6) {
                currentState = MENU_PAGE2;
                displayMenuPage2();
            }
            break;
        case ALARM_SETTING_STATE:
            if(keyGet >= 1 && keyGet <= 3) {
                selected_alarm = keyGet;
                currentState = ALARM_MENU;
                display_alarm_menu(selected_alarm);
                delay_ms(200);
            }
            else if(keyGet == 15) {
                currentState = ALARM_MENU;
                display_alarm_menu(selected_alarm);
            }
            break;
        default:break;
        }
    }
}