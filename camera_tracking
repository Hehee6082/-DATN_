from flask import Flask, Response, render_template
import cv2
import numpy as np
import threading


app = Flask(__name__)


rows, cols = 20, 40
cell_size = 25


roi_selected = False
track_window = None
roi_hist = None


cap = cv2.VideoCapture(0)
white_map = None
latest_frame = None
frame_lock = threading.Lock()

def generate_video():
    global roi_selected, track_window, roi_hist, white_map
    while True:
        ret, frame = cap.read()
        if not ret:
            continue
        global latest_frame

        with frame_lock:
            latest_frame = frame.copy()
        # Cắt và resize khung hình
        height, width, _ = frame.shape
        frame = frame[int(height * 0.15):int(height * 0.95),
                      int(width * 0.05):int(width * 0.95)]
        frame = cv2.resize(frame, (cols * cell_size, rows * cell_size))

        
        local_white_map = np.ones((rows * cell_size, cols * cell_size, 3), dtype=np.uint8) * 255
        grid_map = np.zeros((rows, cols), dtype=np.uint8)

        
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        _, binary = cv2.threshold(gray, 130, 255, cv2.THRESH_BINARY_INV)

        
        kernel = np.ones((3, 3), np.uint8)
        morphed = cv2.morphologyEx(binary, cv2.MORPH_CLOSE, kernel)

        
        for y in range(rows):
            for x in range(cols):
                cell = morphed[y * cell_size:(y + 1) * cell_size,
                               x * cell_size:(x + 1) * cell_size]
                if np.mean(cell) > 10:
                    grid_map[y, x] = 1
                    cv2.rectangle(local_white_map, (x * cell_size, y * cell_size),
                                  ((x + 1) * cell_size, (y + 1) * cell_size),
                                  (0, 0, 255), -1)
                cv2.rectangle(local_white_map, (x * cell_size, y * cell_size),
                              ((x + 1) * cell_size, (y + 1) * cell_size),
                              (200, 200, 200), 1)

        
        if roi_selected:
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            back_proj = cv2.calcBackProject([hsv], [0], roi_hist, [0, 180], 1)
            ret, track_window = cv2.CamShift(back_proj, track_window,
                                             (cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 1))
            pts = cv2.boxPoints(ret)
            pts = np.intp(pts)
            cx = int(ret[0][0])
            cy = int(ret[0][1])
            gx = cx // cell_size
            gy = cy // cell_size
            cv2.circle(local_white_map, (gx * cell_size + cell_size // 2,
                                         gy * cell_size + cell_size // 2),
                       6, (0, 255, 0), -1)
            cv2.putText(local_white_map, "Robot", (gx * cell_size + 5, gy * cell_size),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 200, 0), 1)

        
        white_map = local_white_map.copy()

        



        cv2.imshow("Original", frame)

        key = cv2.waitKey(1) & 0xFF
        if key == ord('s'):  # Chọn ROI khi nhấn 'S'
            r = cv2.selectROI("Original", frame, False)
            track_window = (r[0], r[1], r[2], r[3])
            roi = frame[r[1]:r[1] + r[3], r[0]:r[0] + r[2]]
            hsv_roi = cv2.cvtColor(roi, cv2.COLOR_BGR2HSV)
            mask = cv2.inRange(hsv_roi, np.array((0., 60., 32.)), np.array((180., 255., 255.)))
            roi_hist = cv2.calcHist([hsv_roi], [0], mask, [180], [0, 180])
            cv2.normalize(roi_hist, roi_hist, 0, 255, cv2.NORM_MINMAX)
            roi_selected = True
        elif key == ord('q'):
            break
        _, buffer = cv2.imencode('.jpg', white_map)
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')


