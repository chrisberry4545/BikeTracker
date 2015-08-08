/*
 * Application which points you towards the nearest London Santander Bike Terminal.
 */

#include "pebble.h"
  
#define KEY_BIKENAME 0
#define KEY_ANGLE 1

static const GPathInfo MAIN_ARROW_POINTS = { 
  7,
  (GPoint[]) { { -12, 15 }, { 0, -25 }, { 12, 15 }, { 6, 15 }, { 6, 25 }, { -6, 25 }, { -6, 15 } }
};

static Window *s_main_window;
static BitmapLayer *s_bitmap_layer, *s_refresh_bitmap_layer;
static GBitmap *s_background_bitmap, *s_refresh_bitmap;
static Layer *s_path_layer;
static TextLayer *s_location_name, *s_text_layer_calib_state, *s_time_display;

static GPath *s_main_arrow;

static int angle;

static void compass_heading_handler(CompassHeadingData heading_data) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Current magnetic degress:");
  
  int degrees = TRIGANGLE_TO_DEG(heading_data.magnetic_heading);
  
  int actualAngle = angle + degrees;
  APP_LOG(APP_LOG_LEVEL_INFO, "Actual angle: %d", actualAngle);
  int32_t adjustedAngle = ((double)actualAngle / 360) * TRIG_MAX_ANGLE;
  
  gpath_rotate_to(s_main_arrow, adjustedAngle);
  

  // Modify alert layout depending on calibration state
  GRect bounds = layer_get_frame(window_get_root_layer(s_main_window)); 
  GRect alert_bounds; 
  if(heading_data.compass_status == CompassStatusDataInvalid) {
    // Tell user to move their arm
    alert_bounds = GRect(0, 0, bounds.size.w, bounds.size.h);
    text_layer_set_background_color(s_text_layer_calib_state, GColorBlack);
    text_layer_set_text_color(s_text_layer_calib_state, GColorWhite);
    text_layer_set_font(s_text_layer_calib_state, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(s_text_layer_calib_state, GTextAlignmentCenter);
  } else {
    // Show status at the top
    alert_bounds = GRect(0, 25, bounds.size.w, bounds.size.h / 7);
    text_layer_set_background_color(s_text_layer_calib_state, GColorClear);
    text_layer_set_text_color(s_text_layer_calib_state, GColorWhite);
    text_layer_set_font(s_text_layer_calib_state, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_text_layer_calib_state, GTextAlignmentLeft);
  }
  layer_set_frame(text_layer_get_layer(s_text_layer_calib_state), alert_bounds);

  // Display state of the compass
  static char s_valid_buf[64];
  switch (heading_data.compass_status) {
    case CompassStatusDataInvalid:
      snprintf(s_valid_buf, sizeof(s_valid_buf), "%s", "Compass is calibrating!\n\nMove your arm to aid calibration.");
      break;
    case CompassStatusCalibrating:
      snprintf(s_valid_buf, sizeof(s_valid_buf), "%s", "Fine tuning...");
      break;
    case CompassStatusCalibrated:
      snprintf(s_valid_buf, sizeof(s_valid_buf), "%s", "");
      break;
  }
  text_layer_set_text(s_text_layer_calib_state, s_valid_buf);

  // trigger layer for refresh
  layer_mark_dirty(s_path_layer);
}

static void path_layer_update_callback(Layer *path, GContext *ctx) {
#ifdef PBL_COLOR
  graphics_context_set_fill_color(ctx, GColorWhite);
#endif
  gpath_draw_filled(ctx, s_main_arrow);       
#ifndef PBL_COLOR
  graphics_context_set_fill_color(ctx, GColorWhite);
#endif                                  
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the bitmap for the background and put it on the screen
  s_bitmap_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_background_bitmap);
  bitmap_layer_set_background_color(s_bitmap_layer, GColorBlack);
  
  // Make needle background 'transparent' with GCompOpAnd
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpAnd);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));

  // Create the layer in which we will draw the compass needles
  s_path_layer = layer_create(bounds);
  
  //  Define the draw callback to use for this layer
  layer_set_update_proc(s_path_layer, path_layer_update_callback);
  layer_add_child(window_layer, s_path_layer);

  // Initialize and define the two paths used to draw the needle to north and to south
  s_main_arrow = gpath_create(&MAIN_ARROW_POINTS);

  // Move the needles to the center of the screen.
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  gpath_move_to(s_main_arrow, center);

  // Place text layers onto screen: one for the heading and one for calibration status
  s_location_name = text_layer_create(GRect(0, bounds.size.h * 3 / 4, bounds.size.w, bounds.size.h / 5));
  text_layer_set_text(s_location_name, "No Data");
  text_layer_set_text_alignment(s_location_name, GTextAlignmentCenter);
  text_layer_set_text_color(s_location_name, GColorWhite);
  text_layer_set_background_color(s_location_name, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_location_name));

  s_text_layer_calib_state = text_layer_create(GRect(0, 50, bounds.size.w, bounds.size.h / 7));
  text_layer_set_text_alignment(s_text_layer_calib_state, GTextAlignmentLeft);
  text_layer_set_background_color(s_text_layer_calib_state, GColorClear);

  layer_add_child(window_layer, text_layer_get_layer(s_text_layer_calib_state));
  
  s_time_display = text_layer_create(GRect(0,0, bounds.size.w, bounds.size.h / 7));
  text_layer_set_text_alignment(s_time_display, GTextAlignmentCenter);
  text_layer_set_text_color(s_time_display, GColorWhite);
  text_layer_set_background_color(s_time_display, GColorClear);
  text_layer_set_font(s_time_display, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_time_display));
  
  s_refresh_bitmap_layer = bitmap_layer_create(GRect(bounds.size.w - 16, 10, 16, 16));
  s_refresh_bitmap = gbitmap_create_with_resource(RESOURCE_ID_REFRESH);
  bitmap_layer_set_bitmap(s_refresh_bitmap_layer, s_refresh_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_refresh_bitmap_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_location_name);
  text_layer_destroy(s_text_layer_calib_state);
  text_layer_destroy(s_time_display);
  gpath_destroy(s_main_arrow);
  layer_destroy(s_path_layer);
  gbitmap_destroy(s_background_bitmap);
  gbitmap_destroy(s_refresh_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
  bitmap_layer_destroy(s_refresh_bitmap_layer);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";
  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

   text_layer_set_text(s_time_display, buffer);
}

static void updateLocationData() {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint16(iter, 0, 0);

    text_layer_set_text(s_location_name, "Updating info...");
    app_message_outbox_send();
}
  
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  
  if (tick_time->tm_sec % 60) {
   update_time(); 
  }
   
  
  // Update every 15 seconds
  if(tick_time->tm_sec % 15 == 0) {
    updateLocationData();
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  
  APP_LOG(APP_LOG_LEVEL_INFO, "%d got message back.", (int)0);
  
  static char bikename_buffer[120];
  
  // Read first item
  Tuple *t = dict_read_first(iterator);

  while(t != NULL) {
    switch(t->key) {
    case KEY_BIKENAME:
      APP_LOG(APP_LOG_LEVEL_INFO, "Nearest loc found: %s",  t->value->cstring);
      snprintf(bikename_buffer, sizeof(bikename_buffer), "%s", t->value->cstring);
      break;
    case KEY_ANGLE:
      APP_LOG(APP_LOG_LEVEL_INFO, "Angle to loc: %d", (int)t->value->int32);
      angle = (int)t->value->int32;
      break;
        
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  if (strcmp (bikename_buffer,"ERROR") != 0) {
    text_layer_set_text(s_location_name, bikename_buffer);
  } else {
    text_layer_set_text(s_location_name, "Error connecting to internet");
  }
}
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  updateLocationData();
}

static void click_config_provider() {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
}

static void init() {
  angle = 0;
  
  // initialize compass and set a filter to 2 degrees
  compass_service_set_heading_filter(2 * (TRIG_MAX_ANGLE / 360));
  compass_service_subscribe(&compass_heading_handler);

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  window_set_click_config_provider(s_main_window, click_config_provider);
  
  updateLocationData();
}

static void deinit() {
  app_message_deregister_callbacks();
  tick_timer_service_unsubscribe();
  compass_service_unsubscribe();
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}