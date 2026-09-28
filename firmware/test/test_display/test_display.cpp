// E-paper display test (visual, interactive). There's no sensor feedback from the panel, so
// this test only fails if the display driver itself errors out -- inspect the physical screen
// against each printed instruction as it runs.
#include <Arduino.h>
#include <unity.h>

#include <Fonts/FreeMonoBold12pt7b.h>

#include "../test_common.h"
#include "drivers/EEPROM.h"
#include "drivers/display.h"

namespace {
Display _display;
}

void setUp(void) {}
void tearDown(void) {}

void test_display_pattern_sequence(void) {
    static StatusHeader dummyHeader = FreshHeader();
    _display.Init(true, &dummyHeader);

    TEST_MESSAGE("Full white screen -- check for a uniformly white panel with no artifacts (3s)");
    _display.ForceFullNextRefresh();
    _display.StartRefresh();
    _display.d->fillScreen(GxEPD_WHITE);
    _display.EndRefresh();
    delay(3000);

    TEST_MESSAGE("Full black screen -- check for uniform black, no streaks or dead pixels (3s)");
    _display.StartRefresh();
    _display.d->fillScreen(GxEPD_BLACK);
    _display.EndRefresh();
    delay(3000);

    TEST_MESSAGE("Border + 4 corner boxes + text -- check all are sharp and fully visible (3s)");
    int16_t w = _display.d->width();
    int16_t h = _display.d->height();
    _display.StartRefresh();
    _display.d->fillScreen(GxEPD_WHITE);
    _display.d->drawRect(0, 0, w, h, GxEPD_BLACK);
    _display.d->fillRect(0, 0, 15, 15, GxEPD_BLACK);
    _display.d->fillRect(w - 15, 0, 15, 15, GxEPD_BLACK);
    _display.d->fillRect(0, h - 15, 15, 15, GxEPD_BLACK);
    _display.d->fillRect(w - 15, h - 15, 15, 15, GxEPD_BLACK);
    _display.d->setFont(&FreeMonoBold12pt7b);
    _display.print(20, h / 2, "DISPLAY TEST");
    _display.EndRefresh();
    delay(3000);

    TEST_MESSAGE("Partial refresh x3 -- the small center box should flicker black/white");
    for (int i = 0; i < 3; i++) {
        _display.StartRefresh();
        _display.d->fillRect(w / 2 - 20, h / 2 - 20, 40, 40, (i % 2 == 0) ? GxEPD_BLACK : GxEPD_WHITE);
        _display.EndRefresh();
        delay(800);
    }

    _display.Hibernate();
}

void setup() {
    delay(2000);
    hwtestConfigurePins();

    UNITY_BEGIN();
    RUN_TEST(test_display_pattern_sequence);
    UNITY_END();
}

void loop() {}
