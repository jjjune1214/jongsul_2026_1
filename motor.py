import serial
import time
import keyboard

# ==========================================
# 1. 설정 (환경에 맞게 변경하세요)
# ==========================================
PORT = 'COM3'       # 아두이노가 연결된 포트 (Mac은 /dev/tty.usb...)
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

# 초기 각도 (3개 서보 모터 모두 90도로 시작)
current_angles = [90] * 3 
target_frame_time = 1.0 / FPS  # 1프레임당 걸려야 하는 시간 (약 0.033초)

print("\n=== 제어 시작 ===")
print("1번 모터: Q(Up) / A(Down)")
print("2번 모터: W(Up) / S(Down)")
print("3번 모터: E(Up) / D(Down)")
print("ESC: 종료")

try:
    while True:
        loop_start_time = time.time()

        # ==========================================
        # 3. 키보드 입력 처리 (값 변경)
        # ==========================================
        
        # --- 1번 모터 제어 (Q / A) ---
        if keyboard.is_pressed('q'):
            current_angles[0] = min(current_angles[0] + STEP, 180)
        elif keyboard.is_pressed('a'):
            current_angles[0] = max(current_angles[0] - STEP, 0)

        # --- 2번 모터 제어 (s/ w) ---
        if keyboard.is_pressed('s'):
            current_angles[1] = min(current_angles[1] + STEP, 180)
        elif keyboard.is_pressed('w'):
            current_angles[1] = max(current_angles[1] - STEP, 0)

        # --- 3번 모터 제어 (E / D) ---
        if keyboard.is_pressed('e'):
            current_angles[2] = min(current_angles[2] + STEP, 180)
        elif keyboard.is_pressed('d'):
            current_angles[2] = max(current_angles[2] - STEP, 0)
        
        # --- 프로그램 종료 (ESC) ---
        if keyboard.is_pressed('esc'):
            print("\n초기화 중...")
            current_angles = [90] * 3 
            
            # [중요] 루프를 깨기(break) 전에 데이터를 "직접" 보내줘야 합니다.
            packet = ' '.join(map(str, current_angles)) + '\n'
            ser.write(packet.encode())
            
            time.sleep(0.5) # 아두이노가 명령을 받을 시간을 조금 줍니다
            
            print("종료합니다.")
            break

        # ==========================================
        # 4. 데이터 전송 (무조건 30FPS로 전송)
        # ==========================================
        # 리스트를 문자열로 변환: [90, 90, 90] -> "90 90 90\n"
        packet = ' '.join(map(str, current_angles)) + '\n'
        
        # 아두이노로 전송
        ser.write(packet.encode())

        # (선택사항) 현재 상태 출력
        print(f"\r각도: M1({current_angles[0]}도), M2({current_angles[1]}도), M3({current_angles[2]}도) (FPS: {FPS})", end="")

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
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("\n포트 닫힘")
