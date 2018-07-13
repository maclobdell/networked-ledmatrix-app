/*
 * Copyright (c) 2016 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "led_app.h"

DigitalOut led_red(LED_RED, APP_LED_OFF);
DigitalOut led_green(LED_GREEN, APP_LED_OFF);
DigitalOut led_blue(LED_BLUE, APP_LED_OFF);

uint8_t led_color;

void set_led_color(uint8_t color)
{
    if(color > 0 && color < 4){
       led_color = color;
    }
}

void set_led_off(void)
{
  led_red = APP_LED_OFF;
  led_green = APP_LED_OFF;
  led_blue = APP_LED_OFF;
}

void set_led_on(void)
{   
   if(led_color == LED_COLOR_RED) {
      led_red = APP_LED_ON;
      led_green = APP_LED_OFF;
      led_blue = APP_LED_OFF;
    }
    else if(led_color == LED_COLOR_GREEN) {
      led_red = APP_LED_OFF;
      led_green = APP_LED_ON;
      led_blue = APP_LED_OFF;
    } 
    else if(led_color == LED_COLOR_BLUE) {
      led_red = APP_LED_OFF;
      led_green = APP_LED_OFF;
      led_blue = APP_LED_ON;
    }           
}
