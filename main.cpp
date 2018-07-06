// ----------------------------------------------------------------------------
// Copyright 2016-2018 ARM Ltd.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------

#include "mbed.h"
#include "simple-mbed-cloud-client.h"
#include "LittleFileSystem.h"
#include "SPIFBlockDevice.h"
#include "ESP8266Interface.h"
    
// An event queue is a very useful structure to debounce information between contexts (e.g. ISR and normal threads)
// This is great because things such as network operations are illegal in ISR, so updating a resource in a button's fall() function is not allowed
EventQueue eventQueue;
Thread thread1;

// Storage implementation definition, currently using SDBlockDevice (SPI flash, DataFlash, and internal flash are also available)
/* Hexiwear (+ Basedboard)*/ 
InterruptIn sw2(PTA12);
DigitalOut led2(LED2);
SPIFBlockDevice spif(MBED_CONF_SPIF_DRIVER_SPI_MOSI, MBED_CONF_SPIF_DRIVER_SPI_MISO, MBED_CONF_SPIF_DRIVER_SPI_CLK, MBED_CONF_SPIF_DRIVER_SPI_CS);  //defined in mbed_app.json or spiflash driver
LittleFileSystem fs("sd", &spif);  // must keep the name "sd" for the cloud client currently

// Declaring pointers for access to Mbed Cloud Client resources outside of main()
MbedCloudClientResource *button_res;
MbedCloudClientResource *pattern_res;

static bool button_pressed = false;
static int button_count = 0;
      
void button_press() {
    button_pressed = true;
    ++button_count;
    button_res->set_value(button_count);
}

/**
 * PUT handler
 * @param resource The resource that triggered the callback
 * @param newValue Updated value for the resource
 */
void pattern_updated(MbedCloudClientResource *resource, m2m::String newValue) {
    printf("PUT received, new value: %s\n", newValue.c_str());
}

/**
 * POST handler
 * @param resource The resource that triggered the callback
 * @param buffer If a body was passed to the POST function, this contains the data.
 *               Note that the buffer is deallocated after leaving this function, so copy it if you need it longer.
 * @param size Size of the body
 */
void blink_callback(MbedCloudClientResource *resource, const uint8_t *buffer, uint16_t size) {
    printf("POST received. Going to blink LED pattern: %s\n", pattern_res->get_value().c_str());

    static DigitalOut augmentedLed(LED1); // LED that is used for blinking the pattern

    // Parse the pattern string, and toggle the LED in that pattern
    string s = std::string(pattern_res->get_value().c_str());
    size_t i = 0;
    size_t pos = s.find(':');
    while (pos != string::npos) {
        wait_ms(atoi(s.substr(i, pos - i).c_str()));
        augmentedLed = !augmentedLed;

        i = ++pos;
        pos = s.find(':', pos);

        if (pos == string::npos) {
            wait_ms(atoi(s.substr(i, s.length()).c_str()));
            augmentedLed = !augmentedLed;
        }
    }
}

/**
 * Notification callback handler
 * @param resource The resource that triggered the callback
 * @param status The delivery status of the notification
 */
void button_callback(MbedCloudClientResource *resource, const NoticationDeliveryStatus status) {
    printf("Button notification, status %s (%d)\n", MbedCloudClientResource::delivery_status_to_string(status), status);
}

/**
 * Registration callback handler
 * @param endpoint Information about the registered endpoint such as the name (so you can find it back in portal)
 */
void registered(const ConnectorClientEndpointInfo *endpoint) {
    printf("Connected to Mbed Cloud. Endpoint Name: %s\n", endpoint->internal_endpoint_name.c_str());
}

int main(void) {
    printf("Starting Simple Mbed Cloud Client example\n");
    printf("Connecting to the network using WiFi...\n");

    // Connect to the internet (DHCP is expected to be on)
    ESP8266Interface net(MBED_CONF_APP_WIFI_TX, MBED_CONF_APP_WIFI_RX);  //pins defined in mbed_app.json
    nsapi_error_t status = net.connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);

    if (status != 0) {
        printf("Connecting to the network failed %d!\n", status);
        return -1;
    }

    printf("Connected to the network successfully. IP address: %s\n", net.get_ip_address());

    // SimpleMbedCloudClient handles registering over LwM2M to Mbed Cloud
    SimpleMbedCloudClient client(&net, &spif, &fs);
    int client_status = client.init();
    if (client_status != 0) {
        printf("Initializing Mbed Cloud Client failed (%d)\n", client_status);
        return -1;
    }

    // Creating resources, which can be written or read from the cloud
    button_res = client.create_resource("3200/0/5501", "button_count");
    button_res->set_value(0);
    button_res->methods(M2MMethod::GET);
    button_res->observable(true);
    button_res->attach_notification_callback(button_callback);

    pattern_res = client.create_resource("3201/0/5853", "blink_pattern");
    pattern_res->set_value("500:500:500:500:500:500:500:500");
    pattern_res->methods(M2MMethod::GET | M2MMethod::PUT);
    pattern_res->attach_put_callback(pattern_updated);

    MbedCloudClientResource *blink_res = client.create_resource("3201/0/5850", "blink_action");
    blink_res->methods(M2MMethod::POST);
    blink_res->attach_post_callback(blink_callback);

    printf("Initialized Mbed Cloud Client. Registering...\n");

    // Callback that fires when registering is complete
    client.on_registered(&registered);

    // Register with Mbed Cloud
    client.register_and_connect();

    // Setup the button 
      sw2.mode(PullUp);
    
    // The button fall handler is placed in the event queue so it will run in
    // thread context instead of ISR context, which allows safely updating the cloud resource         
      sw2.fall(eventQueue.event(&button_press));
      button_count = 0;

    // Start the event queue in a separate thread so the main thread continues
    thread1.start(callback(&eventQueue, &EventQueue::dispatch_forever));

    while(1)
    {
        wait_ms(100);

        if (button_pressed) {
            button_pressed = false;
            printf("button clicked %d times\r\n", button_count);            
        }
        
    }
}
