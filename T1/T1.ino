#include <ESP32Servo.h>
#include <WebServer.h>
#include <WiFi.h>
#include <TM1637Display.h>

#define CLK 25
#define DIO 26
TM1637Display display = TM1637Display(CLK, DIO);

#define HTTP_OKAY 200
#define HTTP_USER_ERROR 400
#define HTTP_NOT_FOUND 404

#define TYPE_TEXT "text/plain"
#define TYPE_HTML "text/html"

#define TRIGGER 18
#define ECHO 19
#define BUZZER 14

#define MIN_DELAY 20
#define MAX_DELAY 500

#define FRONT_LED 13
#define BACK_LED 15
bool FRONT_LED_STATUS = LOW;
bool BACK_LED_STATUS = LOW;

#define DEFAULT_STEERING_ANGLE 90 // 0 - 180: 90 is straight
#define STEERING_OFFSET 90
#define MAX_STEERING_ANGLE 30
#define SERVO_PIN 32
Servo servo;
int angle = DEFAULT_STEERING_ANGLE;

#define MOTOR_PIN_1 16
#define MOTOR_PIN_2 17
#define MOTOR_PIN_1_CHANNEL 14
#define MOTOR_PIN_2_CHANNEL 15
#define MOTOR_FREQUENCY 50
#define MOTOR_RESOLUTION 8
#define MAX_MOTOR_PWM ((1 << MOTOR_RESOLUTION) - 1)
int speed = 0;

#define LIGHT_ARG "where"
#define SPEED_ARG "speed"
#define STEERING_ARG "angle"

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
  pinMode(BUZZER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(TRIGGER, OUTPUT);
	
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
	server.on("/", handle_root);
	server.on("/lights", handle_lights);
	server.on("/steering", handle_steering);
	server.on("/motor", handle_speed);
	server.onNotFound(send_not_found);

	server.begin();
	Serial.println("HTTP server started");

  display.setBrightness(7);

}


void loop() {
	server.handleClient();
	digitalWrite(FRONT_LED, FRONT_LED_STATUS);
	digitalWrite(BACK_LED, BACK_LED_STATUS);

  handle_buzzer();
  update_display();
}

void update_display() {
 display.showNumberDec(speed);
}

void handle_buzzer() {
  int delay_time = MAX_DELAY;
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);
  
  int distance_cm = pulseIn(ECHO, HIGH) * 0.017;

  if(distance_cm > 40) {
    digitalWrite(BUZZER, LOW);
  } else if(distance_cm < 5) {
    digitalWrite(BUZZER, HIGH);
  } else {
    delay_time = map(distance_cm, 5, 40, MIN_DELAY, MAX_DELAY);
    digitalWrite(BUZZER, HIGH);
    delay(delay_time);
    digitalWrite(BUZZER, LOW);
  }
  
  delay(delay_time);
}

void handle_root() {
	server.send(HTTP_OKAY, TYPE_HTML, sendHTML());
}

String printBool(bool b) {
	return b ? "true" : "false";
}

bool is_relative() {
	return server.hasArg("mode") && server.arg("mode") == "relative";
}

int normalizeMotorPWM(int input) {
	input = constrain(input, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);
	return input;
}

int normalizeSteeringAngle(int input) {
	input = constrain(input, -MAX_STEERING_ANGLE + STEERING_OFFSET, MAX_STEERING_ANGLE + STEERING_OFFSET);
	return input;
}

void handle_speed() {
	if(!server.hasArg(SPEED_ARG)) {
		arg_missing(SPEED_ARG);
		return;
	}
	
	set_speed(server.arg(SPEED_ARG).toInt(), is_relative());
	
	Serial.print("setting speed to ");
	Serial.println(speed);
	
	server.send(HTTP_OKAY, TYPE_TEXT, String(speed));
}

void set_speed(int value, bool relative) {
	if(relative) {
		speed += value;
	} else {
		speed = value;
	}
	
	speed = normalizeMotorPWM(speed);
	
	int forward = 0;
	int backward = 0;
	
	if(speed > 0) {
		forward = speed;
	} else if(speed < 0) {
		backward = abs(speed);
	}
	
	ledcWrite(MOTOR_PIN_1_CHANNEL, forward);
	ledcWrite(MOTOR_PIN_2_CHANNEL, backward);
}

void handle_steering() {
	if(!server.hasArg(STEERING_ARG)) {
		arg_missing(STEERING_ARG);
		return;
	}
	
	set_angle(server.arg(STEERING_ARG).toInt(), is_relative());
	
	server.send(HTTP_OKAY, TYPE_TEXT, String(angle));
}

void set_angle(int value, bool relative) {
	if(relative) {
		angle += value;
	} else {
		angle = DEFAULT_STEERING_ANGLE + value;
	}
	
	angle = normalizeSteeringAngle(angle);
	Serial.print("setting angle to ");
	Serial.println(angle);
	servo.write(angle);
}

void handle_lights() {
	if(!server.hasArg(LIGHT_ARG)) {
		arg_missing(LIGHT_ARG);
		return;
	}
	
	String where = server.arg(LIGHT_ARG);
	if(where == "front") {
		change_front_led();
	} else if(where == "back") {
		change_back_led();
	} else {
		send_not_found();
	}
}

void change_front_led() {
	FRONT_LED_STATUS = !FRONT_LED_STATUS;
	Serial.print("front: ");
	Serial.println(printBool(FRONT_LED_STATUS));
	server.send(HTTP_OKAY, TYPE_TEXT, printBool(FRONT_LED_STATUS));
}

void change_back_led() {
	BACK_LED_STATUS = !BACK_LED_STATUS;
	Serial.print("back: ");
	Serial.println(printBool(BACK_LED_STATUS));
	server.send(HTTP_OKAY, TYPE_TEXT, printBool(BACK_LED_STATUS));
}

void arg_missing(String arg) {
	server.send(HTTP_NOT_FOUND, TYPE_TEXT, "arg '" + arg + "' is missing");
}

void send_not_found() {
	server.send(HTTP_NOT_FOUND, TYPE_TEXT, "not found");
}

String sendHTML() {
	return "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'><title>ESP32 T1</title><style>html{margin:0px auto; text-align:center;background-color:#000;color:#FFF}.button{min-width:40px; background-color:#000; border:#FF4500 solid 1px; border-radius:4px; padding:13px 30px; font-size:25px; cursor:pointer; transition:background-color 0.2s,color 0.2s;}.button:hover{background-color:#FF4500; color:#000;}.grid{display:grid; row-gap:40px;}.wide-grid{grid-template-columns:auto auto auto; max-width:450px;}.small-grid{grid-template-columns:auto auto; max-width:300px;}.centered{margin:auto; margin-top:20px;}p{margin:0px;}span,p{font-size:30px;}h2{margin-top:60px; margin-bottom:0px;}</style><script>function getRequest(to,target){let req=new XMLHttpRequest();req.open('GET',to,false);req.send(null);if(target!=null){document.getElementById(target).innerHTML=req.responseText;}}</script></head><body><h1>ESP32 T1 control</h1><h2>Lights</h2><div class='centered grid small-grid'><div><span class='button' onclick='getRequest(\"/lights?where=front\",\"lights-front\")'>front</span></div><span id='lights-front'>false</span><div><span class='button' onclick='getRequest(\"/lights?where=back\",\"lights-back\")'>back</span></div><span id='lights-back'>false</span></div><h2>Steering</h1><div><p id='steer'>90</p><div class='centered grid wide-grid'><div><span class='button' onclick='getRequest(\"/steering?angle=30\",\"steer\")'>left</span></div><div><span class='button' onclick='getRequest(\"/steering?angle=0\",\"steer\")'>straight</span></div><div><span class='button' onclick='getRequest(\"/steering?angle=-30\",\"steer\")'>right</span></div></div></div><h2>Motor</h2><div><p id='speed'>0</p><div class='centered grid wide-grid'><div><span class='button' onclick='getRequest(\"/motor?speed=255\",\"speed\")'>forward</span></div><div><span class='button' onclick='getRequest(\"/motor?speed=0\",\"speed\")'>stop</span></div><div><span class='button' onclick='getRequest(\"/motor?speed=-255\",\"speed\")'>backward</span></div></div></div></body></html>";
}