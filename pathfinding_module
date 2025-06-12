import cv2
import numpy as np
import socket
import time
import cv2.aruco as aruco
from pathfinding.core.grid import Grid
from pathfinding.finder.a_star import AStarFinder
from camera_tracking import latest_frame, frame_lock

UDP_IP = "192.168.1.12"
UDP_PORT = 4210
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def send_command(cmd):
    sock.sendto(cmd.encode(), (UDP_IP, UDP_PORT))
    print(f"Sent: {cmd}")


cell_size = 30
aruco_dict = aruco.getPredefinedDictionary(aruco.DICT_5X5_250)
finder = AStarFinder()

def fix_pos(pos, grid):
    y, x = pos
    if grid[y][x] == 1:
        return pos
    for dy in [-1, 0, 1]:
        for dx in [-1, 0, 1]:
            ny, nx = y + dy, x + dx
            if 0 <= ny < len(grid) and 0 <= nx < len(grid[0]):
                if grid[ny][nx] == 1:
                    return (ny, nx)
    return pos

def get_direction(p1, p2):
    y1, x1 = p1
    y2, x2 = p2
    dy = y2 - y1
    dx = x2 - x1
    if dy == -1 and dx == 0:
        return 0
    elif dy == 0 and dx == 1:
        return 90
    elif dy == 1 and dx == 0:
        return 180
    elif dy == 0 and dx == -1:
        return 270
    return None

def get_turn(prev, cur, next_pos):
    dir1 = get_direction(prev, cur)
    dir2 = get_direction(cur, next_pos)
    if dir1 is None or dir2 is None:
        return "Invalid"
    diff = (dir2 - dir1) % 360
    if diff == 0:
        return "Go straight"
    elif diff == 90:
        return "Turn right"
    elif diff == 270:
        return "Turn left"
    elif diff == 180:
        return "Turn around"
    return "Invalid"

def action_to_cmd(action):
    return {
        "Go straight": 'f',
        "Turn left": 'l',
        "Turn right": 'r',
        "Turn around": 'b'
    }.get(action, 's')


def start_pathfinding(frame):

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)
    _, binary = cv2.threshold(blurred, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
    kernel = np.ones((3, 3), np.uint8)
    kernel_close = np.ones((5, 5), np.uint8)
    closed = cv2.morphologyEx(binary, cv2.MORPH_CLOSE, kernel_close, iterations=1)

    # Co ảnh để thu nhỏ line mảnh hơn
    kernel_erode = np.ones((3, 3), np.uint8)
    morphed = cv2.erode(closed, kernel_erode, iterations=1)

    rows = frame.shape[0] // cell_size
    cols = frame.shape[1] // cell_size
    grid_matrix = [[0 for _ in range(cols)] for _ in range(rows)]

    for i in range(rows):
        for j in range(cols):
            cell = morphed[i*cell_size:(i+1)*cell_size, j*cell_size:(j+1)*cell_size]
            if np.mean(cell) > 30:
                grid_matrix[i][j] = 1

    corners, ids, _ = aruco.detectMarkers(frame, aruco_dict)
    start_pos, end_pos = None, None
    robot_orientation = None

    if ids is not None:
        for i, marker_id in enumerate(ids.flatten()):
            cx, cy = np.mean(corners[i][0], axis=0)
            gy, gx = int(cy // cell_size), int(cx // cell_size)

            if marker_id == 3:
                start_pos = (gy, gx)
                rvec, tvec, _ = aruco.estimatePoseSingleMarkers(corners[i], 0.05, np.eye(3), np.zeros(5))
                rmat, _ = cv2.Rodrigues(rvec[0][0])
                direction = rmat @ np.array([0, 0, 1])
                angle_rad = np.arctan2(direction[0], direction[2])
                angle_deg = np.degrees(angle_rad) % 360

                if 315 <= angle_deg or angle_deg < 45:
                    robot_orientation = 0
                elif 45 <= angle_deg < 135:
                    robot_orientation = 90
                elif 135 <= angle_deg < 225:
                    robot_orientation = 180
                elif 225 <= angle_deg < 315:
                    robot_orientation = 270

            elif marker_id == 8:
                end_pos = (gy, gx)

    if start_pos and end_pos:
        start_fixed = fix_pos(start_pos, grid_matrix)
        end_fixed = fix_pos(end_pos, grid_matrix)
        grid = Grid(matrix=grid_matrix)
        start_node = grid.node(start_fixed[1], start_fixed[0])
        end_node = grid.node(end_fixed[1], end_fixed[0])
        grid.cleanup()
        path, _ = finder.find_path(start_node, end_node, grid)
        path = [(x, y) for x, y in path[:-1]]

        # VẼ GRID + PATH
        debug_frame = frame.copy()
        for i in range(rows):
            for j in range(cols):
                x1, y1 = j * cell_size, i * cell_size
                x2, y2 = (j + 1) * cell_size, (i + 1) * cell_size
                color = (50, 50, 50) if grid_matrix[i][j] == 0 else (200, 200, 200)
                cv2.rectangle(debug_frame, (x1, y1), (x2, y2), color, 1)

        for (x, y) in path:
            cv2.rectangle(debug_frame, (x*cell_size, y*cell_size), ((x+1)*cell_size, (y+1)*cell_size), (0, 255, 0), -1)

        cv2.imshow("Grid with Path", debug_frame)
        cv2.waitKey(2000)
        cv2.destroyAllWindows()

        
        if path and robot_orientation is not None and len(path) >= 2:
            command_list = [('f', 1)]
            path_start_index = 2

            actions = []
            for i in range(path_start_index, len(path)):
                p0 = (path[i - 2][1], path[i - 2][0]) if i >= 2 else (path[0][1], path[0][0])
                p1 = (path[i - 1][1], path[i - 1][0])
                p2 = (path[i][1], path[i][0])
                actions.append(get_turn(p0, p1, p2))

            count = 1
            for i in range(1, len(actions)):
                if actions[i] == "Go straight" and actions[i - 1] == "Go straight":
                    count += 1
                else:
                    if actions[i - 1] == "Go straight":
                        command_list.append(('f', count + 2))
                    else:
                        command_list.append((action_to_cmd(actions[i - 1]), 0))
                    count = 1
            if actions:
                if actions[-1] == "Go straight":
                    command_list.append(('f', count + 2))
                else:
                    command_list.append((action_to_cmd(actions[-1]), 0))

            print("\n===== LỆNH ĐÃ GỬI =====")
            for cmd, value in command_list:
                if cmd == 'f':
                    duration = value / 11.0
                    send_command('f')
                    time.sleep(duration)
                    send_command('s')
                    time.sleep(0.2)
                    print(f"{cmd} ({value} ô, {duration:.2f}s)")
                elif cmd in ['r', 'l', 'b']:
                    send_command(cmd)
                    time.sleep(0.6 if cmd == 'b' else 0.35)
                    send_command('s')
                    time.sleep(0.2)
                    print(f"{cmd} (rẽ)")
                else:
                    send_command('s')
                    time.sleep(0.2)
                    print("Stop")
        else:
            print("Không tìm được đường đi hợp lệ.")
    else:
        print("Không tìm thấy ArUco marker START hoặc END.")
def run_pathfinding_from_shared_frame():
    with frame_lock:
        frame = latest_frame.copy() if latest_frame is not None else None

    if frame is None:
        print("Không có frame sẵn để tìm đường.")
        return

    start_pathfinding(frame)
