import serial
import time
import keyboard

# ==========================================
# 1. 설정 (환경에 맞게 변경하세요)
# ==========================================
PORT = 'COM5'       # 아두이노가 연결된 포트 (Mac은 /dev/tty.usb...)
BAUD_RATE = 115200  # 아두이노 코드와 동일하게 설정
FPS = 30            # 초당 전송 횟수
STEP = 3            # 한 번 루프 돌 때 변하는 각도 크기 (속도 조절)


# ==========================================
# 2. 아두이노 연결
# ==========================================
try:
    ser = serial.Serial(PORT, BAUD_RATE)
    print(f"아두이노 연결 성공: {PORT}")
    time.sleep(2)  # 아두이노 리셋 대기 (필수)
except Exception as e:
    print(f"연결 실패! 포트를 확인해주세요.\n에러: {e}")
    exit()

# 초기 각도 (8개 서보 모터 모두 90도로 시작)
current_angles = [90] * 8 
target_frame_time = 1.0 / FPS  # 1프레임당 걸려야 하는 시간 (약 0.033초)

# 제어할 모터 인덱스 그룹 정의
EVEN_IDX = [0, 2, 4, 6]  # 짝수 모터
ODD_IDX  = [1, 3, 5, 7]  # 홀수 모터

print("\n=== 제어 시작 ===")
print("W/up: 각도 증가 (Open/Up)")
print("S/dn: 각도 감소 (Close/Down)")
print("Q: 종료")

try:
    while True:
        loop_start_time = time.time()

        # ==========================================
        # 3. 키보드 입력 처리 (값 변경)
        # ==========================================
        # --- 짝수 모터 제어 (ws) ---
        if keyboard.is_pressed('w'):
            for i in EVEN_IDX:
                current_angles[i] = min(current_angles[i] + STEP, 180)
                
        elif keyboard.is_pressed('s'):
            for i in EVEN_IDX:
                current_angles[i] = max(current_angles[i] - STEP, 0)

        # --- 홀수 모터 제어 (updown) ---
        if keyboard.is_pressed('up'):
            for i in ODD_IDX:
                current_angles[i] = min(current_angles[i] + STEP, 180)
                
        elif keyboard.is_pressed('down'):
            for i in ODD_IDX:
                current_angles[i] = max(current_angles[i] - STEP, 0)
        
        
        if keyboard.is_pressed('q'):
            print("\n초기화 중...")
            current_angles = [90] * 8 
            
            # [중요] 루프를 깨기(break) 전에 데이터를 "직접" 보내줘야 합니다.
            packet = ' '.join(map(str, current_angles)) + '\n'
            ser.write(packet.encode())
            
            time.sleep(0.5) # 아두이노가 명령을 받을 시간을 조금 줍니다
            
            print("종료합니다.")
            break

        # ==========================================
        # 4. 데이터 전송 (무조건 30FPS로 전송)
        # ==========================================
        # 리스트를 문자열로 변환: [90, 90...] -> "90 90 ... 90\n"
        packet = ' '.join(map(str, current_angles)) + '\n'
        
        # 아두이노로 전송
        ser.write(packet.encode())

        # (선택사항) 현재 상태 출력 - 너무 빠르면 정신없으니 주석 처리 가능
        print(f"\r각도: {current_angles[0]}도, {current_angles[1]}도, (FPS: {FPS})", end="")

        # ==========================================
        # 5. FPS 유지 (남는 시간만큼 대기)
        # ==========================================
        process_time = time.time() - loop_start_time
        sleep_time = target_frame_time - process_time
        
        if sleep_time > 0:
            time.sleep(sleep_time)

except KeyboardInterrupt:
    print("\n강제 종료됨")
finally:
    ser.close()
    print("\n포트 닫힘")
