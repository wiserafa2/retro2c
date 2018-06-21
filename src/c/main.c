#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_min_layer;
static TextLayer *s_sec_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_steps_layer;
static TextLayer *s_points_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_weather2_layer;
static TextLayer *s_prompt_layer;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static BitmapLayer *s_Bbattery_layer;
static GBitmap *s_battery_bitmap;

static BitmapLayer *s_BT_layer;
static GBitmap *s_BT_bitmap;

static BitmapLayer *s_cursor_layer;
static GBitmap *s_cursor_bitmap;

static GFont s_time_font;
static GFont s_sec_font;

static bool showPoints = true;
static bool haveWeather = false;
static bool message_sent = false;

static bool isGS, isFahrenheit = false;
static bool showDistance, showDate, showWeather = true;


static bool ShowCursor = false;

char to_upper(char ch1)
{
      char ch2;
 
      if(ch1 >= 'a' && ch1 <= 'z'){
            ch2 = ('A' + ch1 - 'a');
            return ch2;
      }
      else{
            ch2 = ch1;
            return ch2;
      }
}

static void update_time() 
{
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char h_buffer[3];
  static char m_buffer[3];
  static char s_buffer[3];
  static char w_buffer[4];
  static char month_buffer[4]; 
  static char day_buffer[3];
  
  strftime(h_buffer, sizeof(h_buffer), clock_is_24h_style() ?
                                          "%H " : "%I ", tick_time); 
  
  if (showPoints)
    {
    text_layer_set_text(s_points_layer, ":");
    }
  else
    {
    text_layer_set_text(s_points_layer, "");
    }
  
  showPoints = !showPoints;
    
  strftime(m_buffer, sizeof(m_buffer), "%M", tick_time);
  strftime(s_buffer, sizeof(s_buffer), "%S", tick_time);
  strftime(w_buffer, sizeof(w_buffer), "%a", tick_time);
  strftime(month_buffer, sizeof(month_buffer), "%b", tick_time);
  strftime(day_buffer, sizeof(day_buffer), "%d", tick_time);
  
  //h_buffer[0] = '0';
  //h_buffer[1] = '0';
  
  //m_buffer[0] = '0';
  //m_buffer[1] = '0';
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, h_buffer);
  text_layer_set_text(s_min_layer, m_buffer);
  text_layer_set_text(s_sec_layer, s_buffer);
  
  month_buffer[1] = to_upper(month_buffer[1]);
  month_buffer[2] = to_upper(month_buffer[2]);

  static char date_buffer[12];
  
  date_buffer[0] = w_buffer[0];
  date_buffer[1] = w_buffer[1];
  date_buffer[2] = w_buffer[2];
  date_buffer[3] = ',';
  date_buffer[4] = ' ';
  date_buffer[5] = day_buffer[0];
  date_buffer[6] = day_buffer[1];
  date_buffer[7] = '-';
  date_buffer[8] = month_buffer[0];
  date_buffer[9] = month_buffer[1];
  date_buffer[10] = month_buffer[2];
  date_buffer[11] = 0;
  
  text_layer_set_text(s_date_layer, date_buffer);
  
 // text_layer_set_text(s_month_layer, month_buffer); 
 // text_layer_set_text(s_day_layer, day_buffer);
  
  ShowCursor = !ShowCursor;
  layer_set_hidden(bitmap_layer_get_layer(s_cursor_layer), ShowCursor);
    
   #if defined(PBL_HEALTH)
  
  const HealthMetric metric = HealthMetricWalkedDistanceMeters; //HealthMetricStepCount;//
  const HealthValue distance = health_service_sum_today(metric);


  // Get the preferred measurement system
  MeasurementSystem system = health_service_get_measurement_system_for_display(
                                                                        metric);

  // Format accordingly
  static char s_steps_buffer[32];
  switch(system) 
  {
    case MeasurementSystemMetric:
    
    if (distance > 1000)
    {
      int km = (int) distance / 1000;
      int rest = (int) distance  % 1000;
      static char s_rest_buffer[2];
      snprintf(s_rest_buffer, sizeof(s_rest_buffer), "%d", (int)rest);
      snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d,%s Km", (int)km, s_rest_buffer);
    }
    else
      snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d m", (int)distance); 
     // APP_LOG(APP_LOG_LEVEL_DEBUG, "Dist: %d m", (int) distance);
      break;
    
    case MeasurementSystemImperial: 
    {
      // Convert to imperial first
      int feet = (int)((float)distance * 3.28F);
      feet = 1234567;
      
      if (feet > 5280)
      {
        feet = feet / 5280;
        int rest = feet % 5280;
        static char s_rest_buffer[2];
        snprintf(s_rest_buffer, sizeof(s_rest_buffer), "%d", (int)rest);
        snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d,%s Mi", (int)feet, s_rest_buffer);
      }
      else
      {
        snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d Ft", (int)feet);
      }
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Dist: %d m", (int)  distance);
    } break;
    
    case MeasurementSystemUnknown:
    default:
     snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d", (int)distance);
     // APP_LOG(APP_LOG_LEVEL_INFO, "MeasurementSystem unknown or does not apply");
 
  }

  // Display to user in correct units
  text_layer_set_text(s_steps_layer, s_steps_buffer);

   #endif 
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) 
{
  update_time();
  
  if(!message_sent)
  {
    message_sent = true;
    if (!haveWeather)
    {
      //if still don't have weather information, try again in 2 minutes
      if(tick_time->tm_min % 2 == 0) {
      // Begin dictionary
      DictionaryIterator *iter;
      app_message_outbox_begin(&iter);
  
      // Add a key-value pair
      dict_write_uint8(iter, 0, 0);
  
      // Send the message!
      app_message_outbox_send();
    }
    }
    else
    // Get weather update every 20 minutes
    if(tick_time->tm_min % 20 == 0) {
      // Begin dictionary
      DictionaryIterator *iter;
      app_message_outbox_begin(&iter);
  
      // Add a key-value pair
      dict_write_uint8(iter, 0, 0);
  
      // Send the message!
      app_message_outbox_send();
    }
  }
}

static void config_text_layer(TextLayer *layer, GFont font)
{
  
  text_layer_set_background_color(layer, GColorClear);
  if (isGS == false) text_layer_set_text_color(layer, GColorGreen);
    else text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_text_alignment(layer, GTextAlignmentLeft);
  text_layer_set_font(layer, font);
  
}

static void change_Battery(BatteryChargeState charge)
{
 
  static char battery_text[]  = "000%";
  snprintf(battery_text, sizeof(battery_text), "%d%%", charge.charge_percent);
  text_layer_set_text(s_battery_layer, battery_text);
  
  gbitmap_destroy(s_battery_bitmap);
  
  if (charge.charge_percent > 90) 
    {
      // imagem 100
    s_battery_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_B_100);
    }
  else if (charge.charge_percent > 80)
    {
      // imagem 90
     s_battery_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_B_90);
    }   
  else if (charge.charge_percent > 70)
    {
      // imagem 80
     s_battery_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_B_80);
    }
  else if (charge.charge_percent > 60)
    {
      // imagem 70
    s_battery_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_B_70);
    }
  else if (charge.charge_percent > 50)
    {
      // imagem 60
    s_battery_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_B_60);
    }  
  else if (charge.charge_percent > 40)
    {
      // imagem 50
    s_battery_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_B_50);
    }
  else if (charge.charge_percent > 30)
    {
      // imagem 40
    s_battery_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_B_40);
    }
  else if (charge.charge_percent > 20)
    {
      // imagem 30
    s_battery_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_B_30);
    }
 else if (charge.charge_percent > 10)
    {
      // imagem 20
   s_battery_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_B_20);
    }
  else 
    {
      // imagem 10;
    s_battery_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_B_10);
    }
  
  bitmap_layer_set_bitmap(s_Bbattery_layer, s_battery_bitmap);
  
}

static void bluetooth_callback(bool connected) 
{
  
 // APP_LOG(APP_LOG_LEVEL_DEBUG, "Connected: %d", connected);
  gbitmap_destroy(s_BT_bitmap);
  
  if(!connected) 
  {
    // Issue a vibrating alert
    vibes_double_pulse();
    s_BT_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_OFF);
  }
  else
  {
    s_BT_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ON);
  }
  
  bitmap_layer_set_bitmap(s_BT_layer, s_BT_bitmap);
}

static void setBackground(char appletype)
{
  
  uint32_t key = 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "setando");
    if (appletype ==0)  // apple //c
    {
          s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG2C);
          bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap); 
          persist_write_int(key, 0);
          isGS = false;
    } else
    {
      if(appletype==1) // apple //e
      {
          s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG2E);
          bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
          persist_write_int(key, 1);
          isGS = false;
      }
      else 
      {
        if(appletype==2) // APPLE II
          {
            s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
            bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
            persist_write_int(key, 2);
            isGS = false;
          }
        else   // Apple IIgs
          {
            s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG2GS);
            bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
            persist_write_int(key, 3);
            isGS = true;
          }
          
      }
      

    }
  
  APP_LOG(APP_LOG_LEVEL_INFO, "setou");
  
    config_text_layer(s_time_layer,s_time_font);
    config_text_layer(s_min_layer,s_time_font);
    config_text_layer(s_sec_layer,s_sec_font);
    config_text_layer(s_date_layer,s_sec_font);
    //config_text_layer(s_battery_layer,s_sec_font);
    config_text_layer(s_steps_layer,s_sec_font);
    config_text_layer(s_points_layer, s_time_font);
    config_text_layer(s_weather_layer,s_sec_font);
    config_text_layer(s_weather2_layer,s_sec_font);
    config_text_layer(s_prompt_layer,s_sec_font);
  
   APP_LOG(APP_LOG_LEVEL_INFO, "configurou");
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) 
{
  
  haveWeather = true;
  message_sent = false;
  
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  static char weather2_layer_buffer[32];
  
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  Tuple *local_tuple = dict_find(iterator, MESSAGE_KEY_CITY);
  Tuple *config_tuple = dict_find(iterator, MESSAGE_KEY_APPLE_TYPE);
 
  Tuple *unit_tuple = dict_find(iterator, MESSAGE_KEY_USE_FAHRENHEIT);
  Tuple *weather_tuple = dict_find(iterator, MESSAGE_KEY_SHOW_WEATHER);
  Tuple *date_tuple = dict_find(iterator, MESSAGE_KEY_SHOW_DATE);
  Tuple *distance_tuple = dict_find(iterator, MESSAGE_KEY_SHOW_DISTANCE); 
  
  uint32_t key = 0;
  
  if(distance_tuple)
  {
    showDistance = distance_tuple->value->int32 == 1;
    layer_set_hidden(text_layer_get_layer(s_steps_layer), !showDistance);
    
    key = 1;
    persist_write_bool(key, !showDistance);
  }
  
  if(date_tuple)
  {
    showDate = date_tuple->value->int32 == 1 ;
    layer_set_hidden(text_layer_get_layer(s_date_layer), !showDate);
    
    key = 2;
    persist_write_bool(key, !showDate);
  }
  
  if(weather_tuple)
  {
    showWeather = weather_tuple->value->int32 == 1;
    layer_set_hidden(text_layer_get_layer(s_weather_layer), !showWeather);
    layer_set_hidden(text_layer_get_layer(s_weather2_layer), !showWeather);
    
    key = 3;
    persist_write_bool(key, !showWeather);
  }
  
  if(unit_tuple)
  {
    isFahrenheit = unit_tuple->value->int32 == 1;
    
    key = 4;
    persist_write_bool(key, isFahrenheit);
  }  
  
  // Fahrenheit formula = 
  //  (°C × 1.8) + 32 = °F
  static char unit;
  unit = 'C';
    
  // If all data is available, use it
  if(temp_tuple && conditions_tuple) 
  {
    int temp = temp_tuple->value->int32;
      if (isFahrenheit)
      {
        temp = (temp * 1.8) + 32;
        unit = 'F';
      }
    
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%d%c", temp, unit);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    snprintf(weather2_layer_buffer, sizeof(weather2_layer_buffer), "%s", local_tuple->value->cstring);
    
    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s-%s", temperature_buffer, conditions_buffer);
    
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
    text_layer_set_text(s_weather2_layer, weather2_layer_buffer);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Clima: %s", weather_layer_buffer); 
  }
  
  if(config_tuple)
  {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Appletype recieved"); 
    int appletype = config_tuple->value->int32 - 48;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "AppleType: %d",  appletype); 
      gbitmap_destroy(s_background_bitmap);
        
      setBackground(appletype); 
  }
  
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) 
{
  haveWeather = false;
  message_sent = false;
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) 
{
  haveWeather = false;
  message_sent = false;
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void testBackground()
{
uint32_t key = 0;
int config = 0;

if (persist_exists(key)) 
  {
  // Read persisted value
  config = persist_read_int(key);
  
  } else 
  {
  // Choose a default value
  config = 2;

  // Remember the default value until the user chooses their own value
  persist_write_int(key, config);
  }
  
  setBackground(config);
  
  key = 1; // distance
  if (persist_exists(key)) 
    {
    // Read persisted value
    showDistance = persist_read_bool(key);
    layer_set_hidden(text_layer_get_layer(s_steps_layer), showDistance);
    } 
  key = 2; // date
  if (persist_exists(key)) 
    {
    // Read persisted value
    showDate = persist_read_bool(key);
    layer_set_hidden(text_layer_get_layer(s_date_layer), showDate);
    } 
  key = 3; // Weather
  if (persist_exists(key)) 
    {
    // Read persisted value
    showWeather = persist_read_bool(key);
    layer_set_hidden(text_layer_get_layer(s_weather_layer), showWeather);
    layer_set_hidden(text_layer_get_layer(s_weather2_layer), showWeather);
    } 
  key = 4; // Use Fahrenheit
  if (persist_exists(key)) 
    {
    // Read persisted value
    isFahrenheit = persist_read_bool(key);
    } 

}

static void main_window_load(Window *window)
{
 
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_background_layer = bitmap_layer_create(bounds);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  
  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PRINT_CHAR_30));
  s_sec_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PRINT_CHAR_12));
  
  s_time_layer = text_layer_create(
      GRect(6, 38, bounds.size.w, 50));
  
  s_points_layer = text_layer_create(
      GRect(48, 38, bounds.size.w, 50));
  
 s_min_layer = text_layer_create(
      GRect(63, PBL_IF_ROUND_ELSE(58, 38), bounds.size.w, 30));
  
 s_sec_layer = text_layer_create(
      GRect(116, 41, bounds.size.w, 12));
  
 s_date_layer = text_layer_create(
      GRect(12,  75, bounds.size.w, 12));
  
  s_BT_layer = bitmap_layer_create(GRect(15,145,9,14));
  bitmap_layer_set_compositing_mode(s_BT_layer, GCompOpSet);
   
  s_battery_layer = text_layer_create(
      GRect(50, 145, 40, 20));
  
  s_Bbattery_layer =  bitmap_layer_create(GRect(95,145,37,14));
  bitmap_layer_set_compositing_mode(s_Bbattery_layer, GCompOpSet);

  s_prompt_layer = text_layer_create(
      GRect(12, 123, 30, 20));
  
  s_cursor_layer = bitmap_layer_create(GRect(22,125,9,9));
  s_cursor_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CURSOR);
  bitmap_layer_set_compositing_mode(s_cursor_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_cursor_layer, s_cursor_bitmap);
  
  s_steps_layer = text_layer_create(
      GRect(32, 123, bounds.size.w - 32 - 12, 20));
  
  s_weather_layer = text_layer_create(
      GRect(12,107,bounds.size.w, 12));
  
  s_weather2_layer = text_layer_create(
      GRect(12,94,bounds.size.w, 12));
  
  
  config_text_layer(s_time_layer,s_time_font);
  config_text_layer(s_min_layer,s_time_font);
  config_text_layer(s_sec_layer,s_sec_font);
  config_text_layer(s_date_layer,s_sec_font);
  config_text_layer(s_battery_layer,s_sec_font);
  config_text_layer(s_steps_layer,s_sec_font);
  config_text_layer(s_points_layer, s_time_font);
  config_text_layer(s_weather_layer,s_sec_font);
  config_text_layer(s_weather2_layer,s_sec_font);
  config_text_layer(s_prompt_layer,s_sec_font);

  text_layer_set_text(s_weather_layer, "");
  text_layer_set_text(s_prompt_layer, "]");
  
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  text_layer_set_text_color(s_battery_layer, GColorBlack);
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentRight);

  // Add it as a child layer to the Window's root layer
  
   testBackground();  //test and set the correct background
 
  
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_min_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_sec_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_points_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather2_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_prompt_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_BT_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_Bbattery_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_cursor_layer));
  
 
  change_Battery(battery_state_service_peek());

  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
}

static void main_window_unload(Window *window) 
{
  battery_state_service_unsubscribe();
  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_min_layer);
  text_layer_destroy(s_sec_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_steps_layer);
  text_layer_destroy(s_points_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_weather2_layer);
  text_layer_destroy(s_prompt_layer);

  // Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_sec_font);

  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  gbitmap_destroy(s_battery_bitmap);
  gbitmap_destroy(s_BT_bitmap);
  gbitmap_destroy(s_cursor_bitmap);

  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  bitmap_layer_destroy(s_Bbattery_layer);
  bitmap_layer_destroy(s_BT_layer);
  bitmap_layer_destroy(s_cursor_layer);
}


static void init() 
{
  // Create main Window element and assign to pointer
  s_main_window = window_create();
//APP_LOG(APP_LOG_LEVEL_INFO, "1");
  // Set the background color
  window_set_background_color(s_main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
//APP_LOG(APP_LOG_LEVEL_INFO, "2");
   
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
//APP_LOG(APP_LOG_LEVEL_INFO, "3");
  // Make sure the time is displayed from the start
  update_time();
//APP_LOG(APP_LOG_LEVEL_INFO, "4");
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

  battery_state_service_subscribe(change_Battery);
   
  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
  .pebble_app_connection_handler = bluetooth_callback
  });
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  //APP_LOG(APP_LOG_LEVEL_INFO, "Registrou callbacks");
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  // APP_LOG(APP_LOG_LEVEL_INFO, "Chamou mensagem");
  
}

static void deinit() 
{
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void)
{
 // APP_LOG(APP_LOG_LEVEL_INFO, "Initializing");
  init();
 // APP_LOG(APP_LOG_LEVEL_INFO, "Initialized");
  app_event_loop();
  deinit();
}