/*
 * This application shows how to use the Compass API to build a simple watchface
 * that shows where magnetic north is.
 *
 * The compass background image source is:
 *    <http://opengameart.org/content/north-and-southalpha-chanel>
 */

#include "pebble.h"
  
#define KEY_BIKENAME 0
#define KEY_ANGLE 1

// Vector paths for the compass needles
static const GPathInfo NEEDLE_NORTH_POINTS = { 
  3,
  (GPoint[]) { { -8, 0 }, { 8, 0 }, { 0, -36 } }
};
static const GPathInfo NEEDLE_SOUTH_POINTS = { 
  3,
  (GPoint[]) { { 8, 0 }, { 0, 36 }, { -8, 0 } }
};

static Window *s_main_window;
static BitmapLayer *s_bitmap_layer;
static GBitmap *s_background_bitmap;
static Layer *s_path_layer;
static TextLayer *s_heading_layer, *s_text_layer_calib_state, *s_text_test, *s_text_testb;

static GPath *s_needle_north, *s_needle_south;

static int angle;

static void compass_heading_handler(CompassHeadingData heading_data) {
  // rotate needle accordingly
  gpath_rotate_to(s_needle_north, heading_data.magnetic_heading);
  gpath_rotate_to(s_needle_south, heading_data.magnetic_heading);

  APP_LOG(APP_LOG_LEVEL_INFO, "Current magnetic degress:");
  // display heading in degrees and radians
  static char s_heading_buf[64];
  snprintf(s_heading_buf, sizeof(s_heading_buf),
    " %ld°\n%ld.%02ldpi",
    TRIGANGLE_TO_DEG(heading_data.magnetic_heading),
    // display radians, units digit
    (TRIGANGLE_TO_DEG(heading_data.magnetic_heading) * 2) / 360,
    // radians, digits after decimal
    ((TRIGANGLE_TO_DEG(heading_data.magnetic_heading) * 200) / 360) % 100
  );
  
  int degrees = TRIGANGLE_TO_DEG(heading_data.magnetic_heading);
  static char testdegstr[64];
  
  int actualAngle = angle - degrees;
  APP_LOG(APP_LOG_LEVEL_INFO, "Actual angle: %d", actualAngle);
  snprintf(testdegstr, sizeof(testdegstr), "%d", (int)actualAngle);
  
  text_layer_set_text(s_heading_layer, s_heading_buf);
  
  text_layer_set_text(s_text_testb, testdegstr);

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
    alert_bounds = GRect(0, -3, bounds.size.w, bounds.size.h / 7);
    text_layer_set_background_color(s_text_layer_calib_state, GColorClear);
    text_layer_set_text_color(s_text_layer_calib_state, GColorBlack);
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
      snprintf(s_valid_buf, sizeof(s_valid_buf), "%s", "Calibrated");
      break;
  }
  text_layer_set_text(s_text_layer_calib_state, s_valid_buf);

  // trigger layer for refresh
  layer_mark_dirty(s_path_layer);
}

static void path_layer_update_callback(Layer *path, GContext *ctx) {
#ifdef PBL_COLOR
  graphics_context_set_fill_color(ctx, GColorRed);
#endif
  gpath_draw_filled(ctx, s_needle_north);       
#ifndef PBL_COLOR
  graphics_context_set_fill_color(ctx, GColorBlack);
#endif  
  gpath_draw_outline(ctx, s_needle_south);                     

  // creating centerpoint                 
  GRect bounds = layer_get_frame(path);          
  GPoint path_center = GPoint(bounds.size.w / 2, bounds.size.h / 2);  
  graphics_fill_circle(ctx, path_center, 3);       

  // then put a white circle on top               
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, path_center, 2);                      
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the bitmap for the background and put it on the screen
  s_bitmap_layer = bitmap_layer_create(bounds);
//   s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_COMPASS_BACKGROUND);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_background_bitmap);
  
  // Make needle background 'transparent' with GCompOpAnd
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpAnd);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));

  // Create the layer in which we will draw the compass needles
  s_path_layer = layer_create(bounds);
  
  //  Define the draw callback to use for this layer
  layer_set_update_proc(s_path_layer, path_layer_update_callback);
  layer_add_child(window_layer, s_path_layer);

  // Initialize and define the two paths used to draw the needle to north and to south
  s_needle_north = gpath_create(&NEEDLE_NORTH_POINTS);
  s_needle_south = gpath_create(&NEEDLE_SOUTH_POINTS);

  // Move the needles to the center of the screen.
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  gpath_move_to(s_needle_north, center);
  gpath_move_to(s_needle_south, center);

  // Place text layers onto screen: one for the heading and one for calibration status
  s_heading_layer = text_layer_create(GRect(12, bounds.size.h * 3 / 4, bounds.size.w / 4, bounds.size.h / 5));
  text_layer_set_text(s_heading_layer, "No Data");
  layer_add_child(window_layer, text_layer_get_layer(s_heading_layer));

  s_text_layer_calib_state = text_layer_create(GRect(0, 50, bounds.size.w, bounds.size.h / 7));
  text_layer_set_text_alignment(s_text_layer_calib_state, GTextAlignmentLeft);
  text_layer_set_background_color(s_text_layer_calib_state, GColorClear);

  layer_add_child(window_layer, text_layer_get_layer(s_text_layer_calib_state));
  
  s_text_test = text_layer_create(GRect(0,0, bounds.size.w, bounds.size.h / 7));
  text_layer_set_text_alignment(s_text_test, GTextAlignmentLeft);
  text_layer_set_background_color(s_text_test, GColorClear);
  text_layer_set_text(s_text_test, "CHRIS");
  layer_add_child(window_layer, text_layer_get_layer(s_text_test));
  
  s_text_testb = text_layer_create(GRect(25,25, bounds.size.w, bounds.size.h / 7));
  text_layer_set_text_alignment(s_text_testb, GTextAlignmentLeft);
  text_layer_set_background_color(s_text_testb, GColorClear);
  text_layer_set_text(s_text_testb, "CHRIS");
  layer_add_child(window_layer, text_layer_get_layer(s_text_testb));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_heading_layer);
  text_layer_destroy(s_text_layer_calib_state);
  text_layer_destroy(s_text_test);
  text_layer_destroy(s_text_testb);
  gpath_destroy(s_needle_north);
  gpath_destroy(s_needle_south);
  layer_destroy(s_path_layer);
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

    APP_LOG(APP_LOG_LEVEL_INFO, "Updating Time");
  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
//   text_layer_set_text(s_heading_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
   update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_sec % 20 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint16(iter, 0, 0);

    text_layer_set_text(s_text_test, "getting data...");
    // Send the message!
    APP_LOG(APP_LOG_LEVEL_INFO, "%d sending message!", (int)0);
    app_message_outbox_send();
  }
}

static double to_degrees(double radians) {
    return radians * (180.0 / 3.14159);
}

static bool is_c_digit (char c) {
    if ((c>='0') && (c<='9')) return true;
    return false;
}

static double to_dbl(char *s)
{
        double a = 0.0;
        int e = 0;
        int c;
        while ((c = *s++) != '\0' && is_c_digit(c)) {
                a = a*10.0 + (c - '0');
        }
        if (c == '.') {
                while ((c = *s++) != '\0' && is_c_digit(c)) {
                        a = a*10.0 + (c - '0');
                        e = e-1;
                }
        }
        if (c == 'e' || c == 'E') {
                int sign = 1;
                int i = 0;
                c = *s++;
                if (c == '+')
                        c = *s++;
                else if (c == '-') {
                        c = *s++;
                        sign = -1;
                }
                while (is_c_digit(c)) {
                        i = i*10 + (c - '0');
                        c = *s++;
                }
                e += i*sign;
        }
        while (e > 0) {
                a *= 10.0;
                e--;
        }
        while (e < 0) {
                a *= 0.1;
                e++;
        }
        return a;
}


static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  
  APP_LOG(APP_LOG_LEVEL_INFO, "%d got message back.", (int)0);
  
  // Store incoming information
  static char bikename_buffer[120];
  static char angle_buffer[32];
  
  
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_BIKENAME:
      APP_LOG(APP_LOG_LEVEL_INFO, "My Lat Found found: %s",  t->value->cstring);
      snprintf(bikename_buffer, sizeof(bikename_buffer), "%s", t->value->cstring);
      break;
    case KEY_ANGLE:
      APP_LOG(APP_LOG_LEVEL_INFO, "My Lng found: %d", (int)t->value->int32);
      snprintf(angle_buffer, sizeof(angle_buffer), "%d", (int)t->value->int32);
      angle = (int)t->value->int32;
      break;
        
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  text_layer_set_text(s_text_test, bikename_buffer);
  text_layer_set_text(s_heading_layer, angle_buffer);
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
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  compass_service_unsubscribe();
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}