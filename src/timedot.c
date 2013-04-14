/*

  TimeDot watch
  (c) 2013 by Lukas Zeller, plan44.ch

 */

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x9B, 0x3A, 0xAB, 0x35, 0xB0, 0x92, 0x48, 0xE9, 0x80, 0x02, 0xDC, 0x8F, 0x57, 0x2C, 0x50, 0xBC }
PBL_APP_INFO(MY_UUID,
             "TimeDot", "plan44.ch",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

// Variables
// - the window
Window window;
// - layers
Layer second_display_layer;
TextLayer time_display_textlayer;
TextLayer weekday_display_textlayer;
TextLayer date_display_textlayer;

// - internal variables
int currentSecond;
int timeTextValid;
int dateTextValid;
// - the ziit text string
#define TIME_STR_BUFFER_BYTES 20
char time_str_buffer[TIME_STR_BUFFER_BYTES];
char weekday_str_buffer[TIME_STR_BUFFER_BYTES];
char date_str_buffer[TIME_STR_BUFFER_BYTES];

// Geometry constants
#define TIMETEXT_LAYER_MINMARGIN 13
#define TIMETEXT_LAYER_EXTRAMARGIN_H 36
#define TIMETEXT_FONT FONT_KEY_GOTHAM_42_MEDIUM_NUMBERS

#define AUXTEXT_LAYER_EXTRAMARGIN_H 17
#define AUXTEXT_H 20
#define AUXTEXT_FONT FONT_KEY_GOTHIC_18_BOLD

#define DOT_MARGIN 8
#define DOT_RADIUS 4


void second_display_layer_callback(Layer *me, GContext* ctx)
{
  // unsigned int angle = currentSecond * 6;
  int32_t hexAngle = (int32_t)currentSecond * 1092;
  GPoint center = grect_center_point(&me->frame);
  int32_t r = center.x-DOT_MARGIN;
  GPoint dotPos;
  dotPos.x = center.x + sin_lookup(hexAngle)*r/TRIG_MAX_RATIO;
  dotPos.y = center.y - cos_lookup(hexAngle)*r/TRIG_MAX_RATIO;
  graphics_context_set_fill_color(ctx, GColorWhite);
  // paint the dot
  graphics_fill_circle(ctx, dotPos, DOT_RADIUS);
}


// define to show specific string instead of time for debugging layout
//#define DEBUG_TIME_STRING "23:33" // fattest time

// handle second tick
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *event)
{
  currentSecond = event->tick_time->tm_sec;
  if (currentSecond==0) timeTextValid = 0; // new minute, needs new text
  if (!timeTextValid) {
    // update time text
    if (event->tick_time->tm_min==0 && event->tick_time->tm_hour==0) dateTextValid = 0;
    #ifdef DEBUG_TIME_STRING
    text_layer_set_text(&time_display_textlayer, DEBUG_TIME_STRING);
    #else
    if (clock_is_24h_style())
      string_format_time(time_str_buffer, TIME_STR_BUFFER_BYTES, "%H:%M", event->tick_time);
    else
      string_format_time(time_str_buffer, TIME_STR_BUFFER_BYTES, "%I:%M", event->tick_time);
    text_layer_set_text(&time_display_textlayer, time_str_buffer);
    #endif
    timeTextValid = 1; // now valid
  }
  if (!dateTextValid) {
    // update weekday text
    string_format_time(weekday_str_buffer, TIME_STR_BUFFER_BYTES, "%A", event->tick_time);
    text_layer_set_text(&weekday_display_textlayer, weekday_str_buffer);
    // update date text
    string_format_time(date_str_buffer, TIME_STR_BUFFER_BYTES, "%Y-%m-%d", event->tick_time);
    text_layer_set_text(&date_display_textlayer, date_str_buffer);
  }
  // anyway, update second
  layer_mark_dirty(&second_display_layer);
}



void handle_init(AppContextRef ctx)
{
  (void)ctx;

  // the window
  window_init(&window, "TimeDot");
  window_set_background_color(&window, GColorBlack);
  window_stack_push(&window, true /* Animated */);

  // init the engine
  timeTextValid = 0; // time text not valid
  dateTextValid = 0; // date text not valid
  PblTm t;
  get_time(&t);
  currentSecond = t.tm_sec; // start with correct second position

  // the text layer for displaying the Ziit time text in big font
  GRect tf = window.layer.frame;
  // - vertically centered rectangle for time
  tf.origin.y += (tf.size.h-(tf.size.w-2*TIMETEXT_LAYER_MINMARGIN)) / 2 + TIMETEXT_LAYER_EXTRAMARGIN_H;
  tf.size.h = tf.size.w - TIMETEXT_LAYER_EXTRAMARGIN_H;
  text_layer_init(&time_display_textlayer, tf);
  // - parameters
  text_layer_set_text_alignment(&time_display_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(&time_display_textlayer, GColorBlack); // black background
  text_layer_set_font(&time_display_textlayer, fonts_get_system_font(TIMETEXT_FONT)); // font
  text_layer_set_text_color(&time_display_textlayer, GColorWhite); // white text
  text_layer_set_text(&time_display_textlayer, "");
  layer_add_child(&window.layer, &time_display_textlayer.layer);

  // the text layer for displaying the weekday on top of the time
  GRect wf = window.layer.frame;
  // - on top of the screen
  wf.origin.y = -7; // to REALLY use the first pixel on the screen
  wf.size.h = AUXTEXT_H;
  text_layer_init(&weekday_display_textlayer, wf);
  // - parameters
  text_layer_set_text_alignment(&weekday_display_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(&weekday_display_textlayer, GColorBlack); // black background
  text_layer_set_font(&weekday_display_textlayer, fonts_get_system_font(AUXTEXT_FONT)); // font
  text_layer_set_text_color(&weekday_display_textlayer, GColorWhite); // white text
  text_layer_set_text(&weekday_display_textlayer, "TimeDot © 2013 by");
  layer_add_child(&window.layer, &weekday_display_textlayer.layer);

  // the text layer for displaying the date below the time
  GRect df = window.layer.frame;
  // - at bottom of the screen
  df.origin.y = df.size.h-AUXTEXT_H+2; // to REALLY use the last pixel on the screen
  df.size.h = AUXTEXT_H;
  text_layer_init(&date_display_textlayer, df);
  // - parameters
  text_layer_set_text_alignment(&date_display_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(&date_display_textlayer, GColorBlack); // black background
  text_layer_set_font(&date_display_textlayer, fonts_get_system_font(AUXTEXT_FONT)); // font
  text_layer_set_text_color(&date_display_textlayer, GColorWhite); // white text
  text_layer_set_text(&date_display_textlayer, "plan44.ch / luz");
  layer_add_child(&window.layer, &date_display_textlayer.layer);

  // the layer for displaying the seconds dot
  layer_init(&second_display_layer, window.layer.frame);
  second_display_layer.update_proc = &second_display_layer_callback;
  layer_add_child(&window.layer, &second_display_layer);
  layer_mark_dirty(&second_display_layer); // draw immediately

}



void pbl_main(void *params)
{
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
