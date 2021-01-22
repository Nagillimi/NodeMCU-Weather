/* Ben Milligan, June 3, 2020. WiFi Temperature/Humidity/Pressure System rev2. Sends all data
 *  to a Google sheet. Removed the photoresistor since the analog read messes with 
 *  the WiFi connection...
 *  
 *  Inludes:
 *  - 1 DHT11
 *  - 1 DHT22
 *  - 1 BMP280
 *  - 1 Tricolour LED
 * 
 *  Pressure Correction Calculation
 *  
 *  CF = [760ft - (Altitude x 0.026)] ÷ 760ft
 *  CF = [760ft - ((375m)(3.28084ft/m) x 0.026)] ÷ 760ft
 *  CF = 0.9579102763
 *  For Sea Level Correction -> 1/CF = 1.0439391086
 *  
 *  < 966.5hPa 
 *    STORMY
 */
#include <DHT.h>
#include <BMP280_DEV.h>
#include <ESP8266WiFi.h>

#define dht_outside_pin D6
#define dht_inside_pin D2
#define red_pin D3
#define green_pin D4
#define blue_pin D5
#define bmp_SCL_pin D1
#define bmp_SDA_pin D7

const double pressure_correction = 1.0439391086;

enum {OFF, RED_LED, ORANGE_LED, YELLOW_LED, GREEN_LED, BLUE_LED, PURPLE_LED};
float out_temp_val, out_hum_val, in_temp_val, in_hum_val, pressure;
String out_temp, out_hum, in_temp, in_hum, pressure_string;
const char *ssid = "Stan";
const char *password = "SeVw14919";
const char *host = "api.pushingbox.com";
const int http_port = 80;
String url = "/pushingbox?devid=v9443173C2E0277A"; // HAS A DAILY LIMIT OF 100/DAY!!!!

DHT dht_outside(dht_outside_pin, DHT22);
DHT dht_inside(dht_inside_pin, DHT11); // The DHT11 unit works for now

BMP280_DEV bmp(bmp_SDA_pin, bmp_SCL_pin); // GPIO for SDA, SCL coms

void setup() {  
  display_LED_colour(ORANGE_LED);
  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);

  WiFi.mode(WIFI_OFF); // helps with reconnection issues
  delay(1000);
  WiFi.mode(WIFI_STA); // Prevents hotspot
  
  Serial.begin(115200);
  dht_outside.begin();
  dht_inside.begin();
  
  WiFi.begin(ssid, password);
  Serial.println();
  while(WiFi.status() != WL_CONNECTED) {
      display_LED_colour(OFF);
      delay(250);
      Serial.print(".");
      display_LED_colour(ORANGE_LED);
      delay(250);
  }
  display_LED_colour(GREEN_LED);
  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  delay(1000);
  bmp.begin(SLEEP_MODE, BMP280_I2C_ALT_ADDR); // 0x76 for the BMP over I2C
}

void loop() {
  display_LED_colour(BLUE_LED);

  sense_DHT(1); // outside
  delay(1000);
  sense_DHT(2); // inside
  delay(1000);
  sense_BMP_Pressure();
    
  make_string();
  url += "&temp_out=" + out_temp;
  url += "&hum_out=" + out_hum;
  url += "&temp_in=" + in_temp;
  url += "&hum_in=" + in_hum;
  url += "&pressure=" + pressure_string;
  
  WiFiClient client;
  
  if (!client.connect(host, http_port)) {
    display_LED_colour(OFF);
    delay(500);
    Serial.print(".");
    display_LED_colour(RED_LED);
    delay(500);
    return;
  }
  
  Serial.println("Requesting URL: ");
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  Serial.println(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 4000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      display_LED_colour(RED_LED);
      delay(100);
      display_LED_colour(OFF);
      delay(100);
      display_LED_colour(RED_LED);
      delay(100);
      display_LED_colour(OFF);
      delay(100);
      display_LED_colour(RED_LED);
      delay(100);
      display_LED_colour(OFF);
      delay(100);
      return;
    }
  }
  
  display_LED_colour(OFF);
  delay(1200000); // Every 20mins
}

void display_LED_colour(int input) {
  switch (input) {
    case OFF :
      digitalWrite(green_pin, LOW);
      digitalWrite(blue_pin, LOW);
      digitalWrite(red_pin, LOW);
      break;
    case RED_LED :
      digitalWrite(green_pin, LOW);
      digitalWrite(blue_pin, LOW);
      digitalWrite(red_pin, LOW);
      // set colour
      digitalWrite(red_pin, HIGH);
      break;
    case ORANGE_LED :
      digitalWrite(green_pin, LOW);
      digitalWrite(blue_pin, LOW);
      digitalWrite(red_pin, LOW);
      // set colour
      digitalWrite(red_pin, HIGH);
      analogWrite(green_pin, 664);
      break;
    case YELLOW_LED :
      digitalWrite(green_pin, LOW);
      digitalWrite(blue_pin, LOW);
      digitalWrite(red_pin, LOW);
      // set colour
      digitalWrite(blue_pin, HIGH);
      digitalWrite(green_pin, HIGH);
      break;
    case GREEN_LED :
      digitalWrite(green_pin, LOW);
      digitalWrite(blue_pin, LOW);
      digitalWrite(red_pin, LOW);
      // set colour
      digitalWrite(green_pin, HIGH);
      break;
    case BLUE_LED :
      digitalWrite(green_pin, LOW);
      digitalWrite(blue_pin, LOW);
      digitalWrite(red_pin, LOW);
      // set colour
      digitalWrite(blue_pin, HIGH);
      break;
    case PURPLE_LED : 
      digitalWrite(green_pin, LOW);
      digitalWrite(blue_pin, LOW);
      digitalWrite(red_pin, LOW);
      // set colour
      analogWrite(red_pin, 511);
      analogWrite(blue_pin, 511);
      break;
  }
}

void sense_DHT(int state) {
  if(state == 1) {
    out_temp_val = dht_outside.readTemperature();
    out_hum_val = dht_outside.readHumidity();
  }
  if(state == 2) {
    in_temp_val = dht_inside.readTemperature();
    in_hum_val = dht_inside.readHumidity();
  }
}

void sense_BMP_Pressure() {
  bmp.startForcedConversion();
  while(!bmp.getPressure(pressure)) {;}
  bmp.stopConversion();
  pressure *= pressure_correction; // Normalized to 101325 Pa (sea lvl)
}

void make_string() {
  out_temp = String(out_temp_val);
  out_hum = String(out_hum_val);
  in_temp = String(in_temp_val);
  in_hum = String(in_hum_val);
  pressure_string = String(pressure);
}
