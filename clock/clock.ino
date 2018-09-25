// NeoPixelBus by Makuna
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#include <time.h>
#include <ESP8266WiFi.h>

#pragma GCC diagnostic pop

#include "udp.h"
#include "display.h"
#include "dst.h"

#define DEBUG false

const char *ssid = ""; //  your network SSID (name)
const char *pass = ""; // your network password

// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
const unsigned long NTP_TIMESTAMP_DIFF = 2208988800UL; 

const unsigned long NTP_WAIT = 0;
const unsigned long NTP_WIFI_TIMEOUT = 1;
const unsigned long NTP_RESP_TIMEOUT = 2;

const unsigned long UPDATE_INTERVAL_NTP = (1000 * 60 * 60 * 12);
const int UPDATE_INTERVAL_NTP_STATUS = 500;
const int UPDATE_INTERVAL_DISPLAY = 2000;

union epoch_union {
  time_t tm;
  unsigned long n;
} ntp_epoch;

unsigned long time_start = 0;


unsigned long callNTP()
{
  const uint8_t max_retry = 15;
  enum State
  {
    none,
    wait_packet
  };
  static State state = none;
  static uint8_t wifi_retry_counter = 0;
  static uint8_t ntp_retry_counter = 0;

  #if DEBUG
    Serial.print("wifi status: ");
    Serial.println(WiFi.isConnected() ? "connected" : "disconnected");
  #endif

  if (!WiFi.isConnected())
  {
     #if DEBUG
      Serial.print("wifi retry: ");
      Serial.println(wifi_retry_counter);
    #endif

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
    if (!cb)
    {
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

#if DEBUG
void dump_time(struct tm *tm)
{

  Serial.print(tm->tm_hour);
  Serial.print(":");
  Serial.print(tm->tm_min);
  Serial.print(":");
  Serial.println(tm->tm_sec);
}
#else
#define dump_time (void)
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println("begin setup");

  display_init();

  WiFi.enableSTA(true);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

#if DEBUG
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif

  WiFi.begin(ssid, pass);

  while (!WiFi.isConnected())
  {
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
  while (ntp < NTP_TIMESTAMP_DIFF)
  {
    ntp = callNTP();
    spin();
    delay(200);
  }

  time_start = millis() / 1000;
  ntp_epoch.n = ntp - NTP_TIMESTAMP_DIFF;
}

void loop()
{
  static bool ntp_in_progress = false;
  unsigned long local_time = millis();

  /*
  static bool err = false;
  static bool on_off = false;

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

  if (local_time % UPDATE_INTERVAL_DISPLAY == 0) // every 2 seconds
  {
    epoch_union tx_time;
    tx_time.n = ntp_epoch.n + (local_time / 1000) - time_start;

    struct tm *tm = gmtime(&tx_time.tm);
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

  if (local_time < time_start || local_time % UPDATE_INTERVAL_NTP == 0)
  {
    callNTP();
    ntp_in_progress = true;
#ifdef DEBUG
    Serial.println("call ntp start");
#endif
  }

  if (ntp_in_progress && local_time % UPDATE_INTERVAL_NTP_STATUS)
  {
    unsigned long ntp = callNTP();
    if (ntp == NTP_WIFI_TIMEOUT || ntp == NTP_RESP_TIMEOUT)
    {
      //err = true;
      #if DEBUG
        Serial.print("ntp err: ");
        Serial.println(err);
      #endif
    }
    else if (ntp > NTP_TIMESTAMP_DIFF)
    {
      ntp_epoch.n = ntp - NTP_TIMESTAMP_DIFF;
      time_start = millis() / 1000;
      ntp_in_progress = false;
      //err = false;
      #if DEBUG
        Serial.println("ntp updated");
      #endif
    }
  }
}
