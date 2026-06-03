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

unsigned long timer = 0;

// === 노이즈 필터용 변수 ===
float filteredAngleY = 0.0;
// 노이즈 필터링 강도 (0.1 ~ 0.9 사이 값)
// 숫자가 작을수록 묵직하고 부드러워지지만 반응이 약간 느려집니다.
const float FILTER_ALPHA = 0.2; 

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

  // 3. 모터 초기화 
  for(int i = 0; i < NUM_SERVOS; i++) {
    setServoAngle(i, currentAngles[i]);
  }
  
  Serial.println("Arduino Ready (MPU6050 Filtered Mode).");
}

void loop() {
  mpu.update(); 

  // 50ms마다 센서 값을 확인
  if((millis() - timer) > 50) { 
    
    // 1. 센서에서 현재 Y축 값을 읽어옵니다.
    float rawAngleY = mpu.getAngleY();
    
    // 2. 로우패스 필터 적용 (새로운 값과 이전 필터링된 값을 섞어 노이즈 제거)
    filteredAngleY = (FILTER_ALPHA * rawAngleY) + ((1.0 - FILTER_ALPHA) * filteredAngleY);
    
    bool angleChanged = false; 

    // ==========================================
    // 필터링된 Y축 각도에 따른 모터 트리거 구동 로직
    // ==========================================
    if (filteredAngleY > 4.0) {
      // 목표: 1번 0도, 2번 180도
      if (currentAngles[0] != 1) { // <--- 논리 오류 수정: 목표 각도인 0도인지 확인
        currentAngles[0] = 1;
        currentAngles[1] = 179;
        angleChanged = true;
      }
    }
    else if (filteredAngleY < -4.0) {
      // 목표: 1번 180도, 2번 0도
      if (currentAngles[0] != 179) { // <--- 논리 오류 수정: 목표 각도인 180도인지 확인
        currentAngles[0] = 179;
        currentAngles[1] = 1;
        angleChanged = true;
      }
    } 

    // 각도 목표치에 변화가 생겼을 때만 모터 드라이버로 신호 전송
    if (angleChanged) {
      for(int i = 0; i < 2; i++) { // 1번(0), 2번(1) 모터만 제어
        setServoAngle(i, currentAngles[i]);
      }
    }
    
    // 시리얼 모니터로 노이즈가 제거된 상태 출력 
    //Serial.print("Raw_Y: "); Serial.print(rawAngleY);
    //Serial.print(" | Filtered_Y: "); Serial.print(filteredAngleY);
    //Serial.print(" | M1: "); Serial.print(currentAngles[0]);
    //Serial.print(" | M2: "); Serial.println(currentAngles[1]);
    
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
