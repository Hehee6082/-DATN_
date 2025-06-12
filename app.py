from flask import Flask, render_template, Response, request
import socket
import cv2
import threading
from camera_tracking import generate_video

app = Flask(__name__)


UDP_IP = "192.168.1.12"
UDP_PORT = 4210
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


def send_udp_command(cmd):
    sock.sendto(cmd.encode(), (UDP_IP, UDP_PORT))
    print(f"Gửi: {cmd}")

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/send_command', methods=['POST'])
def handle_command():
    data = request.get_json()
    cmd = data['command']
    if cmd == 'a':
        from pathfinding_module import run_pathfinding_from_shared_frame
        threading.Thread(target=run_pathfinding_from_shared_frame).start()
    else:
        send_udp_command(cmd)
    return '', 204

@app.route('/video_feed')
def video_feed():
    return Response(generate_video(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
