/*********************************************************************
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  MIT license, check LICENSE for more information
  Copyright (c) 2019 Ha Thach for Adafruit Industries
  All text above, and the splash screen below must be included in
  any redistribution
*********************************************************************/

#include "Adafruit_TinyUSB.h"

Adafruit_USBD_HID usb_hid;

// Report ID
enum {
  RID_KEYBOARD = 1,
  RID_CONSUMER_CONTROL,  // Media, volume etc ..
};

// HID report descriptor using TinyUSB's template
uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD)),
  TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(RID_CONSUMER_CONTROL))
};

// A相・B相の接続ピンの定義
#define PIN_ROT_B 10
#define PIN_ROT_A 9

uint8_t keycode[6] = { 0 };

void setup() {
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();

  // B相の入力端子の設定（A相はアナログ入力として使用）
  pinMode(PIN_ROT_B, INPUT);

  // wait until device mounted
  while (!USBDevice.mounted()) delay(1);

  // digital入力であれば、以下のようにピン割り込みを使えるが、アナログ入力では使えない
  //  attachInterrupt(PIN_ROT_A, ISR_ROT_A, RISING);
}

uint8_t pinA, pinB, pinA_previous = 0;

#define TH_MID_L 500
#define TH_MID_H 550

void loop() {
  if (!usb_hid.ready()) return;
  uint16_t pinA_value = analogRead(PIN_ROT_A);
  pinB = digitalRead(PIN_ROT_B);
  if (pinA_value > TH_MID_L && pinA_value < TH_MID_H) {
    // クリック時
    // Alt+A for Zoom's mute
    keycode[0] = HID_KEY_A;
    usb_hid.keyboardReport(RID_KEYBOARD, KEYBOARD_MODIFIER_LEFTALT, keycode);
    delay(10);
    keycode[0] = 0;
    usb_hid.keyboardReport(RID_KEYBOARD, 0, keycode);
    delay(10);
    while (pinA_value > TH_MID_L && pinA_value < TH_MID_H) { // クリック解除まで待つ
      pinA_value = analogRead(PIN_ROT_A);
      delay(100);
    }
  } else {
    pinA = (pinA_value < 200) ? 0 : 1; // A相の値をデジタル化
    if (pinA_previous == 0 && pinA == 1) { // A相の立ち上がり時
      if (pinB == 1) {
        // increment
        usb_hid.sendReport16(RID_CONSUMER_CONTROL, HID_USAGE_CONSUMER_VOLUME_INCREMENT);
        delay(10);
        usb_hid.sendReport16(RID_CONSUMER_CONTROL, 0);
        delay(50);
      } else {
        // decrement
        usb_hid.sendReport16(RID_CONSUMER_CONTROL, HID_USAGE_CONSUMER_VOLUME_DECREMENT);
        delay(10);
        usb_hid.sendReport16(RID_CONSUMER_CONTROL, 0);
        delay(50);
      }
    }
    pinA_previous = pinA;
  }
}
