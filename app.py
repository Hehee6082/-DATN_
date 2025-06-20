from flask import Flask, render_template, Response, request, jsonify
import socket
import cv2
import threading
import time
import numpy as np
from camera_tracking import generate_video

app = Flask(__name__)

# ========== Biến toàn cục ==========
fire_frame = None
fire_result = "Đang chờ dữ liệu..."
UDP_IP = "192.168.1.12"
UDP_PORT = 4210
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# ========== Gửi lệnh UDP ==========
def send_udp_command(cmd):
    sock.sendto(cmd.encode(), (UDP_IP, UDP_PORT))
    print(f"Gửi: {cmd}")

# ========== Giao diện chính ==========
@app.route('/')
def index():
    return render_template('index.html')

# ========== Nhận lệnh từ client ==========
@app.route('/send_command', methods=['POST'])
def handle_command():
    start_time = time.time()
    data = request.get_json()
    cmd = data['command']
    if cmd == 'a':
        from pathfinding_module import run_pathfinding_from_shared_frame
        threading.Thread(target=run_pathfinding_from_shared_frame).start()
    else:
        send_udp_command(cmd)
    end_time = time.time()
    print(f"[INFO] /send_command handled in {(end_time - start_time) * 1000:.2f} ms")
    return '', 204

# ========== Stream video điều hướng ==========
@app.route('/video_feed')
def video_feed():
    return Response(generate_video(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

# ========== Nhận ảnh & nhãn từ Raspberry Pi ==========
@app.route('/upload_fire_frame', methods=['POST'])
def upload_fire_frame():
    global fire_frame, fire_result
    fire_result = request.headers.get("X-Fire-Label", "Không rõ")
    file_bytes = request.data
    np_arr = np.frombuffer(file_bytes, np.uint8)
    fire_frame = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
    return '', 204

# ========== Stream ảnh từ Ras Pi ==========
@app.route('/fire_feed')
def fire_feed():
    def generate():
        global fire_frame
        while True:
            if fire_frame is not None:
                _, buffer = cv2.imencode('.jpg', fire_frame)
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
            else:
                time.sleep(0.1)
    return Response(generate(), mimetype='multipart/x-mixed-replace; boundary=frame')

# ========== API cung cấp nhãn phân loại ==========
@app.route('/fire_result')
def get_fire_result():
    return jsonify({"label": fire_result})

# ========== Khởi động server ==========
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
