#include <Wire.h>
#include <MPU6050_light.h>
#include <Adafruit_PWMServoDriver.h>

// 객체 생성
MPU6050 mpu(Wire);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// === MG996R 모터에 맞춘 안전 범위 ===
#define SERVOMIN 102  // 최소 펄스 길이 (0도 부근)
#define SERVOMAX 512  // 최대 펄스 길이 (180도 부근)

const int NUM_SERVOS = 3;
int currentAngles[NUM_SERVOS] = {90, 90, 50};

// F, G 입력 시 한 번에 이동할 각도 크기 (파이썬의 STEP3)
const int STEP3 = 10;
unsigned long timer = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // 1. 모터 드라이버 초기화
  pwm.begin();
  pwm.setPWMFreq(50); // MG996R 모터 50Hz
  
  delay(1000); // 보드 및 센서 안정화 대기
  
  // 2. MPU6050 초기화 및 영점 잡기
  mpu.begin();
  mpu.calcOffsets(); 

  // 3. 모터 초기화 (각각 90도로 세팅)
  for(int i = 0; i < NUM_SERVOS; i++) {
    setServoAngle(i, currentAngles[i]);
  }
  
  Serial.println("Arduino Ready (MPU6050 + PCA9685 Autonomous Mode).");
}

void loop() {
  mpu.update(); 

  // 50ms마다 센서 값을 확인
  if((millis() - timer) > 50) { 
    float angleX = mpu.getAngleX();
    bool angleChanged = false; 

    // ==========================================
    // X축 각도에 따른 모터 트리거 구동 로직
    // ==========================================
    if (angleX < -8.0) {
      // X값이 -8보다 작으면 1, 2번 모터를 즉시 180도로 설정
      if (currentAngles[0] != 180) { // 이미 180도면 중복 통신 방지
        currentAngles[0] = 180;
        currentAngles[1] = 180;
        angleChanged = true;
      }
    } 
    else if (angleX > 8.0) {
      // X값이 8보다 커지면 1, 2번 모터를 즉시 0도로 설정
      if (currentAngles[0] != 0) { // 이미 0도면 중복 통신 방지
        currentAngles[0] = 0;
        currentAngles[1] = 0;
        angleChanged = true;
      }
    }

    // 각도 목표치에 변화가 생겼을 때만 모터 드라이버로 신호 전송
    if (angleChanged) {
      for(int i = 0; i < 2; i++) { // 1번(0), 2번(1) 모터만 제어
        setServoAngle(i, currentAngles[i]);
      }
      
      // 시리얼 모니터로 현재 상태 출력
      Serial.print("X각도: "); Serial.print(angleX);
      Serial.print(" | 타겟 도착 -> M1: "); Serial.print(currentAngles[0]);
      Serial.print(" | M2: "); Serial.println(currentAngles[1]);
    }
    
    timer = millis();
  }
}

// === 서보 모터 각도 제어 함수 ===
void setServoAngle(int servoNum, int angle) {
  if (angle < 1) angle = 1;
  if (angle > 179) angle = 179;

  // 서보 각도(0~180)를 PCA9685 펄스 값(SERVOMIN~SERVOMAX)으로 매핑
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(servoNum, 0, pulse);
}
