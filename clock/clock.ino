// NeoPixelBus by Makuna
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#include <time.h>
#include <ESP8266WiFi.h>


#pragma GCC diagnostic pop

#include "udp.h"
#include "display.h"

#define DEBUG false

const char* ssid = "";  //  your network SSID (name)
const char* pass = "";       // your network password

// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
const unsigned long seventyYears = 2208988800UL; //NTP_TIMESTAMP_DIFF     
const int beginDSTMonth = 3;
const int endDSTMonth = 10;

const unsigned long NTP_WAIT = 0;
const unsigned long NTP_WIFI_TIMEOUT = 1;
const unsigned long NTP_RESP_TIMEOUT = 2;

union epoch_union {
time_t tm;
unsigned long n;
} ntp_epoch;

unsigned long time_start = 0;
    


int adjustDstEurope(int year, int month, int day)
{
    // last sunday of march
    int beginDSTDate = (31 - (5 * year / 4 + 4) % 7);
    //last sunday of october
    int endDSTDate = (31 - (5 * year /4 + 1) % 7);
    //Serial.println(endDSTDate);
    // DST is valid as:
    if (((month > beginDSTMonth) && (month < endDSTMonth))
        || ((month == beginDSTMonth) && (day >= beginDSTDate)) 
        || ((month == endDSTMonth) && (day <= endDSTDate)))
    {
        return 7200;  // DST europe = utc +2 hour
    }
 
    return 3600; // nonDST europe = utc +1 hour
}

void setup()
{
  Serial.begin(115200);
  Serial.println("begin setup");

  display_init();

  WiFi.enableSTA(true);
  WiFi.setAutoConnect (true);
  WiFi.setAutoReconnect (true);

  #if DEBUG
  Serial.print("Connecting to ");
  Serial.println(ssid);
  #endif
  
  WiFi.begin(ssid, pass);
  
  while (!WiFi.isConnected()) {
    spin();
    delay(200);
  }

  #if DEBUG
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  #endif
  
  upd_init();

  // get time 
  unsigned long ntp = 0;
  while (ntp < seventyYears)
  {
    ntp = callNTP();
    spin();
    delay(200);
  }

  time_start = millis() / 1000;
  ntp_epoch.n = ntp - seventyYears;
}

unsigned long callNTP() 
{
  const uint8_t max_retry = 15;
  enum State {none, wait_packet};
  static State state = none;
  static uint8_t wifi_retry_counter = 0;
  static uint8_t ntp_retry_counter = 0;

  Serial.print("wifi status: ");
  Serial.println(WiFi.isConnected() ? "connected" : "disconnected");
  if (!WiFi.isConnected()) 
  {
    Serial.print("wifi retry: ");
    Serial.println(wifi_retry_counter);

    WiFi.begin(ssid, pass);
    wifi_retry_counter++;
    
    return wifi_retry_counter > max_retry ? NTP_WIFI_TIMEOUT : NTP_WAIT;
  }

  wifi_retry_counter = 0;
  
  if (state != wait_packet)
  {
    //WiFi.hostByName(ntpServerName, timeServerIP); 
    sendNTPpacket();
    state = wait_packet;
    ntp_retry_counter = 0;
    return NTP_WAIT;
  }

  if (state == wait_packet)
  {
    int cb = udp_parse();
    if (!cb) {

      ntp_retry_counter++;

      if (ntp_retry_counter > max_retry)
      {
        state = none;
        return NTP_RESP_TIMEOUT;
      }
      
      return NTP_WAIT;
    }
  }

  state = none;

  return udp_read_time();
}

void dump_time(struct tm *tm)
{
  #if DEBUG
  Serial.print(tm->tm_hour);
  Serial.print(":");
  Serial.print(tm->tm_min);
  Serial.print(":");
  Serial.println(tm->tm_sec);
  #endif
}
void loop()
{
  static bool err = false;
  static bool ntp_in_progress = false;
  static bool on_off = false;
  
  unsigned long local_time = millis();
  
  /*
  if (err && local_time % 500 == 0)
  {
    if (on_off) 
    {
      display_time(current_hour);
    }
    else
    {
      display_clean();
    }

    on_off = !on_off;
  }*/
  
  if (local_time % 2000 == 0)  // every 2 seconds
  {
    epoch_union tx_time;
    tx_time.n = ntp_epoch.n + (local_time / 1000) - time_start;
    
    struct tm *tm = gmtime(&tx_time.tm) ;
    int y = tm->tm_year + 1900;
    int m = tm->tm_mon + 1;
    int d = tm->tm_mday;
    
    int offset = adjustDstEurope(y, m, d);
    tx_time.n += offset;
    tm = gmtime(&tx_time.tm);

    
    render_time(tm->tm_hour, tm->tm_min, tm->tm_sec);
    display_time(tm->tm_hour);
    dump_time(tm);
  }

  if (local_time < time_start || local_time % (1000 * 60 * 60 * 12) == 0)
  {
    callNTP();
    ntp_in_progress = true;
    #ifdef DEBUG
    Serial.println("call ntp start");
    #endif
  }

  if (ntp_in_progress && local_time % 500)
  {
    unsigned long ntp = callNTP();
    if (ntp == NTP_WIFI_TIMEOUT || ntp == NTP_RESP_TIMEOUT)
    {
      err = true;
      Serial.print("ntp err: ");
      Serial.println(err);
    }
    else if (ntp > seventyYears) 
    {
      ntp_epoch.n = ntp - seventyYears;
      time_start = millis() / 1000;
      ntp_in_progress = false;
      err = false;
      Serial.println("ntp update");
    }
  }
}
