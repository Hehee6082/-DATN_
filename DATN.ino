#include <WiFi.h>
#include <WiFiUdp.h>

// ====== Cấu hình Wi-Fi ======
const char* ssid = "HLHUST BRO_2.4G";
const char* password = "12356789";

WiFiUDP udp;
const int UDP_PORT = 4210;
char incomingPacket[255]; // buffer nhận dữ liệu

// ====== Cấu hình chân L298N ======
#define IN1 0
#define IN2 1
#define IN3 8
#define IN4 9
#define ENA 5
#define ENB 6

#define MOTOR_SPEED 150
#define TURN_SPEED 255

// ====== Cảm biến IR ======
#define IR_LEFT       3  // cảm biến trái
#define IR_RIGHT      4  // cảm biến phải
#define IR_LEFT_OUTER 2  // cảm biến trái ngoài cùng
#define IR_RIGHT_OUTER 7 // cảm biến phải ngoài cùng

bool autoMode = false;

// ====== SETUP ======
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

  // Cấu hình cảm biến IR
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_RIGHT, INPUT);
  pinMode(IR_LEFT_OUTER, INPUT);  // Cảm biến trái ngoài cùng
  pinMode(IR_RIGHT_OUTER, INPUT); // Cảm biến phải ngoài cùng

  stopMotors();
}

// ====== LOOP ======
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

// ====== XỬ LÝ LỆNH MANUAL ======
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

// ====== HÀM LINE FOLLOWER ======
void followLine() {
  bool left = digitalRead(IR_LEFT);        // Cảm biến trái
  bool right = digitalRead(IR_RIGHT);     // Cảm biến phải
  bool leftOuter = digitalRead(IR_LEFT_OUTER);   // Trái ngoài cùng
  bool rightOuter = digitalRead(IR_RIGHT_OUTER); // Phải ngoài cùng

  Serial.print("LEFT: ");
  Serial.print(left);
  Serial.print(" | RIGHT: ");
  Serial.print(right);
  Serial.print(" | LEFT_OUTER: ");
  Serial.print(leftOuter);
  Serial.print(" | RIGHT_OUTER: ");
  Serial.println(rightOuter);

  // Trường hợp tất cả cảm biến đều trên line đen
  if (left == LOW && right == LOW && leftOuter == LOW && rightOuter == LOW) {
    Serial.println("🛑 Tất cả cảm biến đều trên line - Dừng lại");
    stopMotors();
  } 
  // Trường hợp tất cả cảm biến đều trên nền trắng
  else if (left == HIGH && right == HIGH && leftOuter == HIGH && rightOuter == HIGH) {
    Serial.println("⬆️ Tất cả cảm biến đều trắng - Đi thẳng");
    moveForward(110);
  } 
  // Trường hợp cảm biến bên trái phát hiện line
  else if (left == LOW || leftOuter == LOW) {
    Serial.println("↩️ Một trong các cảm biến bên trái trên line - Rẽ trái");
    turnLeft(250);
    delay(30);
  } 
  // Trường hợp cảm biến bên phải phát hiện line
  else if (right == LOW || rightOuter == LOW) {
    Serial.println("↪️ Một trong các cảm biến bên phải trên line - Rẽ phải");
    turnRight(250);
    delay(30);
  } 
  // Nếu không rơi vào các trường hợp trên
  else {
    Serial.println("🛑 Mất line hoặc không xác định được hướng - Dừng lại");
    stopMotors();
  }

  delay(10); // Thời gian chờ để tránh nhiễu
}


// ====== HÀM ĐIỀU KHIỂN ĐỘNG CƠ ======
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
