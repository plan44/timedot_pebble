/*

  TimeDot watch
  (c) 2013-2014 by Lukas Zeller, plan44.ch

 */

#include <pebble.h>

// Variables
// - the window
Window *window;
// - layers
Layer *second_display_layer;
TextLayer *time_display_textlayer;
TextLayer *weekday_display_textlayer;
TextLayer *date_display_textlayer;

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
#define TIMETEXT_FONT FONT_KEY_BITHAM_42_MEDIUM_NUMBERS

#define AUXTEXT_LAYER_EXTRAMARGIN_H 17
#define AUXTEXT_H 20
#define AUXTEXT_FONT FONT_KEY_GOTHIC_18_BOLD

#define DOT_MARGIN 8
#define DOT_RADIUS 4


void second_display_layer_callback(struct Layer *layer, GContext *ctx)
{
  // unsigned int angle = currentSecond * 6;
  int32_t hexAngle = (int32_t)currentSecond * 1092;
  GRect f = layer_get_frame(layer);
  GPoint center = grect_center_point(&f);
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
void handle_second_tick(struct tm *tick_time, TimeUnits units_changed)
{
  currentSecond = tick_time->tm_sec;
  if (units_changed & MINUTE_UNIT) timeTextValid = 0; // new minute, needs new text
  if (!timeTextValid) {
    // update time text
    if (units_changed & DAY_UNIT) dateTextValid = 0;
    #ifdef DEBUG_TIME_STRING
    text_layer_set_text(time_display_textlayer, DEBUG_TIME_STRING);
    #else
    if (clock_is_24h_style())
      strftime(time_str_buffer, TIME_STR_BUFFER_BYTES, "%H:%M", tick_time);
    else
      strftime(time_str_buffer, TIME_STR_BUFFER_BYTES, "%I:%M", tick_time);
    text_layer_set_text(time_display_textlayer, time_str_buffer);
    #endif
    timeTextValid = 1; // now valid
  }
  if (!dateTextValid) {
    // update weekday text
    strftime(weekday_str_buffer, TIME_STR_BUFFER_BYTES, "%A", tick_time);
    text_layer_set_text(weekday_display_textlayer, weekday_str_buffer);
    // update date text
    strftime(date_str_buffer, TIME_STR_BUFFER_BYTES, "%Y-%m-%d", tick_time);
    text_layer_set_text(date_display_textlayer, date_str_buffer);
  }
  // anyway, update second
  layer_mark_dirty(second_display_layer);
}



void init()
{
  // the window
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_stack_push(window, true /* Animated */);

  // init the engine
  timeTextValid = 0; // time text not valid
  dateTextValid = 0; // date text not valid
  struct tm *t;
  time_t now = time(NULL);
  t = localtime(&now);
  currentSecond = t->tm_sec; // start with correct second position

  // get the window frame
  GRect wf = layer_get_frame(window_get_root_layer(window));
  // the text layer for displaying the Ziit time text in big font
  GRect tf = wf;  // start calculation with window frame
  // - vertically centered rectangle for time
  tf.origin.y += (tf.size.h-(tf.size.w-2*TIMETEXT_LAYER_MINMARGIN)) / 2 + TIMETEXT_LAYER_EXTRAMARGIN_H;
  tf.size.h = tf.size.w - TIMETEXT_LAYER_EXTRAMARGIN_H;
  time_display_textlayer = text_layer_create(tf);

  // - parameters
  text_layer_set_text_alignment(time_display_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(time_display_textlayer, GColorBlack); // black background
  text_layer_set_font(time_display_textlayer, fonts_get_system_font(TIMETEXT_FONT)); // font
  text_layer_set_text_color(time_display_textlayer, GColorWhite); // white text
  text_layer_set_text(time_display_textlayer, "");
  layer_add_child(window_get_root_layer(window), (Layer *)time_display_textlayer);

  // the text layer for displaying the weekday on top of the time
  GRect wdf = wf;
  // - on top of the screen
  wdf.origin.y = -7; // to REALLY use the first pixel on the screen
  wdf.size.h = AUXTEXT_H;
  weekday_display_textlayer = text_layer_create(wdf);
  // - parameters
  text_layer_set_text_alignment(weekday_display_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(weekday_display_textlayer, GColorBlack); // black background
  text_layer_set_font(weekday_display_textlayer, fonts_get_system_font(AUXTEXT_FONT)); // font
  text_layer_set_text_color(weekday_display_textlayer, GColorWhite); // white text
  text_layer_set_text(weekday_display_textlayer, "TimeDot © 2013 by");
  layer_add_child(window_get_root_layer(window), (Layer *)weekday_display_textlayer);

  // the text layer for displaying the date below the time
  GRect df = wf;
  // - at bottom of the screen
  df.origin.y = df.size.h-AUXTEXT_H+2; // to REALLY use the last pixel on the screen
  df.size.h = AUXTEXT_H;
  date_display_textlayer = text_layer_create(df);
  // - parameters
  text_layer_set_text_alignment(date_display_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(date_display_textlayer, GColorBlack); // black background
  text_layer_set_font(date_display_textlayer, fonts_get_system_font(AUXTEXT_FONT)); // font
  text_layer_set_text_color(date_display_textlayer, GColorWhite); // white text
  text_layer_set_text(date_display_textlayer, "plan44.ch / luz");
  layer_add_child(window_get_root_layer(window), (Layer *)date_display_textlayer);

  // the layer for displaying the seconds dot
  second_display_layer = layer_create(wf);
  layer_set_update_proc(second_display_layer, &second_display_layer_callback);
  layer_add_child(window_get_root_layer(window), second_display_layer);
  layer_mark_dirty(second_display_layer); // draw immediately

  // now subscribe to ticks
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
}


void deinit()
{
  layer_destroy(second_display_layer);
  text_layer_destroy(date_display_textlayer);
  text_layer_destroy(weekday_display_textlayer);
  text_layer_destroy(time_display_textlayer);
  window_destroy(window);
}



int main(void)
{
  init();
  app_event_loop();
  deinit();
  return 0;
}
