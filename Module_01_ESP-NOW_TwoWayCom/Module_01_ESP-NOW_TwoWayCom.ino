/*
* Module_01_ESP-NOW_TwoWayCom.ino     
*
*/


// --> Load libraries
#include <esp_now.h> 
#include <WiFi.h> 
#include <Wire.h> 
#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Commom_structs_functions.h"




/******************************************
** Constants and Global scope variables  **
******************************************/
// Parametros do display OLED
#define OLED_RESET -1 // My OLED display don't have a 'Reset' pin
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define I2C_ADDRESS_OLED 0x3C // I2C address of the OLED display

// MAC Address of the second ESP32 module
const uint8_t MAC_MODULE2[] = {0x80, 0x7D, 0x3A, 0xBA, 0xE0, 0xAC };

// Wi-Fi channel
#define CHAN_AP 2 

// Global scope variables
int received_bytes = 0, sent_packs = 0;
//String str_date = "", str_time = "";
RTC_Data obj_rtc_data; //Hora e temperatura
BMP280_Data obj_bmp280_data; //Dados a serem recebidos
String status_msg = "--";





/********************************************************* 
** Objects o control the devices connected to the ESP32 **   
*********************************************************/ 
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Objeto para controlar o display OLED
RTC_DS3231 rtc; //Real Time Clock DS3231




/******************************
** Function prototypes       **
*****************************/ 
// Callback functions:
void fcalback_data_sent( const uint8_t *mac_addr, esp_now_send_status_t deliv_status );
void fcalback_data_received(const uint8_t * mac, const uint8_t *received_data, int len);


// Auxiliary functions
void print_rtc_data_in_serial( RTC_Data *p_rtc_data );
void read_rtc_data( RTC_Data *p_rtc_data );
void print_data_oled_display( BMP280_Data struct_bmp, int recv, int sent, String msg );





/******************
** setup()       ** 
******************/
void setup() {
      // Give initial values to the fields of 'obj_bmp280_data'
      obj_bmp280_data.t = -1.23; obj_bmp280_data.p = -1.23;
      
      //Activate the monitor serial
      Serial.begin( 115200 ); 
      delay(3000);
      
      // Initialize the OLED display
      Serial.print( "Initializing the OLED display SSD1306..." );
      oled.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS_OLED);
      delay(500); Serial.println( "Ok" );

      // Write the OLED  display status:
      oled.clearDisplay(); oled.setTextSize(1); oled.setTextColor(WHITE);
      oled.setCursor(0, 0); oled.print("Module 1");
      oled.setCursor(0, 10); oled.print("RTC: ");
      oled.display();


      //  Initialize the RTC:
      Serial.print( "Initializing the RTC DS3231..." );
      if( !rtc.begin() ){  Serial.println("ERROR");  oled.print("--");  }
      else{  delay(500); Serial.println("OK"); oled.print("OK");  }
      oled.display();



      // Initialize the ESP-NOW:
      Serial.print( "Initializing the ESP-NOW..." ); 
      oled.setCursor(0, 20); oled.print("ESP-NOW: ");
      WiFi.mode(WIFI_STA);
      if( esp_now_init() != ESP_OK ){ 
            Serial.println("ERROR");  
            oled.print("--");
            return;
      }
      else{ 
            delay(500);
            oled.print("OK"); 
            Serial.println("OK"); 
     }
      

     // Register the callback function for sending data
     esp_now_register_send_cb(fcalback_data_sent);
     Serial.print( "Registering the callback function for SENDING data..." );
     oled.setCursor(0, 30); oled.print("Callback S: "); 
     Serial.print("Registering the peer device...");
     delay(500); Serial.println("OK");


     // Register the peer device which will receive the data:
      esp_now_peer_info_t peerInfo;
      memcpy(peerInfo.peer_addr, MAC_MODULE2, 6);
      peerInfo.channel = CHAN_AP;  
      peerInfo.encrypt = false;

      Serial.print("Registering the peer device...");
      oled.setCursor(0, 40); oled.print("peerInfo: ");
      
      // Adicionar o receptor:
      if( esp_now_add_peer(&peerInfo) != ESP_OK ){
            Serial.println("ERRO!");
            return;
      }
      delay(500); Serial.println("OK"); oled.print("OK");


      // Register the callback function for receiving data
     esp_now_register_recv_cb(fcalback_data_received);
     Serial.print( "Registering the callback function for RECEIVING data..." );
     oled.setCursor(0, 50); oled.print("Callback R: "); 
     Serial.print("Registering the peer device...");
     delay(500); Serial.println("OK");

      Serial.println("------------------------\n\n");
      delay(1000);


     // Write the initial values in the oled display
     print_data_oled_display(obj_bmp280_data, received_bytes, sent_packs, status_msg);
}




/****************
** loop()      **  
****************/
void loop() {
      // Read the data in the RTC:
      read_rtc_data( &obj_rtc_data );

      // Write the RTC data in the monitor serial:
      print_rtc_data_in_serial( &obj_rtc_data );


      // Send the data using the ESP-NOW
      esp_err_t result = esp_now_send(MAC_MODULE2, (uint8_t *) &obj_rtc_data, sizeof(obj_rtc_data));

      if( result == ESP_OK ){ Serial.println("Message successfully sent!"); }
      else{ Serial.println("Error in sending the message"); }

      // --> Write the number of received bytes in the OLED display
      print_rtc_data_in_serial(&obj_rtc_data);
      print_data_oled_display(obj_bmp280_data, received_bytes, sent_packs, status_msg);
      
      // Wait 5s before sending a new message
      delay(5000);
}




/*************************
** Callback functions   **
*************************/
// --> Function called whenever the data is sent
void fcalback_data_sent( const uint8_t *mac_addr, esp_now_send_status_t deliv_status ){
      Serial.print("\r\nLast Packet Send Status:\t");
      // Update the global variable 'status_msg'
      if( deliv_status == ESP_NOW_SEND_SUCCESS ){
            Serial.println("Delivery Success");

            // Update 'status_msg' and 'sent_packs'
            status_msg = "OK";
            ++sent_packs;
      }
      else {
            Serial.println("Delivery Fail");
            // Update 'status_msg'
            status_msg = "--";
      }
}




// --> Function called whenever the data is received
void fcalback_data_received(const uint8_t * mac, const uint8_t *received_data, int len){
      // Copy 'received_data' content to 'last_picture'
      memcpy(&obj_bmp280_data, received_data, sizeof(obj_bmp280_data));
      // Number o received bytes:
      Serial.print("Bytes received: "); Serial.println(len);

      // --> Size of the received pack:
      received_bytes = len;

      // --> I removed from here. It messes my display
      //print_data_oled_display(obj_bmp280_data, received_bytes, sent_packs, status_msg);
}




/**************************
** Auxiliary functions   **
**************************/
// --> Function which prints the date and time in the monitor serial
void print_rtc_data_in_serial( RTC_Data *p_rtc_data ){
      String str_date = get_formatted_date(p_rtc_data );
      String str_time = get_formatted_hour(p_rtc_data);

      Serial.print("Date = "); Serial.print(str_date); 
      Serial.print(";\tTime = "); Serial.print(str_time);
      Serial.print(";\tTemp = "); Serial.print(p_rtc_data->t);
      Serial.println("ºC"); 
}



void read_rtc_data( RTC_Data *p_rtc_data ){
      DateTime time_now = rtc.now();
      
      p_rtc_data->hh = (uint8_t) time_now.hour();
      p_rtc_data->hm = (uint8_t) time_now.minute();
      p_rtc_data->hs = (uint8_t) time_now.second();
      p_rtc_data->dy = (uint16_t) time_now.year();
      p_rtc_data->dm = (uint8_t) time_now.month();
      p_rtc_data->dd = (uint8_t) time_now.day();

      // temperature
      p_rtc_data->t = (float) rtc.getTemperature();
}

void print_data_oled_display( BMP280_Data struct_bmp, int recv, int sent, String msg ){
      // Clean the display content
      oled.clearDisplay();
      oled.setTextColor(WHITE);
      oled.setTextSize(1);

      oled.setCursor(0, 0); 
      oled.print("BMP280 Data:");

      oled.setCursor(0, 10); 
      oled.print("t = ");
      oled.print(struct_bmp.t);
      oled.print(char(247));
      oled.print("C");

      oled.setCursor(0, 20); 
      oled.print("p = ");
      oled.print(struct_bmp.p);
      oled.print(" hPa");

      oled.setCursor(0, 30); 
      oled.print("recv: ");
      oled.print(recv);
      oled.print(" bytes");
      
      oled.setCursor(0, 40); 
      oled.print("sent packs: ");
      oled.print(sent);

      oled.setCursor(0, 50); 
      oled.print("status: ");
      oled.print(msg);
      oled.display();
}
