/*
* Module_02_ESP-NOW_TwoWayCom.ino     
*
*/


// --> Load libraries
#include <esp_now.h> 
#include <WiFi.h> 
#include <Wire.h> 
#include <Adafruit_BMP280.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Commom_structs_functions.h"




/******************************************
** Constants and Global scope variables  **
******************************************/
#define OLED_RESET -1 // My OLED display don't have a 'Reset' pin
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define I2C_ADDRESS_OLED 0x3C // I2C address of the OLED display

#define I2C_BMP280 0x76

// MAC Address of the FIRST ESP32 module
const uint8_t MAC_MODULE1[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Wi-Fi channel
#define CHAN_AP 2 

// Global scope variables
int received_bytes = 0, sent_packs = 0;
//String str_date = "", str_time = "";
RTC_Data obj_rtc_data; //Date time and temperature
BMP280_Data obj_bmp280_data; //Dados a serem recebidos
String status_msg = "--";





/********************************************************* 
** Objects o control the devices connected to the ESP32 **   
*********************************************************/ 
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Objeto para controlar o display OLED
Adafruit_BMP280 bmp; //BMP280




/******************************
** Function prototypes       **
*****************************/ 
// Callback functions:
void fcalback_data_sent( const uint8_t *mac_addr, esp_now_send_status_t deliv_status );
void fcalback_data_received(const uint8_t * mac, const uint8_t *received_data, int len);


// Auxiliary functions
void print_bmp280_data_in_serial( RTC_Data *p_rtc_data );
void read_bmp280_data( BMP280_Data *p_bmp_data );
void print_data_oled_display( RTC_Data struct_rtc, int recv, int sent, String msg );





/******************
** setup()       ** 
******************/
void setup() {
      // Give initial values to the fields of 'obj_rtc_data'
      obj_rtc_data.dy = 0; obj_rtc_data.dm = 0; obj_rtc_data.dd = 0;
      obj_rtc_data.hh = 0; obj_rtc_data.hm = 0; obj_rtc_data.hs = 0;
      
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
      oled.setCursor(0, 10); oled.print("BMP280: ");
      oled.display();


      //  Initialize the BMP280:
      Serial.print( "Initializing the BMP280 module..." );
      if( !bmp.begin( I2C_BMP280 ) ){  Serial.println("ERROR");  oled.print("--");  }
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
      memcpy(peerInfo.peer_addr, MAC_MODULE1, 6);
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
     print_data_oled_display(obj_rtc_data, received_bytes, sent_packs, status_msg);
}




/****************
** loop()      **  
****************/
void loop() {
      // Read the data in the RTC:
      read_bmp280_data( &obj_bmp280_data );


      // Send the data using the ESP-NOW
      esp_err_t result = esp_now_send(MAC_MODULE1, (uint8_t *) &obj_bmp280_data, sizeof(obj_bmp280_data));

      // Avisar se a mensagem foi enviada com sucesso ou nao:
      if( result == ESP_OK ){ Serial.println("Message successfully sent!"); }
      else{ Serial.println("Error in sending the message"); }


      // --> Write the number of received bytes in the OLED display
      print_bmp280_data_in_serial( &obj_bmp280_data );
      print_data_oled_display(obj_rtc_data, received_bytes, sent_packs, status_msg);
      
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
      memcpy(&obj_rtc_data, received_data, sizeof(obj_rtc_data));
      // Number o received bytes:
      Serial.print("Bytes received: "); Serial.println(len);

      // --> Size of the received pack:
      received_bytes = len;

      // --> I removed from here. It messes my display
      //print_data_oled_display(obj_rtc_data, received_bytes, sent_packs, status_msg);
}




/**************************
** Auxiliary functions   **
**************************/
// --> Function which prints the date and time in the monitor serial
void print_bmp280_data_in_serial( BMP280_Data *p_bmp_data ){
      Serial.print("Temp = "); Serial.print(p_bmp_data->t);
      Serial.print(";\tPress. = "); Serial.print(p_bmp_data->p);
      Serial.println(" hPa");
}



void read_bmp280_data( BMP280_Data *p_bmp_data ){
      p_bmp_data->t = (float) bmp.readTemperature();
      p_bmp_data->p = (float) bmp.readPressure() / 100.0F;
      
     
}

void print_data_oled_display( RTC_Data struct_rtc, int recv, int sent, String msg ){
      String str_date = get_formatted_date(&struct_rtc );
      String str_time = get_formatted_hour(&struct_rtc);
      
      // Clean the display content
      oled.clearDisplay();
      oled.setTextColor(WHITE);
      oled.setTextSize(1);

      oled.setCursor(0, 0); 
      oled.print("RTC Data:");

      oled.setCursor(0, 10); 
      oled.print(str_date);

      oled.setCursor(0, 20); 
      oled.print(str_time);

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
