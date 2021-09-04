#include "app.h"
#include "sun.h"

static uint16_t dayNum, sunrise_time, moonrise_time, sunset_time, moonset_time;


static int update_day_number(){
	/* Okay, so to calculate the pretty times from the data, first
	 * we must get the date. the array is arranged by taking the number of
	 * days since a common date.*/ 
	hal_rtc_timedate_t td;
	uint16_t dayNumber;
	uint16_t year;
	uint8_t month;
	int i;
	uint8_t found = 0;
	hal_rtc_get(&td);
	year = first_year;
	dayNumber = 0;
	while(year++ < td.year){
		found = 0;
		for (i=0; i<NUM_LEAP_YEARS; i++){
			if (year == leap_years[i]){
				dayNumber += 366;
				found = 1;
			}
		}
		if (found == 0){
			dayNumber += 365;
			/* Otherwise, we just add 365 for number of days */ 
		}
	}
	month = 0;
	i = 0;
	while (month++ < td.month){
		dayNumber += month_days[i++];
	}
	/* Now we account for if this was a leap year... */ 
	for (i=0; i<NUM_LEAP_YEARS; i++){
		if (td.year == leap_years[i]){
			/* Then this is infact a leap year */ 
			if (td.month > 2){
				dayNumber++;
			}
		}
	}
	dayNumber += td.dom;
	dayNum = dayNumber;
}

static void update_moonrise(void){
	int dayNumber = dayNum;
	uint16_t time;
	uint8_t minutes = moonrise_times[dayNumber];
	/* Now moonrise is number of tenth hours (6 minutes) past 0000
	 * So if we divide by 10, we get the hours.
	 * And if we % by 10, and multiply by 6, we get minutes*/ 
	time = (minutes / 10) * 100;  /* 100 will shift up two digits. */ 
	time += ((minutes % 10) * 6);
	moonrise_time = time;

}
static void update_moonset(void){
	int dayNumber = dayNum;
	uint16_t time;
	uint8_t minutes = moonset_times[dayNumber];
	/* Now moonrise is number of tenth hours (6 minutes) past 0000
	 * So if we divide by 10, we get the hours.
	 * And if we % by 10, and multiply by 6, we get minutes*/ 
	time = (minutes / 10) * 100;  /* 100 will shift up two digits. */ 
	time += ((minutes % 10) * 6);
	moonset_time = time;

}
static void update_sunset(void){
	int dayNumber = dayNum;
	uint16_t time;
	uint8_t minutes = sunset_times[dayNumber];
	/* Now sunset is number of minutes past 1645. 
	 * Number of hours is minutes / 60.
	 * minutes is the remainder.
	 * */ 
	time = 0;
	while (minutes > 60){
		minutes -= 60; /* Remove the hour */ 
		time += 100; /* Add an hour. */ 
		/* Save some cycles doing addition instead of division.  */ 
	}
	time += 1645;  /* Add the starting time of the dates*/ 
	time += minutes;  /* Add the remaining minutes */ 
	sunset_time = time;

}
static void update_sunrise(void){
	int dayNumber = dayNum;
	uint16_t time;
	uint8_t minutes = sunrise_times[dayNumber];
	
	/* Now sunrise is number of minutes past 0500. 
	 * Number of hours is minutes / 60.
	 * minutes is the remainder.
	 * */ 

	time = 0;
	while (minutes > 60){
		minutes -= 60; /* Remove the hour */ 
		time += 100; /* Add an hour. */ 
	}
	
	time += 500;  /* Add the starting time of the dates*/ 
	time += minutes;  /* Add the remaining minutes */ 
	sunrise_time = time;

}

static void update_sunrise_times(void){
	update_day_number();
	update_moonrise();
	update_moonset();
	update_sunrise();
	update_sunset();
}

static void sunrise_exit(void *ud) {
	PRIV(app_current)->st.item_current = 0;
	app_exit();
}
static void show_moonset(svc_menu_state_t *state, svc_menu_item_unknown_t *item, void *userdata){

	svc_lcd_puti(0,4,moonset_time);
}
static void show_moonrise(svc_menu_state_t *state, svc_menu_item_unknown_t *item, void *userdata){
	svc_lcd_puti(0,4,moonrise_time);
}
static void show_sunset(svc_menu_state_t *state, svc_menu_item_unknown_t *item, void *userdata){
	svc_lcd_puti(0,4,sunset_time);
}
static void show_sunrise(svc_menu_state_t *state, svc_menu_item_unknown_t *item, void *userdata){
	svc_lcd_puti(0,4, sunrise_time);
}
static void show_moon_phase(svc_menu_state_t *state, svc_menu_item_unknown_t *item, void *userdata){
	svc_lcd_puti(0,4, sunrise_time);
}
static const svc_menu_item_text_t menu_item_sunrise = {
	.type = SVC_MENU_ITEM_T_TEXT,
	.text = "    sr",
	.handler_draw = show_sunrise
};
static const svc_menu_item_text_t menu_item_sunset = {
	.type = SVC_MENU_ITEM_T_TEXT,
	.text = "    ss",
	.handler_draw = show_sunset
};
static const svc_menu_item_text_t menu_item_moonrise = {
	.type = SVC_MENU_ITEM_T_TEXT,
	.text = "    mr",
	.handler_draw = show_moonrise
};
static const svc_menu_item_text_t menu_item_moonset = {
	.type = SVC_MENU_ITEM_T_TEXT,
	.text = "    ms",
	.handler_draw = show_moonset
};
static const svc_menu_item_text_t menu_item_up = {
	.type = SVC_MENU_ITEM_T_TEXT,
	.text = "----up",
	.handler = sunrise_exit
};


static const svc_menu_item_text_t *menu_items[] = {
	(void*)&menu_item_sunrise,
	(void*)&menu_item_sunset,
	(void*)&menu_item_moonrise,
	(void*)&menu_item_moonset,
	(void*)&menu_item_up,
};

static const svc_menu_t menu = {
	.n_items = ARRAY_SIZE(menu_items),
	.items = (void*)menu_items,
	.item_up = (void*)&menu_item_up,
	.header = "ss",
	.header_pos = 8
};


static void main(uint8_t view, const app_t *app, svc_main_proc_event_t event) {
	update_sunrise_times();
	svc_menu_run(&menu, &(PRIV(app)->st), event);
}

static app_view_t views[] = {
	{
		.main = main
	}
};

static priv_t priv = {0};

const app_t app_app_sunrise = {
	.n_views = 2,
	.priv = (app_priv_t*)(&priv),
	.views = views
};
