/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

#include "usb_descriptors.h"

#include "pico/stdlib.h"
#include "adns5050.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

#define ENC_A 2
#define ENC_B 6

adns5050_t adns5050;
int8_t delta_x;
int8_t delta_y;
uint8_t encoder;
int8_t vertical;
int8_t horizontal;
uint8_t button_mask;

void hid_task(void);
void enc_a_changed();
void enc_b_changed();
void irq_callback(uint gpio, uint32_t events);

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

  // Right button
  gpio_init(20);
  gpio_set_dir(20, GPIO_IN);
  gpio_set_pulls(20, true, false);
  // Left button
  gpio_init(11);
  gpio_set_dir(11, GPIO_IN);
  gpio_set_pulls(11, true, false);

  // A phase
  gpio_init(18);
  gpio_set_dir(18, GPIO_IN);
  gpio_pull_up(18);
  gpio_set_irq_enabled_with_callback(18, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, irq_callback);
  // B phase
  gpio_init(10);
  gpio_set_dir(10, GPIO_IN);
  gpio_pull_up(10);
  gpio_set_irq_enabled_with_callback(10, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, irq_callback);

  adns5050_init(&adns5050, 16, 14, 9);
  gpio_init(12);
  gpio_set_dir(12, GPIO_OUT);
  gpio_put(12, true);

  adns5050_begin(&adns5050);
  sleep_ms(1);
  adns5050_sync(&adns5050);

  tusb_init();

  while (1)
  {
    tud_task(); // tinyusb device task

    hid_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  delta_y += adns5050_read(&adns5050, ADNS5050_DELTA_X_REG);
  delta_x -= adns5050_read(&adns5050, ADNS5050_DELTA_Y_REG);

  if (gpio_get(20) == 0)
    button_mask |= MOUSE_BUTTON_LEFT;
  else
    button_mask &= ~MOUSE_BUTTON_LEFT;
  if (gpio_get(11) == 0)
    button_mask |= MOUSE_BUTTON_RIGHT;
  else
    button_mask &= ~MOUSE_BUTTON_RIGHT;

  if (board_millis() - start_ms < interval_ms)
    return; // not enough time
  start_ms += interval_ms;

  uint32_t const btn = board_button_read();

  // Remote wakeup
  if (tud_suspended() && btn)
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }
  else
  {
    // keyboard interface
    if (tud_hid_n_ready(ITF_NUM_KEYBOARD))
    {
      // used to avoid send multiple consecutive zero report for keyboard
      static bool has_keyboard_key = false;

      uint8_t const report_id = 0;
      uint8_t const modifier = 0;

      if (btn)
      {
        uint8_t keycode[6] = {0};
        keycode[0] = HID_KEY_ARROW_RIGHT;

        tud_hid_n_keyboard_report(ITF_NUM_KEYBOARD, report_id, modifier, keycode);
        has_keyboard_key = true;
      }
      else
      {
        // send empty key report if previously has key pressed
        if (has_keyboard_key)
          tud_hid_n_keyboard_report(ITF_NUM_KEYBOARD, report_id, modifier, NULL);
        has_keyboard_key = false;
      }
    }

    // mouse interface
    if (tud_hid_n_ready(ITF_NUM_MOUSE))
    {
      uint8_t const report_id = 0;
      tud_hid_n_mouse_report(ITF_NUM_MOUSE, report_id, button_mask, delta_x, delta_y, vertical, horizontal);

      button_mask = delta_x = delta_y = vertical = horizontal = 0;
    }
  }
}

// Invoked when received SET_PROTOCOL request
// protocol is either HID_PROTOCOL_BOOT (0) or HID_PROTOCOL_REPORT (1)
void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol)
{
  (void)instance;
  (void)protocol;

  // nothing to do since we use the same compatible boot report for both Boot and Report mode.
  // TOOD set a indicator for user
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint8_t len)
{
  (void)instance;
  (void)report;
  (void)len;

  // nothing to do
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  (void)report_id;

  // keyboard interface
  if (instance == ITF_NUM_KEYBOARD)
  {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_type == HID_REPORT_TYPE_OUTPUT)
    {
      // bufsize should be (at least) 1
      if (bufsize < 1)
        return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        // Capslock On: disable blink, turn led on
      }
      else
      {
        // Caplocks Off: back to normal blink
      }
    }
  }
}

void enc_a_changed()
{
  uint8_t prev = encoder & 0b11;

  // An-1:Bn-1:An:Bn
  encoder <<= 2;
  encoder |= (prev ^ 0b10);

  switch (encoder & 0b1111)
  {
  // A:Low->High, B:Low
  case 0b0010:
  // A:High->Low, B:High
  case 0b1101:
    vertical++;
    break;
  // A:Low->High, B:High
  case 0b0111:
  // A:High->Low, B:Low
  case 0b1000:
    vertical--;
    break;
  }
}

void enc_b_changed()
{
  uint8_t prev = encoder & 0b11;

  // An-1:Bn-1:An:Bn
  encoder <<= 2;
  encoder |= (prev ^ 0b01);

  switch (encoder & 0b1111)
  {
  // A:High, B:Low->High
  case 0b1011:
  // A:Low, B:High->Low
  case 0b0100:
    vertical++;
    break;
  // A:High, B:High->Low
  case 0b1110:
  // A:Low, B:Low->High
  case 0b0001:
    vertical--;
    break;
  }
}

void irq_callback(uint gpio, uint32_t events)
{
  if (gpio == 18) {
    enc_a_changed();
  }
  else if (gpio == 10) {
    enc_b_changed();
  }
}
