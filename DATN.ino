#include <WiFi.h>
#include <WiFiUdp.h>


const char* ssid = "HLHUST BRO_2.4G";
const char* password = "12356789";

WiFiUDP udp;
const int UDP_PORT = 4210;
char incomingPacket[255]; // buffer nhận dữ liệu


#define IN1 0
#define IN2 1
#define IN3 8
#define IN4 9
#define ENA 5
#define ENB 6

#define MOTOR_SPEED 150
#define TURN_SPEED 255


#define IR_LEFT       3  
#define IR_RIGHT      4  
#define IR_LEFT_OUTER 2  
#define IR_RIGHT_OUTER 7 

bool autoMode = false;


void setup() {
  Serial.begin(115200);

  // Khởi động WiFi
  WiFi.begin(ssid, password);
  Serial.print("Đang kết nối WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Đã kết nối WiFi!");
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());

  // Bắt đầu UDP
  udp.begin(UDP_PORT);
  Serial.printf("🟢 Đang chờ lệnh UDP tại cổng %d...\n", UDP_PORT);

  // Cấu hình các chân điều khiển
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

 
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_RIGHT, INPUT);
  pinMode(IR_LEFT_OUTER, INPUT);  
  pinMode(IR_RIGHT_OUTER, INPUT);

  stopMotors();
}


void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
      String cmd = String(incomingPacket);

      Serial.printf("📩 Nhận lệnh: %s\n", incomingPacket);

      if (cmd == "auto") {
        autoMode = true;
        Serial.println("🔁 Chế độ AUTO FOLLOW LINE kích hoạt");
      } else if (cmd == "stop_auto") {
        autoMode = false;
        stopMotors();
        Serial.println("🛑 Dừng chế độ AUTO FOLLOW LINE");
      } else {
        autoMode = false;
        handleCommand(cmd[0]);
      }
    }
  }

  if (autoMode) {
    followLine();
  }
}


void handleCommand(char cmd) {
  switch (cmd) {
    case 'f': moveForward(MOTOR_SPEED); break;
    case 'b': moveBackward(); break;
    case 'l': turnLeft(TURN_SPEED); break;
    case 'r': turnRight(TURN_SPEED); break;
    case 's': stopMotors(); break;
    default: stopMotors(); break;
  }
}


void followLine() {
  bool left = digitalRead(IR_LEFT);        
  bool right = digitalRead(IR_RIGHT);     
  bool leftOuter = digitalRead(IR_LEFT_OUTER);   
  bool rightOuter = digitalRead(IR_RIGHT_OUTER);

  Serial.print("LEFT: ");
  Serial.print(left);
  Serial.print(" | RIGHT: ");
  Serial.print(right);
  Serial.print(" | LEFT_OUTER: ");
  Serial.print(leftOuter);
  Serial.print(" | RIGHT_OUTER: ");
  Serial.println(rightOuter);


  if (left == LOW && right == LOW && leftOuter == LOW && rightOuter == LOW) {
    Serial.println("🛑 Tất cả cảm biến đều trên line - Dừng lại");
    stopMotors();
  } 

  else if (left == HIGH && right == HIGH && leftOuter == HIGH && rightOuter == HIGH) {
    Serial.println("⬆️ Tất cả cảm biến đều trắng - Đi thẳng");
    moveForward(110);
  } 
 
  else if (left == LOW || leftOuter == LOW) {
    Serial.println("↩️ Một trong các cảm biến bên trái trên line - Rẽ trái");
    turnLeft(250);
    delay(30);
  } 
 
  else if (right == LOW || rightOuter == LOW) {
    Serial.println("↪️ Một trong các cảm biến bên phải trên line - Rẽ phải");
    turnRight(250);
    delay(30);
  } 

  else {
    Serial.println("🛑 Mất line hoặc không xác định được hướng - Dừng lại");
    stopMotors();
  }

  delay(10); 
}



void moveForward(int speed) {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void moveBackward() {
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void turnRight(int r_speed) {
  analogWrite(ENA, r_speed);
  analogWrite(ENB, r_speed);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void turnLeft(int l_speed) {
  analogWrite(ENA, l_speed);
  analogWrite(ENB, l_speed);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}
