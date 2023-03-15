#include <ESP32Servo.h>
#include <WebServer.h>
#include <WiFi.h>

#define TYPE_TEXT "text/plain"
#define TYPE_HTML "text/html"

// 301 as redirect response must not be used due to caching
const int REDIRECT = 302;

const int FRONT_LED = 13;
const int BACK_LED = 15;
bool FRONT_LED_STATUS = LOW;
bool BACK_LED_STATUS = LOW;

const int DEFAULT_STEERING_ANGLE = 90; // 0 - 180: 90 is straight
const int SERVO_PIN = 14;
Servo servo;


const char *ssid = "ESP32-T1";
const char *password = "passwordd";

/* Put IP Address details */
IPAddress local_ip(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

void setup() {
	Serial.begin(115200);
	pinMode(FRONT_LED, OUTPUT);
	pinMode(BACK_LED, OUTPUT);
	
	servo.attach(SERVO_PIN);
	servo.write(DEFAULT_STEERING_ANGLE);

	WiFi.softAP(ssid, password);
	WiFi.softAPConfig(local_ip, gateway, subnet);
	delay(100);

	server.on("/", handle_root);
	server.on("/togglefront", handle_toggle_front);
	server.on("/toggleback", handle_toggle_back);
	server.on("/steering_dir", handle_steering);
	server.onNotFound(handle_NotFound);

	server.begin();
	Serial.println("HTTP server started");
}

void handle_steering() {
	if(server.hasArg("angle")) {
		int angle = DEFAULT_STEERING_ANGLE + server.arg("angle").toInt();
		Serial.print("Setting angle to ");
		Serial.println(angle);
		servo.write(angle);
		server.send(200, TYPE_TEXT, String(angle));
	} else {
		server.send(400, TYPE_TEXT, "param 'angle' missing");
	}
}

void loop() {
	server.handleClient();
	digitalWrite(FRONT_LED, FRONT_LED_STATUS);
	digitalWrite(BACK_LED, BACK_LED_STATUS);
}

void handle_root() {
	server.send(200, TYPE_HTML, sendHTML());
}

void handle_toggle_front() {
	FRONT_LED_STATUS = !FRONT_LED_STATUS;
	Serial.print("front: ");
	Serial.println(FRONT_LED_STATUS);
	server.sendHeader("location", "/", true);
	server.send(REDIRECT, TYPE_HTML, "");
}

void handle_toggle_back() {
	BACK_LED_STATUS = !BACK_LED_STATUS;
	Serial.print("back: ");
	Serial.println(BACK_LED_STATUS);
	server.sendHeader("location", "/", true);
	server.send(REDIRECT, TYPE_HTML, "");
}

void handle_NotFound() {
	server.send(404, TYPE_TEXT, "Not found");
}

String sendHTML() {
	  String ptr = \
"<!DOCTYPE html> <html>\n\
<head><meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'>\n\
	<title>ESP32 T1</title>\n\
	<style>\
		html { display: inline-block; margin: 0px auto; text-align: center;}\n\
		body {margin-top: 50px;} \
		h1 {color: #444444;margin: 50px auto 30px;}\
		h3 {color: #444444;margin-bottom: 50px;}\
		.button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n\
		.button-on {background-color: #3498db;}\n\
		.button-on:active {background-color: #2980b9;}\n\
		.button-off {background-color: #34495e;}\n\
		.button-off:active {background-color: #2c3e50;}\n\
		p {font-size: 14px;color: #888;margin-bottom: 10px;}\n\
	</style>\n\
</head>\n\
<body>\n\
	<h1>ESP32 T1 control</h1>\n\
	<h3>AP Mode</h3>\n";

	if (FRONT_LED_STATUS) {
		ptr += "<p>front LED: ON</p><a class='button button-off' href='/togglefront'>OFF</a>\n";
	} else {
		ptr += "<p>front LED: OFF</p><a class='button button-on' href='/togglefront'>ON</a>\n";
	}

	if (BACK_LED_STATUS) {
	ptr += "<p>back LED: ON</p><a class='button button-off' href='/toggleback'>OFF</a>\n";

	} else {
		ptr += "<p>back LED: OFF</p><a class='button button-on' href='/toggleback'>ON</a>\n";
	}

	ptr += "</body>\n</html>\n";

	return ptr;
}
