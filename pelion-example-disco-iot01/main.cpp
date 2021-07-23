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
#ifndef MBED_TEST_MODE

#include "mbed.h"
#include "simple-mbed-cloud-client.h"
#include "LittleFileSystem.h"
#include "math.h"

// Default network interface object. Don't forget to change the WiFi SSID/password in mbed_app.json if you're using WiFi.
NetworkInterface *net;

// Default block device available on the target board
BlockDevice* bd = BlockDevice::get_default_instance();
SlicingBlockDevice sd(bd, 0, 2*1024*1024);

Serial btdevice(PC_1, PC_0, 9600);  // tx, rx
#if COMPONENT_SD || COMPONENT_NUSD
// Use FATFileSystem for SD card type blockdevices
FATFileSystem fs("fs");
#else
// Use LittleFileSystem for non-SD block devices to enable wear leveling and other functions
LittleFileSystem fs("fs");
#endif

// Default User button for GET example and for resetting the storage
InterruptIn button(BUTTON1);
// Default LED to use for PUT/POST example
DigitalOut led(LED1, 1);    

// How often to fetch sensor data (in seconds)
#define SENSORS_POLL_INTERVAL 1.0

// Send all sensor data or just limited (useful for when running out of memory)
#define SEND_ALL_SENSORS

// Sensors related includes and initialization
#include "HTS221Sensor.h"

static DevI2C devI2c(PB_11,PB_10);
static HTS221Sensor sen_hum_temp(&devI2c);

// Temperature reading from microcontroller

// Voltage reference reading from microcontroller


// Declaring pointers for access to Pelion Client resources outside of main()
MbedCloudClientResource *res_button;
MbedCloudClientResource *res_led;

// Additional resources for sensor readings
#ifdef SEND_ALL_SENSORS
MbedCloudClientResource *res_humidity;
MbedCloudClientResource *res_temperature;
MbedCloudClientResource *res_dust;

#endif /* SEND_ALL_SENSORS */

// An event queue is a very useful structure to debounce information between contexts (e.g. ISR and normal threads)
// This is great because things such as network operations are illegal in ISR, so updating a resource in a button's fall() function is not allowed
EventQueue eventQueue;

// When the device is registered, this variable will be used to access various useful information, like device ID etc.
static const ConnectorClientEndpointInfo* endpointInfo;

//Dust Init
InterruptIn input(A0); // Change your pin name here
Timer t;    // measure amount of time signal is low
Ticker m;   // every 30 seconds calculate measurement
float dust;
float btdust;
// Start Timer
void start(void){
    t.start();
}

// Stop TImer
void stop(void){
    t.stop();
}

void calculate(){
    // run measurements
//    int lpo = t.read_us();
    float ratio = t.read_us()/(30000*10.0);
    float concentration = (1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62)/20;
    dust = concentration;
    //printf("pcs/0.01cf\r\n",concentration);
    
    // reset and run sensors
    t.reset(); // reset mesurement every 30 seconds
    input.fall(&start); // start measuring when signal is low
    input.rise(&stop);  // stop measuring when signal is high
}



/**
 * PUT handler
 * @param resource The resource that triggered the callback
 * @param newValue Updated value for the resource
 */
void put_callback(MbedCloudClientResource *resource, m2m::String newValue) {
    printf("*** PUT received, new value: %s                             \n", newValue.c_str());
    led = atoi(newValue.c_str());
}

/**
 * POST handler
 * @param resource The resource that triggered the callback
 * @param buffer If a body was passed to the POST function, this contains the data.
 *               Note that the buffer is deallocated after leaving this function, so copy it if you need it longer.
 * @param size Size of the body
 */
void post_callback(MbedCloudClientResource *resource, const uint8_t *buffer, uint16_t size) {
    printf("*** POST received (length %u). Payload: ", size);
    for (size_t ix = 0; ix < size; ix++) {
        printf("%02x ", buffer[ix]);
    }
    printf("\n");
}

/**
 * Button function triggered by the physical button press.
 */
void button_press() {
    int v = res_button->get_value_int() + 1;
    res_button->set_value(v);
    printf("*** Button clicked %d times                                 \n", v);
}

/**
 * Notification callback handler
 * @param resource The resource that triggered the callback
 * @param status The delivery status of the notification
 */
void button_callback(MbedCloudClientResource *resource, const NoticationDeliveryStatus status) {
    printf("*** Button notification, status %s (%d)                     \n", MbedCloudClientResource::delivery_status_to_string(status), status);
}

/**
 * Registration callback handler
 * @param endpoint Information about the registered endpoint such as the name (so you can find it back in portal)
 */
void registered(const ConnectorClientEndpointInfo *endpoint) {
    printf("Registered to Pelion Device Management. Endpoint Name: %s\n", endpoint->internal_endpoint_name.c_str());
    endpointInfo = endpoint;
}
/**
 * Initialize sensors
 */
void sensors_init() {
    uint8_t id1;

    printf ("\nSensors configuration:\n");
    // Initialize sensors
    sen_hum_temp.init(NULL);

    /// Call sensors enable routines
    sen_hum_temp.enable();
    m.attach(&calculate,5.0); // measure for 30 seconds

    printf("HTS221  humidity & temperature    = 0x%X\n", id1);

    printf("\n"); ;
}

/**
 * Update sensors and report their values.
 * This function is called periodically.
 */
void sensors_update() {
    float temp1_value, humid_value;

    sen_hum_temp.get_humidity(&humid_value);
    sen_hum_temp.get_temperature(&temp1_value);
    
    btdust = dust;
    btdevice.printf("%f\n",btdust);

    printf("                                                             \n");
    printf("                                                   \n");
    printf("HTS221 temp:  %7.3f C,  humidity: %7.2f %%         \n", temp1_value, humid_value);
    printf("Dust Concentration = %f ug/m^3                     \n",dust);
    printf("                                                     \n");
    printf("                                                     \n");
    printf("                                                     \n");
    printf("                                                     \n");

    printf("\r\033[8A");

    if (endpointInfo) {
#ifdef SEND_ALL_SENSORS
        res_humidity->set_value(humid_value);
        res_temperature->set_value(temp1_value);
        res_dust->set_value(dust);
#endif /* SEND_ALL_SENSORS */
    }
}

int main(void) {
    printf("\nStarting Simple Pelion Device Management Client example\n");

    int storage_status = fs.mount(&sd);
    if (storage_status != 0) {
        printf("Storage mounting failed.\n");
    }
    // If the User button is pressed ons start, then format storage.
    bool btn_pressed = (button.read() == MBED_CONF_APP_BUTTON_PRESSED_STATE);
    if (btn_pressed) {
        printf("User button is pushed on start...\n");
    }

    if (storage_status || btn_pressed) {
        printf("Formatting the storage...\n");
        int storage_status = StorageHelper::format(&fs, &sd);
        if (storage_status != 0) {
            printf("ERROR: Failed to reformat the storage (%d).\n", storage_status);
        }
    } else {
        printf("You can hold the user button during boot to format the storage and change the device identity.\n");
    }

    sensors_init();

    // Connect to the internet (DHCP is expected to be on)
    printf("Connecting to the network using Wifi...\n");
    net = NetworkInterface::get_default_instance();

    nsapi_error_t net_status = -1;
    for (int tries = 0; tries < 3; tries++) {
        net_status = net->connect();
        if (net_status == NSAPI_ERROR_OK) {
            break;
        } else {
            printf("Unable to connect to network. Retrying...\n");
        }
    }

    if (net_status != NSAPI_ERROR_OK) {
        printf("ERROR: Connecting to the network failed (%d)!\n", net_status);
        return -1;
    }

    printf("Connected to the network successfully. IP address: %s\n", net->get_ip_address());

    printf("Initializing Pelion Device Management Client...\n");

    // SimpleMbedCloudClient handles registering over LwM2M to Pelion DM
    SimpleMbedCloudClient client(net, bd, &fs);
    int client_status = client.init();
    if (client_status != 0) {
        printf("ERROR: Pelion Client initialization failed (%d)\n", client_status);
        return -1;
    }

    // Creating resources, which can be written or read from the cloud
    res_button = client.create_resource("3200/0/5501", "Button Count");
    res_button->set_value(0);
    res_button->methods(M2MMethod::GET);
    res_button->observable(true);
    res_button->attach_notification_callback(button_callback);

    res_led = client.create_resource("3201/0/5853", "LED State");
    res_led->set_value(1);
    res_led->methods(M2MMethod::GET | M2MMethod::PUT);
    res_led->attach_put_callback(put_callback);

#ifdef SEND_ALL_SENSORS
    // Sensor resources
    res_temperature = client.create_resource("3303/0/5700", "Temperature HTS221 (C)");
    res_temperature->set_value(0);
    res_temperature->methods(M2MMethod::GET);
    res_temperature->observable(true);

    res_humidity = client.create_resource("3304/0/5700", "Humidity");
    res_humidity->set_value(0);
    res_humidity->methods(M2MMethod::GET);
    res_humidity->observable(true);
    
    res_dust = client.create_resource("3305/0/5700", "Dust");
    res_dust->set_value(0);
    res_dust->methods(M2MMethod::GET);
    res_dust->observable(true);
#endif /* SEND_ALL_SENSORS */

    printf("Initialized Pelion Client. Registering...\n");

    // Callback that fires when registering is complete
    client.on_registered(&registered);

    // Register with Pelion DM
    client.register_and_connect();

    int i = 600; // wait up 60 seconds before attaching sensors and button events
    while (i-- > 0 && !client.is_client_registered()) {
        wait_ms(100);
    }

    button.fall(eventQueue.event(&button_press));

    // The timer fires on an interrupt context, but debounces it to the eventqueue, so it's safe to do network operations
    Ticker timer;
    timer.attach(eventQueue.event(&sensors_update), SENSORS_POLL_INTERVAL);

    // You can easily run the eventQueue in a separate thread if required
    eventQueue.dispatch_forever();
}

#endif