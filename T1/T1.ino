#include <ESP32Servo.h>
#include <WebServer.h>
#include <WiFi.h>
// #include <TM1637Display.h>

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST     0

#define HTTP_OKAY 200
#define HTTP_BAD_REQUEST 400
#define HTTP_NOT_FOUND 404

#define TYPE_TEXT "text/plain"
#define TYPE_HTML "text/html"

#define RIGHT_TRIGGER 25
#define RIGHT_ECHO 34
#define LEFT_TRIGGER 26
#define LEFT_ECHO 35
bool autodrive_enabled = false;
bool autodrive_led_updated = false;
#define AUTODRIVE_DISTANCE_THRESHOLD 60
#define AUTODRIVE_DISTANCE_MINIMUM 10
#define AUTODRIVE_DIFFERENCE_THRESHHOLD 10

#define MIN_DELAY 20
#define MAX_DELAY 500

#define FRONT_LED 12
#define BACK_LED 15
bool front_led_status = LOW;
bool back_led_status = LOW;

#define DEFAULT_STEERING_ANGLE 90 // 0 - 180: 90 is straight
#define STEERING_DEFAULT_OFFSET -10
#define STEERING_OFFSET 90
#define MAX_STEERING_ANGLE 30
#define SERVO_PIN 14
Servo servo;
int angle = DEFAULT_STEERING_ANGLE;

#define MOTOR_FORWARD_PIN 16
#define MOTOR_BACKWARD_PIN 17
#define MOTOR_FORWARD_CHANNEL 14
#define MOTOR_BACKWARD_CHANNEL 15
#define MOTOR_FREQUENCY 50
#define MOTOR_RESOLUTION 8
#define MAX_MOTOR_PWM ((1 << MOTOR_RESOLUTION) - 1)
int speed = 0;

#define LIGHT_ARG "where"
#define SPEED_ARG "speed"
#define STEERING_ARG "angle"

#define PASSIVE_BUZZER_PIN 13

#define RGB_RED 21
#define RGB_GREEN 19
#define RGB_BLUE 18
#define RGB_RESOLUTION 8
#define RGB_FREQUENCY 100

#define RGB_RED_CHANNEL 11
#define RGB_BLUE_CHANNEL 12
#define RGB_GREEN_CHANNEL 13

enum RGB_STATE {
  INCREASE_RED,
  DECREASE_GREEN,
  INCREASE_BLUE,
  DECREASE_RED,
  INCREASE_GREEN,
  DECREASE_BLUE,
};

int rgb_state = INCREASE_RED;
int r = 0;
int g = 255;
int b = 0;

#define RGB_MIN 0
#define RGB_MAX 255
#define RGB_OFFSET 20

#define RGB_DELAY_MS 50
unsigned long last_update = 0;

const char *ssid = "ESP32-T1";
const char *password = "passwordd";

/* Put IP Address details */
IPAddress local_ip(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

int tempo = 250;

// notes of the moledy followed by the duration.
// a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
// !!negative numbers are used to represent dotted notes,
// so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
int melody[] = {
  
  // Dart Vader theme (Imperial March) - Star wars 
  // Score available at https://musescore.com/user/202909/scores/1141521
  // The tenor saxophone part was used
  
  NOTE_AS4,8, NOTE_AS4,8, NOTE_AS4,8,//1
  NOTE_F5,2, NOTE_C6,2,
  NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4,   
};

// sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
// there are two values per note (pitch and duration), so for each note there are four bytes
int notes = sizeof(melody) / sizeof(melody[0]) / 2;

// this calculates the duration of a whole note in ms
int wholenote = (60000 * 4) / tempo;

int divider = 0, noteDuration = 0;

void setup() {
	pinMode(FRONT_LED, OUTPUT);
	pinMode(BACK_LED, OUTPUT);

  pinMode(RIGHT_ECHO, INPUT);
  pinMode(RIGHT_TRIGGER, OUTPUT);
  pinMode(LEFT_ECHO, INPUT);
  pinMode(LEFT_TRIGGER, OUTPUT);
	
	servo.attach(SERVO_PIN);
	set_angle(0, false);
	
	ledcAttachPin(MOTOR_FORWARD_PIN, MOTOR_FORWARD_CHANNEL);
	ledcAttachPin(MOTOR_BACKWARD_PIN, MOTOR_BACKWARD_CHANNEL);
	ledcSetup(MOTOR_FORWARD_CHANNEL, MOTOR_FREQUENCY, MOTOR_RESOLUTION);
	ledcSetup(MOTOR_BACKWARD_CHANNEL, MOTOR_FREQUENCY, MOTOR_RESOLUTION);
	ledcWrite(MOTOR_FORWARD_CHANNEL, 0);
	ledcWrite(MOTOR_BACKWARD_CHANNEL, 0);

  ledcSetup(RGB_RED_CHANNEL, RGB_FREQUENCY, RGB_RESOLUTION);
  ledcSetup(RGB_GREEN_CHANNEL, RGB_FREQUENCY, RGB_RESOLUTION);
  ledcSetup(RGB_BLUE_CHANNEL, RGB_FREQUENCY, RGB_RESOLUTION);

  ledcAttachPin(RGB_RED, RGB_RED_CHANNEL);
  ledcAttachPin(RGB_GREEN, RGB_GREEN_CHANNEL);
  ledcAttachPin(RGB_BLUE, RGB_BLUE_CHANNEL);

  ledcWrite(RGB_RED_CHANNEL, r);
  ledcWrite(RGB_GREEN_CHANNEL, g);
  ledcWrite(RGB_BLUE_CHANNEL, b);

	WiFi.softAP(ssid, password);
	WiFi.softAPConfig(local_ip, gateway, subnet);
	server.on("/", handle_root);
	server.on("/lights", handle_lights);
	server.on("/steering", handle_steering);
	server.on("/motor", handle_speed);
  server.on("/toggle_autodrive", handle_autodrive);
	server.onNotFound(send_not_found);

  sw_sound();

	server.begin();
}


void loop() {
	server.handleClient();
	digitalWrite(FRONT_LED, front_led_status);
	digitalWrite(BACK_LED, back_led_status);

  update_rgb_led();

  if(autodrive_enabled){
    update_autodrive();
  }

}

void sw_sound() {
  // for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
  //   // calculates the duration of each note
  //   divider = melody[thisNote + 1];
  //   if (divider > 0) {
  //     // regular note, just proceed
  //     noteDuration = (wholenote) / divider;
  //   } else if (divider < 0) {
  //     // dotted notes are represented with negative durations!!
  //     noteDuration = (wholenote) / abs(divider);
  //     noteDuration *= 1.5; // increases the duration in half for dotted notes
  //   }

  //   // we only play the note for 90% of the duration, leaving 10% as a pause
  //   tone(PASSIVE_BUZZER_PIN, melody[thisNote], noteDuration*0.9);

  //   // Wait for the specief duration before playing the next note.
  //   delay(noteDuration);
    
  //   // stop the waveform generation before the next note.
  //   noTone(PASSIVE_BUZZER_PIN);
  // }

  tone(PASSIVE_BUZZER_PIN, NOTE_GS2);
  delay(200);
  noTone(PASSIVE_BUZZER_PIN);

}

double getDistance(int trigger, int echo) {
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  unsigned long distance = pulseIn(echo, HIGH);
  return distance * 0.017;
}

void update_autodrive() {
  double distanceLeft = getDistance(LEFT_TRIGGER, LEFT_ECHO);
  double distanceRight = getDistance(RIGHT_TRIGGER, RIGHT_ECHO);

  // quickly changing distances causes a lot of problems
  if(abs(distanceLeft - distanceRight) < AUTODRIVE_DIFFERENCE_THRESHHOLD) {
    if(distanceLeft < distanceRight) {
      distanceRight = AUTODRIVE_DISTANCE_THRESHOLD + 1;
    } else {
      distanceLeft = AUTODRIVE_DISTANCE_THRESHOLD + 1;
    }
  }

  if(distanceLeft > AUTODRIVE_DISTANCE_THRESHOLD && distanceRight > AUTODRIVE_DISTANCE_THRESHOLD){
    set_angle(0, false);
    return;
  }
  if(distanceLeft < AUTODRIVE_DISTANCE_MINIMUM && distanceRight < AUTODRIVE_DISTANCE_MINIMUM){
    set_angle(0, false);
    set_speed(0, false);
    return;
  }
  if(distanceLeft < AUTODRIVE_DISTANCE_THRESHOLD){
    set_angle(map(distanceLeft, AUTODRIVE_DISTANCE_THRESHOLD, AUTODRIVE_DISTANCE_MINIMUM, 0, -30), false);
    return;
  }
  if(distanceRight < AUTODRIVE_DISTANCE_THRESHOLD){
    set_angle(map(distanceRight, AUTODRIVE_DISTANCE_THRESHOLD, AUTODRIVE_DISTANCE_MINIMUM, 0, 30), false);
    return;
  }

}

void update_rgb_led() {
  if(autodrive_enabled) {
    if(!autodrive_led_updated) {
      ledcWrite(RGB_RED_CHANNEL, 128);
      ledcWrite(RGB_GREEN_CHANNEL, 128);
      ledcWrite(RGB_BLUE_CHANNEL, 128);

      autodrive_led_updated = true;
    }

    return;
  }

  autodrive_led_updated = false;

  unsigned long current_time = millis();
  if(last_update + RGB_DELAY_MS > current_time) {
    return;
  }

  last_update = current_time;

  RGB_STATE st = static_cast<RGB_STATE>(rgb_state);
  switch(st) {
    case INCREASE_RED:
      r += RGB_OFFSET;
      if(r >= RGB_MAX) {
        rgb_state += 1;
        r = RGB_MAX;
      }      
      break;
    case DECREASE_GREEN:
      g -= RGB_OFFSET;
      if(g <= RGB_MIN) {
        rgb_state += 1;
        g = RGB_MIN;
      }
      break;
    case INCREASE_BLUE:
      b += RGB_OFFSET;
      if(b >= RGB_MAX) {
        rgb_state += 1;
        b = RGB_MAX;
      }      
      break;
    case DECREASE_RED:
      r -= RGB_OFFSET;
      if(r <= RGB_MIN) {
        rgb_state += 1;
        r = RGB_MIN;
      }
      break;
    case INCREASE_GREEN:
      g += RGB_OFFSET;
      if(g >= RGB_MAX) {
        rgb_state += 1;
        g = RGB_MAX;
      }      
      break;
    case DECREASE_BLUE:
      b -= RGB_OFFSET;
      if(b <= RGB_MIN) {
        rgb_state = 0;
        b = RGB_MIN;
      }
      break;
  }

  ledcWrite(RGB_RED_CHANNEL, r);
  ledcWrite(RGB_GREEN_CHANNEL, g);
  ledcWrite(RGB_BLUE_CHANNEL, b);
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
	
	ledcWrite(MOTOR_FORWARD_CHANNEL, forward);
	ledcWrite(MOTOR_BACKWARD_CHANNEL, backward);
}

void handle_steering() {
  if(autodrive_enabled) {
    server.send(HTTP_BAD_REQUEST, TYPE_TEXT, String(angle));
    return;
  }

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
	servo.write(angle + STEERING_DEFAULT_OFFSET);
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
	front_led_status = !front_led_status;
	server.send(HTTP_OKAY, TYPE_TEXT, printBool(front_led_status));
}

void change_back_led() {
	back_led_status = !back_led_status;
	server.send(HTTP_OKAY, TYPE_TEXT, printBool(back_led_status));
}

void handle_autodrive() {
  autodrive_enabled = !autodrive_enabled;
  server.send(HTTP_OKAY, TYPE_TEXT, printBool(autodrive_enabled));
}

void arg_missing(String arg) {
	server.send(HTTP_NOT_FOUND, TYPE_TEXT, "arg '" + arg + "' is missing");
}

void send_not_found() {
	server.send(HTTP_NOT_FOUND, TYPE_TEXT, "not found");
}

String sendHTML() {
	return "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'><title>ESP32 T1</title><style>html{margin:0px auto; text-align:center;background-color:#000;color:#FFF}.button{min-width:40px; background-color:#000; border:#FF4500 solid 1px; border-radius:4px; padding:13px 30px; font-size:25px; cursor:pointer; transition:background-color 0.2s,color 0.2s;}.button:hover{background-color:#FF4500; color:#000;}.grid{display:grid; row-gap:40px;}.wide-grid{grid-template-columns:auto auto auto; max-width:450px;}.small-grid{grid-template-columns:auto auto; max-width:300px;}.centered{margin:auto; margin-top:20px;}p{margin:0px;}span,p{font-size:30px;}h2{margin-top:60px; margin-bottom:0px;}</style><script>function getRequest(to,target){let req=new XMLHttpRequest();req.open('GET',to,false);req.send(null);if(target!=null){document.getElementById(target).innerHTML=req.responseText;}}</script></head><body><h1>ESP32 T1 control</h1><h2>Lights</h2><div class='centered grid small-grid'><div><span class='button' onclick='getRequest(\"/lights?where=front\",\"lights-front\")'>front</span></div><span id='lights-front'>false</span><div><span class='button' onclick='getRequest(\"/lights?where=back\",\"lights-back\")'>back</span></div><span id='lights-back'>false</span></div><h2>Steering</h1><div><p id='steer'>90</p><div class='centered grid wide-grid'><div><span class='button' onclick='getRequest(\"/steering?angle=30\",\"steer\")'>left</span></div><div><span class='button' onclick='getRequest(\"/steering?angle=0\",\"steer\")'>straight</span></div><div><span class='button' onclick='getRequest(\"/steering?angle=-30\",\"steer\")'>right</span></div></div></div><h2>Motor</h2><div><p id='speed'>0</p><div class='centered grid wide-grid'><div><span class='button' onclick='getRequest(\"/motor?speed=100\",\"speed\")'>forward</span></div><div><span class='button' onclick='getRequest(\"/motor?speed=0\",\"speed\")'>stop</span></div><div><span class='button' onclick='getRequest(\"/motor?speed=-100\",\"speed\")'>backward</span></div></div></div><h2>Autodrive</h2><div><p id=\"autodrive\">false</p><div class='centered'><div><span class='button' onclick='getRequest(\"/toggle_autodrive\", \"autodrive\")'>toggle autodrive</span></div><div></div></body></html>";
}