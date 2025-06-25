# -*- coding: utf-8 -*-
import cv2
import requests
import time
import torch
import numpy as np
import math

from models.common import DetectMultiBackend
from utils.general import non_max_suppression, scale_boxes
from utils.augmentations import letterbox
from utils.torch_utils import select_device

SERVER_URL = 'http://172.20.10.3:5000/upload_fire_frame'
weights_path = 'best_5n.pt'
device = select_device('cpu')


model = DetectMultiBackend(weights_path, device=device)
model.eval()
names = model.names  # ['fire', 'smoke', 'outlet', 'cardboard']

cap = cv2.VideoCapture(0)

def get_center(box):
    x1, y1, x2, y2 = box
    return ((x1 + x2) / 2, (y1 + y2) / 2)

def distance(c1, c2):
    return math.hypot(c1[0] - c2[0], c1[1] - c2[1])

def check_proximity(src_boxes, tgt_boxes, threshold=80):
    for src in src_boxes:
        c_src = get_center(src)
        for tgt in tgt_boxes:
            c_tgt = get_center(tgt)
            if distance(c_src, c_tgt) <= threshold:
                return True
    return False

def detect_fire(frame):
    img0 = frame.copy()
    img = letterbox(img0, new_shape=416)[0]
    img = img.transpose((2, 0, 1))[::-1]  # BGR to RGB, HWC to CHW
    img = np.ascontiguousarray(img)

    img = torch.from_numpy(img).to(device).float() / 255.0
    if img.ndimension() == 3:
        img = img.unsqueeze(0)

    pred = model(img)
    pred = non_max_suppression(pred, 0.25, 0.45)

    classification_result = "NO FIRE DETECTED"

    for det in pred:
        if len(det):
            det[:, :4] = scale_boxes(img.shape[2:], det[:, :4], img0.shape).round()

            fire_boxes, smoke_boxes, outlet_boxes, cardboard_boxes = [], [], [], []

            for *xyxy, conf, cls in reversed(det):
                c = int(cls)
                box = list(map(int, xyxy))
                if c == 0: fire_boxes.append(box)
                elif c == 1: smoke_boxes.append(box)
                elif c == 2: outlet_boxes.append(box)
                elif c == 3: cardboard_boxes.append(box)

            danger_boxes = fire_boxes + smoke_boxes
            if danger_boxes:
                if check_proximity(danger_boxes, outlet_boxes):
                    classification_result = "POWER OUTLET FIRE!"
                elif check_proximity(danger_boxes, cardboard_boxes):
                    classification_result = "CARDBOARD BOX FIRE!"
                else:
                    classification_result = "FIRE!"

    return classification_result

while True:
    ret, frame = cap.read()
    if not ret:
        continue

    classification_result = detect_fire(frame)

    _, img_encoded = cv2.imencode('.jpg', frame)

    try:
        requests.post(SERVER_URL,
                      data=img_encoded.tobytes(),
                      headers={"X-Fire-Label": classification_result},
                      timeout=0.5)
        print("a:", classification_result)
    except Exception as e:
        print("a", e)

    time.sleep(1)
