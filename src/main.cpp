#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h>
#include "esp_adc_cal.h"
#include <WiFiUdp.h>
#include "esp_wifi.h"

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23

#define TFT_BL              4   // Display backlight control pin
#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_1            35
#define BUTTON_2            0

#define MOTOR_1				25
#define MOTOR_2				26
#define MOTOR_3				27

const int motorFreq = 5000;
const int motorResolution = 8;
const int motorChannel1 = 0;
const int motorChannel2 = 1;
const int motorChannel3 = 2;

typedef	struct 	s_data_task
{
	int duration;
	int pwm;
	int motor_id;
	TaskHandle_t thisTaskHandler;
}				t_data_task;


// typedef void(*t_task_func)(void *param);



t_data_task g_data_task[3];

int vref = 1100;

#define BLACK 0x0000
#define WHITE 0xFFFF
TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);


// const char* ssid = "Matrice";
const char* ssid = "Freebox-0E3EAE";
// const char* password =  "QGmatrice";
const char* password =  "taigaest1chien";

const char *APssid = "tripodesAP";
const char *APpassword = "44448888";



WiFiUDP Udp;
unsigned int localUdpPort = 49141;
char incomingPacket[255];
String convertedPacket;
char  replyPacket[] = "Message received";


void set_pwm0(int pwm);
void set_pwm1(int pwm);
void set_pwm2(int pwm);

typedef void(*t_set_pwm)(int pwm);

#define TASK_NUMBER 3
static const t_set_pwm	g_set_pwm[TASK_NUMBER] = {
	(t_set_pwm)set_pwm0,
	(t_set_pwm)set_pwm1,
	(t_set_pwm)set_pwm2,
};

void stop_pwm0(void);
void stop_pwm1(void);
void stop_pwm2(void);

typedef void(*t_stop_pwm)(void);

#define TASK_NUMBER 3
static const t_stop_pwm	g_stop_pwm[TASK_NUMBER] = {
	(t_stop_pwm)stop_pwm0,
	(t_stop_pwm)stop_pwm1,
	(t_stop_pwm)stop_pwm2,
};



bool	timersActives[3];
int		pwmValues[3];
int		timerPansements[3];
hw_timer_t * timers[3];



const char* wl_status_to_string(int ah) {
	switch (ah) {
		case WL_NO_SHIELD: return "WL_NO_SHIELD";
		case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
		case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
		case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
		case WL_CONNECTED: return "WL_CONNECTED";
		case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
		case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
		case WL_DISCONNECTED: return "WL_DISCONNECTED";
		default: return "ERROR NOT VALID WL";
	}
}

const char* eTaskGetState_to_string(int ah) {
	switch (ah) {
		case eRunning: return "eRunning";
		case eReady: return "eReady";
		case eBlocked: return "eBlocked";
		case eSuspended: return "eSuspended";
		case eDeleted: return "eDeleted";
		default: return "ERROR NOT STATE";
	}
}




void showVoltage()
{
	static uint64_t timeStamp = 0;
	if (millis() - timeStamp > 1000) {
		timeStamp = millis();
		uint16_t v = analogRead(ADC_PIN);
		float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
		String voltage = "Voltage :" + String(battery_voltage) + "V";
		Serial.println(voltage);
		tft.fillScreen(TFT_BLACK);
		tft.setTextDatum(MC_DATUM);
		tft.drawString(voltage,  tft.width() / 2, tft.height() / 2 );
	}
}

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
	
	tft.init();
	tft.setRotation(0);
	tft.fillScreen(TFT_BLACK);
	if (TFT_BL > 0) {                           // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
			pinMode(TFT_BL, OUTPUT);                // Set backlight pin to output mode
			digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
	}

    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
	
	
	WiFi.mode(WIFI_AP);
	WiFi.softAP(APssid, APpassword);
	IPAddress myIP = WiFi.softAPIP();
Serial.print("AP IP address: ");
Serial.println(myIP);
	tft.printf("AP addr: %s\n", myIP.toString().c_str());



}




void drawNetworkActivity()
{
	TFT_eSprite drawing_sprite = TFT_eSprite(&tft);

  drawing_sprite.setColorDepth(8);
  drawing_sprite.createSprite(tft.width(), tft.height());
	drawing_sprite.fillSprite(TFT_BLACK);
	    drawing_sprite.setTextSize(1);
		drawing_sprite.setTextFont(1);
    drawing_sprite.setTextDatum(MC_DATUM);
	drawing_sprite.setCursor(0, 0);
	uint16_t v = analogRead(ADC_PIN);
	float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    drawing_sprite.setTextColor(TFT_RED);
	drawing_sprite.printf("AP SSID: ");
    drawing_sprite.setTextColor(TFT_WHITE);
	drawing_sprite.printf("%s\n", APssid);
    drawing_sprite.setTextColor(TFT_RED);
	drawing_sprite.printf("AP PSSWD: ");
    drawing_sprite.setTextColor(TFT_WHITE);
	drawing_sprite.printf("%s\n", APpassword);
    drawing_sprite.setTextColor(TFT_RED);
	drawing_sprite.printf("Voltage : ");
    drawing_sprite.setTextColor(TFT_WHITE);
	drawing_sprite.printf("%.2fv\n", battery_voltage);
    drawing_sprite.setTextColor(TFT_RED);
	drawing_sprite.printf("Connected clients : ");
    drawing_sprite.setTextColor(TFT_WHITE);
	drawing_sprite.printf("%d\n", WiFi.softAPgetStationNum());

	
	  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;
 
  memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
 
  esp_wifi_ap_get_sta_list(&wifi_sta_list);
  tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
 
  for (int i = 0; i < adapter_sta_list.num; i++) {

    drawing_sprite.setTextColor(TFT_YELLOW);

	  drawing_sprite.println("");
 
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
 
    drawing_sprite.setTextColor(TFT_BLUE);

    drawing_sprite.print("MAC: ");
    drawing_sprite.setTextColor(TFT_WHITE);
 
    for(int i = 0; i< 6; i++){
      
      drawing_sprite.printf("%02X", station.mac[i]);  
      if(i<5)drawing_sprite.print(":");
    }
    drawing_sprite.setTextColor(TFT_BLUE);
 
    drawing_sprite.print("\nIP: ");  
    drawing_sprite.setTextColor(TFT_WHITE);

    drawing_sprite.println(ip4addr_ntoa(&(station.ip)));    
  }
 




	drawing_sprite.pushSprite(0, 0);
	drawing_sprite.deleteSprite();
	
}





void loop() {
  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;
 
  memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
 
  esp_wifi_ap_get_sta_list(&wifi_sta_list);
  tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
 
  for (int i = 0; i < adapter_sta_list.num; i++) {
 
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
 
    Serial.print("station nr ");
    Serial.println(i);
    Serial.print("MAC: ");
 
    for(int i = 0; i< 6; i++){
      
      Serial.printf("%02X", station.mac[i]);  
      if(i<5)Serial.print(":");
    }
 
    Serial.print("\nIP: ");  
    Serial.println(ip4addr_ntoa(&(station.ip)));    
  }
 
  Serial.println("-----------");
  drawNetworkActivity();
  delay(5000);
	// put your main code here, to run repeatedly:
}