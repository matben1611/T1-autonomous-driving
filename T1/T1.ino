#include <ESP32Servo.h>
#include <WebServer.h>
#include <WiFi.h>

#define HTTP_OKAY 200
#define HTTP_USER_ERROR 400
#define HTTP_NOT_FOUND 404

#define TYPE_TEXT "text/plain"
#define TYPE_HTML "text/html"

#define FRONT_LED 13
#define BACK_LED 15
bool FRONT_LED_STATUS = LOW;
bool BACK_LED_STATUS = LOW;

#define DEFAULT_STEERING_ANGLE 90 // 0 - 180: 90 is straight
#define SERVO_PIN 32
Servo servo;

#define MOTOR_PIN_1 16
#define MOTOR_PIN_2 17
#define MOTOR_PIN_1_CHANNEL 14
#define MOTOR_PIN_2_CHANNEL 15
#define MOTOR_FREQUENCY 50
#define MOTOR_RESOLUTION 8
#define MAX_PWM_VALUE ((1 << MOTOR_RESOLUTION) - 1)


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
	
	ledcAttachPin(MOTOR_PIN_1, MOTOR_PIN_1_CHANNEL);
	ledcAttachPin(MOTOR_PIN_2, MOTOR_PIN_2_CHANNEL);
	ledcSetup(MOTOR_PIN_1_CHANNEL, MOTOR_FREQUENCY, MOTOR_RESOLUTION);
	ledcSetup(MOTOR_PIN_2_CHANNEL, MOTOR_FREQUENCY, MOTOR_RESOLUTION);
	ledcWrite(MOTOR_PIN_1_CHANNEL, 0);
	ledcWrite(MOTOR_PIN_2_CHANNEL, 0);

	WiFi.softAP(ssid, password);
	WiFi.softAPConfig(local_ip, gateway, subnet);
	delay(100);

	server.on("/", handle_root);
	server.on("/togglefront", handle_toggle_front);
	server.on("/toggleback", handle_toggle_back);
	server.on("/steering", handle_steering);
	server.on("/motor", handle_speed);
	server.onNotFound(handle_NotFound);

	server.begin();
	Serial.println("HTTP server started");
}

int normalize(int input) {
	// to either 1 or -1
	if(abs(input) > MAX_PWM_VALUE) {
		if(input >= 0) {
			input = MAX_PWM_VALUE;
		} else {
			input = -MAX_PWM_VALUE;
		}
	}
	return input;
}

String printBool(bool b) {
	return b ? "true" : "false";
}

void handle_speed() {
	if(server.hasArg("speed")) {
		int speed = server.arg("speed").toInt();
		speed = normalize(speed);
		Serial.print("setting speed to ");
		Serial.println(speed);
		
		int forward = 0;
		int backward = 0;
		
		if(speed > 0) {
			forward = speed;
		} else if(speed < 0) {
			backward = abs(speed);
		}
		
		ledcWrite(MOTOR_PIN_1_CHANNEL, forward);
		ledcWrite(MOTOR_PIN_2_CHANNEL, backward);
		
		server.send(HTTP_OKAY, TYPE_TEXT, String(speed));
	} else {
		server.send(HTTP_USER_ERROR, TYPE_TEXT, "param 'speed' missing");
	}
}

void handle_steering() {
	if(server.hasArg("angle")) {
		int angle = DEFAULT_STEERING_ANGLE + server.arg("angle").toInt();
		Serial.print("setting angle to ");
		Serial.println(angle);
		servo.write(angle);
		server.send(HTTP_OKAY, TYPE_TEXT, String(angle));
	} else {
		server.send(HTTP_USER_ERROR, TYPE_TEXT, "param 'angle' missing");
	}
}

void loop() {
	server.handleClient();
	digitalWrite(FRONT_LED, FRONT_LED_STATUS);
	digitalWrite(BACK_LED, BACK_LED_STATUS);
}

void handle_root() {
	server.send(HTTP_OKAY, TYPE_HTML, sendHTML());
}

void handle_toggle_front() {
	FRONT_LED_STATUS = !FRONT_LED_STATUS;
	Serial.print("front: ");
	Serial.println(FRONT_LED_STATUS);
	server.send(HTTP_OKAY, TYPE_TEXT, printBool(FRONT_LED_STATUS));
}

void handle_toggle_back() {
	BACK_LED_STATUS = !BACK_LED_STATUS;
	Serial.print("back: ");
	Serial.println(BACK_LED_STATUS);
	server.send(HTTP_OKAY, TYPE_TEXT, printBool(BACK_LED_STATUS));
}

void handle_NotFound() {
	server.send(HTTP_NOT_FOUND, TYPE_TEXT, "Not found");
}

String sendHTML() {
	return "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'><title>ESP32 T1</title><style>html{margin:0px auto; text-align:center;background-color:#000;color:#FFF}.button{min-width:40px; background-color:#000; border:#FF4500 solid 1px; border-radius:4px; padding:13px 30px; font-size:25px; cursor:pointer; transition:background-color 0.2s,color 0.2s;}.button:hover{background-color:#FF4500; color:#000;}.grid{display:grid; row-gap:40px;}.wide-grid{grid-template-columns:auto auto auto; max-width:450px;}.small-grid{grid-template-columns:auto auto; max-width:300px;}.centered{margin:auto; margin-top:20px;}p{margin:0px;}span,p{font-size:30px;}h2{margin-top:60px; margin-bottom:0px;}</style><script>function getRequest(to,target){let req=new XMLHttpRequest();req.open('GET',to,false);req.send(null);if(target!=null){document.getElementById(target).innerHTML=req.responseText;}}</script></head><body><h1>ESP32 T1 control</h1><h2>Lights</h2><div class='centered grid small-grid'><div><span class='button' onclick='getRequest(\"/togglefront\",\"lights-front\")'>front</span></div><span id='lights-front'>false</span><div><span class='button' onclick='getRequest(\"/toggleback\",\"lights-back\")'>back</span></div><span id='lights-back'>false</span></div><h2>Steering</h1><div><p id='steer'>90</p><div class='centered grid wide-grid'><div><span class='button' onclick='getRequest(\"/steering?angle=-90\",\"steer\")'>left</span></div><div><span class='button' onclick='getRequest(\"/steering?angle=0\",\"steer\")'>straight</span></div><div><span class='button' onclick='getRequest(\"/steering?angle=90\",\"steer\")'>right</span></div></div></div><h2>Motor</h2><div><p id='speed'>0</p><div class='centered grid wide-grid'><div><span class='button' onclick='getRequest(\"/motor?speed=255\",\"speed\")'>forward</span></div><div><span class='button' onclick='getRequest(\"/motor?speed=0\",\"speed\")'>stop</span></div><div><span class='button' onclick='getRequest(\"/motor?speed=-255\",\"speed\")'>backward</span></div></div></div></body></html>";
}
