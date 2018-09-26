#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#include <NeoPixelBrightnessBus.h>

#pragma GCC diagnostic pop

const uint16_t PixelCount = 12;
const uint8_t PixelPin = 14; // ignored

const uint8_t pixMap[] = {6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5};
const uint8_t brightness[] = {25, 20, 20, 20, 20, 50, 60, 70, 80, 90, 200, 200, 200, 200, 200, 200, 180, 150, 140, 130, 100, 100, 80, 40, 0};
const uint8_t static_pts[] = {3, 6, 9, 0};

const uint8_t colorSaturation = 255; // saturation of color constants
const RgbColor hh_color(colorSaturation, 0, 0);
const RgbColor mm_color(0, colorSaturation, 0);
const RgbColor hh_mm_color(colorSaturation, colorSaturation, 0);
const RgbColor ss_color(0, 0, colorSaturation / 2);
const RgbColor static_color(8, 0, 8);

const RgbColor black(0, 0, 0);
const RgbColor spin_color(0, 105, 85);

NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

const uint8_t PIXELS = 12;
const RgbColor *display[PIXELS];

void display_init()
{
  strip.Begin();
  strip.Show();
}

void spin()
{
  static int led_no = 0;

  if (led_no > 0)
  {
    strip.SetPixelColor(led_no - 1, black);
  }

  strip.SetPixelColor(led_no, spin_color);
  strip.Show();
  led_no++;
  led_no %= PixelCount;
}

void render_time(int h, int m, int s)
{
  
  uint8_t px;

  h %= 12;
  m /= 5;
  s /= 5;

  for (uint8_t i = 0; i < PIXELS; i++)
  {
    display[i] = &black;
  }

  for (uint8_t i = 0; i < sizeof(static_pts); i++)
  {
    uint8_t pt = static_pts[i];
    px = pixMap[pt];
    display[px] = &static_color;
  }

  px = pixMap[s];
  display[px] = &ss_color;

  px = pixMap[h];
  display[px] = &hh_color;

  px = pixMap[m];
  display[px] = (h == m) ? &hh_mm_color : &mm_color;
}

void display_time(int h)
{
  uint8_t br = brightness[h];
  for (uint8_t i = 0; i < PIXELS; i++)
  {
    strip.SetPixelColor(i, *display[i]);
  }

  strip.SetBrightness(br);
  strip.Show();
}

void display_clean()
{
  for (uint8_t i = 0; i < PIXELS; i++)
  {
    strip.SetPixelColor(i, black);
  }

  strip.Show();
}
