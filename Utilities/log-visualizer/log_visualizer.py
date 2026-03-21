#!/usr/bin/env python3
"""
AprilTag Frame Log Visualizer

Reads FrameLogger text files and field JSON to visualize:
- Top-down field view with robot position and AprilTag locations
- Per-camera views showing detected tag corners
- Frame-by-frame stepping or continuous playback

Usage:
    python log_visualizer.py <field.json> <framelog> [--poses <pose-debug-output>]

    The optional --poses file is the stdout capture from pose-debug, containing
    lines like:
        <seq>
        <q0> <qx> <qy> <qz> : <x> <y> <z>   (bEstimate output)
        <q0> <qx> <qy> <qz> : <x> <y> <z>   (bSolve output)
    or:
        <seq>
        Lost Tracking

Controls:
    Space       - Play/Pause
    Right Arrow - Next frame
    Left Arrow  - Previous frame
    +/-         - Speed up/slow down playback
    Q/Esc       - Quit
"""

import sys
import json
import math
import re
import argparse
import numpy as np
import cv2


# --- Data Structures ---

class TagDetection:
    def __init__(self, tag_id, corners):
        self.tag_id = tag_id
        self.corners = corners  # list of 4 (x, y) tuples (pixel coords)


class Frame:
    def __init__(self, seq, cam_id, detections):
        self.seq = seq
        self.cam_id = cam_id
        self.detections = detections  # list of TagDetection


class FrameSet:
    """All camera frames for a single sequence number."""
    def __init__(self, seq, frames):
        self.seq = seq
        self.frames = frames  # list of Frame, one per camera


class FieldTag:
    def __init__(self, tag_id, x, y, z, qw, qx, qy, qz):
        self.tag_id = tag_id
        self.x = x  # inches
        self.y = y
        self.z = z
        self.qw = qw
        self.qx = qx
        self.qy = qy
        self.qz = qz

    def yaw_rad(self):
        """Extract yaw angle from quaternion."""
        siny_cosp = 2.0 * (self.qw * self.qz + self.qx * self.qy)
        cosy_cosp = 1.0 - 2.0 * (self.qy * self.qy + self.qz * self.qz)
        return math.atan2(siny_cosp, cosy_cosp)


class RobotPose:
    """Robot pose from pose-debug output."""
    def __init__(self, seq, x, y, z, q0, qx, qy, qz):
        self.seq = seq
        self.x = x  # inches
        self.y = y
        self.z = z
        self.q0 = q0
        self.qx = qx
        self.qy = qy
        self.qz = qz

    def yaw_rad(self):
        siny_cosp = 2.0 * (self.q0 * self.qz + self.qx * self.qy)
        cosy_cosp = 1.0 - 2.0 * (self.qy * self.qy + self.qz * self.qz)
        return math.atan2(siny_cosp, cosy_cosp)


# --- File Parsing ---

def load_field(json_path):
    """Load field tag positions from WPILib-format JSON."""
    with open(json_path, 'r') as f:
        data = json.load(f)

    tags = {}
    for tag_data in data['tags']:
        tag_id = tag_data['ID']
        trans = tag_data['pose']['translation']
        quat = tag_data['pose']['rotation']['quaternion']
        # Convert meters to inches
        x = trans['x'] * 1000.0 / 25.4
        y = trans['y'] * 1000.0 / 25.4
        z = trans['z'] * 1000.0 / 25.4
        tags[tag_id] = FieldTag(
            tag_id, x, y, z,
            quat['W'], quat['X'], quat['Y'], quat['Z']
        )
    return tags


def load_framelog(log_path):
    """Parse a FrameLogger text file into a list of FrameSets."""
    frame_sets = []

    with open(log_path, 'r') as f:
        while True:
            line = f.readline()
            if not line:
                break
            line = line.strip()
            if not line or not line.startswith('S:'):
                continue

            seq = int(line.split(':')[1])

            line = f.readline().strip()
            if not line.startswith('Cams:'):
                continue
            n_cams = int(line.split(':')[1])

            frames = []
            for cam_id in range(n_cams):
                line = f.readline().strip()
                if not line.startswith('T:'):
                    break
                n_tags = int(line.split(':')[1])

                detections = []
                for _ in range(n_tags):
                    line = f.readline().strip()
                    parts = line.split(',')
                    tag_id = int(parts[0])
                    corners = []
                    for c in range(4):
                        cx = float(parts[1 + c * 2])
                        cy = float(parts[2 + c * 2])
                        corners.append((cx, cy))
                    detections.append(TagDetection(tag_id, corners))

                frames.append(Frame(seq, cam_id, detections))

            frame_sets.append(FrameSet(seq, frames))

    return frame_sets


def load_poses(pose_path):
    """
    Parse pose-debug stdout output.
    Format per frame:
        <seq>
        <q0> <qx> <qy> <qz> : <x> <y> <z>     (estimate)
        <q0> <qx> <qy> <qz> : <x> <y> <z>     (solve)
    or:
        <seq>
        (no output if bEstimate returns false)

    Returns dict: seq -> RobotPose (using bSolve result if available, else bEstimate)
    """
    poses = {}
    pose_re = re.compile(
        r'^\s*(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s*:\s*(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s*$'
    )

    with open(pose_path, 'r') as f:
        lines = f.readlines()

    i = 0
    while i < len(lines):
        line = lines[i].strip()
        i += 1

        # Try to read a sequence number (just a bare integer)
        try:
            seq = int(line)
        except ValueError:
            continue

        # Read up to 2 pose lines after the sequence number
        last_pose = None
        while i < len(lines):
            m = pose_re.match(lines[i])
            if m:
                q0, qx, qy, qz = float(m.group(1)), float(m.group(2)), float(m.group(3)), float(m.group(4))
                x, y, z = float(m.group(5)), float(m.group(6)), float(m.group(7))
                last_pose = RobotPose(seq, x, y, z, q0, qx, qy, qz)
                i += 1
            elif lines[i].strip() == 'Lost Tracking':
                i += 1
            else:
                break

        if last_pose:
            poses[seq] = last_pose

    return poses


# --- Drawing ---

CAM_COLORS = [
    (0, 255, 0),     # Green
    (255, 100, 0),   # Blue-ish
    (0, 100, 255),   # Orange-ish
    (255, 0, 255),   # Magenta
]

# FRC field dimensions in inches
FIELD_LENGTH_IN = 690.875
FIELD_WIDTH_IN = 317.0


class FieldView:
    def __init__(self, width=800, height=400):
        self.width = width
        self.height = height
        margin = 30
        fw = width - 2 * margin
        fh = height - 2 * margin
        scale_x = fw / FIELD_LENGTH_IN
        scale_y = fh / FIELD_WIDTH_IN
        self.scale = min(scale_x, scale_y)
        field_w = int(FIELD_LENGTH_IN * self.scale)
        field_h = int(FIELD_WIDTH_IN * self.scale)
        self.ox = margin + (fw - field_w) // 2
        self.oy = margin + (fh - field_h) // 2
        self.field_w = field_w
        self.field_h = field_h

    def field_to_px(self, fx, fy):
        return (int(self.ox + fx * self.scale),
                int(self.oy + (FIELD_WIDTH_IN - fy) * self.scale))

    def draw(self, field_tags, frame_set, poses, frame_sets, current_idx):
        img = np.zeros((self.height, self.width, 3), dtype=np.uint8)

        # Field border
        cv2.rectangle(img, (self.ox, self.oy),
                      (self.ox + self.field_w, self.oy + self.field_h), (60, 60, 60), 2)

        # Center line
        cx = self.field_to_px(FIELD_LENGTH_IN / 2, 0)[0]
        cv2.line(img, (cx, self.oy), (cx, self.oy + self.field_h), (40, 40, 40), 1)

        # Draw all field tags
        for tag_id, ft in field_tags.items():
            px, py = self.field_to_px(ft.x, ft.y)
            yaw = ft.yaw_rad()
            tag_len = max(int(3.25 * self.scale), 4)
            dx = int(tag_len * math.cos(yaw))
            dy = int(-tag_len * math.sin(yaw))
            cv2.line(img, (px - dy, py - dx), (px + dy, py + dx), (100, 100, 100), 2)
            cv2.arrowedLine(img, (px, py), (px + dx, py - dy), (80, 80, 80), 1, tipLength=0.4)
            cv2.putText(img, str(tag_id), (px + 5, py - 5),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.35, (120, 120, 120), 1)

        # Highlight tags seen in current frame
        if frame_set:
            seen_tags = set()
            for frame in frame_set.frames:
                for det in frame.detections:
                    seen_tags.add(det.tag_id)
            for tag_id in seen_tags:
                if tag_id in field_tags:
                    ft = field_tags[tag_id]
                    px, py = self.field_to_px(ft.x, ft.y)
                    cv2.circle(img, (px, py), 6, (0, 255, 255), 2)
                    cv2.putText(img, str(tag_id), (px + 8, py - 2),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 255), 1)

        # Robot trail (last 100 poses)
        trail_len = 100
        start = max(0, current_idx - trail_len)
        for i in range(start, current_idx + 1):
            seq = frame_sets[i].seq
            if seq not in poses:
                continue
            rp = poses[seq]
            px, py = self.field_to_px(rp.x, rp.y)
            alpha = (i - start) / max(trail_len, 1)
            color = (int(50 + 200 * alpha), int(50 + 200 * alpha), 0)
            cv2.circle(img, (px, py), 2, color, -1)

        # Current robot pose
        cur_pose = None
        if frame_set and frame_set.seq in poses:
            cur_pose = poses[frame_set.seq]
            px, py = self.field_to_px(cur_pose.x, cur_pose.y)
            cv2.circle(img, (px, py), 8, (0, 255, 0), 2)
            yaw = cur_pose.yaw_rad()
            arrow_len = 20
            ax = int(px + arrow_len * math.cos(yaw))
            ay = int(py - arrow_len * math.sin(yaw))
            cv2.arrowedLine(img, (px, py), (ax, ay), (0, 255, 0), 2, tipLength=0.3)

        # Info text
        if frame_set:
            total_tags = sum(len(f.detections) for f in frame_set.frames)
            cv2.putText(img, f"Seq: {frame_set.seq}  Tags: {total_tags}", (10, 20),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
            if cur_pose:
                cv2.putText(img,
                            f"X:{cur_pose.x:.1f}  Y:{cur_pose.y:.1f}  "
                            f"Yaw:{math.degrees(cur_pose.yaw_rad()):.1f}",
                            (10, self.height - 10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.45, (200, 200, 200), 1)
            elif total_tags > 0:
                cv2.putText(img, "No pose (bEstimate rejected)", (10, self.height - 10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 0, 200), 1)

        return img


def draw_camera_views(frame_set, n_cams, cam_width=320, cam_height=240):
    """Draw per-camera views showing detected tag corners."""
    cols = min(n_cams, 2)
    rows = (n_cams + cols - 1) // cols
    img = np.zeros((rows * cam_height, cols * cam_width, 3), dtype=np.uint8)

    for cam_id in range(n_cams):
        col = cam_id % cols
        row = cam_id // cols
        x_off = col * cam_width
        y_off = row * cam_height

        cv2.rectangle(img, (x_off + 1, y_off + 1),
                      (x_off + cam_width - 2, y_off + cam_height - 2),
                      (60, 60, 60), 1)

        color = CAM_COLORS[cam_id % len(CAM_COLORS)]
        cv2.putText(img, f"Cam {cam_id}", (x_off + 5, y_off + 18),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)

        frame = None
        if frame_set:
            for f in frame_set.frames:
                if f.cam_id == cam_id:
                    frame = f
                    break

        if frame is None or len(frame.detections) == 0:
            cv2.putText(img, "No tags", (x_off + cam_width // 2 - 30, y_off + cam_height // 2),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (80, 80, 80), 1)
            continue

        # Scale from assumed camera resolution to display
        src_w, src_h = 1280.0, 800.0
        sx = cam_width / src_w
        sy = cam_height / src_h

        tag_count = len(frame.detections)
        cv2.putText(img, f"{tag_count} tag{'s' if tag_count != 1 else ''}",
                    (x_off + cam_width - 80, y_off + 18),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, (180, 180, 180), 1)

        for det in frame.detections:
            pts = []
            for cx, cy in det.corners:
                px = int(x_off + cx * sx)
                py = int(y_off + cy * sy)
                pts.append((px, py))

            # Draw quadrilateral
            for i in range(4):
                cv2.line(img, pts[i], pts[(i + 1) % 4], color, 2)

            # Corner dots (first corner in red to show orientation)
            for i, pt in enumerate(pts):
                cv2.circle(img, pt, 3, (0, 0, 255) if i == 0 else (255, 255, 255), -1)

            # Tag ID at center
            ctr_x = sum(p[0] for p in pts) // 4
            ctr_y = sum(p[1] for p in pts) // 4
            cv2.putText(img, str(det.tag_id), (ctr_x - 5, ctr_y + 5),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)

            # Show pixel area
            area = cv2.contourArea(np.array(pts))
            cv2.putText(img, f"{int(area)}px", (ctr_x - 15, ctr_y + 20),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.3, (180, 180, 180), 1)

    return img


# --- Main ---

def main():
    parser = argparse.ArgumentParser(description='AprilTag Frame Log Visualizer')
    parser.add_argument('field_json', help='Path to field JSON file (WPILib format)')
    parser.add_argument('framelog', help='Path to FrameLogger output file')
    parser.add_argument('--poses', help='Path to pose-debug stdout output file (optional)')
    parser.add_argument('--cams', type=int, default=4, help='Number of cameras (default: 4)')
    args = parser.parse_args()

    print(f"Loading field from {args.field_json}...")
    field_tags = load_field(args.field_json)
    print(f"  Loaded {len(field_tags)} tags")

    print(f"Loading frame log from {args.framelog}...")
    frame_sets = load_framelog(args.framelog)
    print(f"  Loaded {len(frame_sets)} frame sets")

    if not frame_sets:
        print("No frames found in log file!")
        return

    # Load pose data
    poses = {}
    if args.poses:
        print(f"Loading poses from {args.poses}...")
        poses = load_poses(args.poses)
        print(f"  Loaded {len(poses)} pose estimates")
    else:
        print("  No --poses file provided. Robot position will not be shown.")
        print("  Generate with: ./pose-debug <config> <framelog> > poses.txt")

    n_cams = args.cams
    for fs in frame_sets:
        n_cams = max(n_cams, len(fs.frames))

    # Playback state
    current = 0
    playing = False
    delay_ms = 100

    field_view = FieldView()
    window_name = "AprilTag Log Visualizer"
    cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)

    print("\nControls:")
    print("  Space       - Play/Pause")
    print("  Right/Left  - Next/Previous frame")
    print("  +/-         - Speed up/slow down")
    print("  Q/Esc       - Quit")

    while True:
        fs = frame_sets[current]

        # Draw views
        field_img = field_view.draw(field_tags, fs, poses, frame_sets, current)
        cam_img = draw_camera_views(fs, n_cams)

        # Stack field on top of cameras, matching width
        field_w = field_img.shape[1]
        cam_h = int(cam_img.shape[0] * field_w / cam_img.shape[1])
        cam_resized = cv2.resize(cam_img, (field_w, cam_h))

        # Status bar
        bar = np.zeros((30, field_w, 3), dtype=np.uint8)
        status = 'PLAYING' if playing else 'PAUSED'
        cv2.putText(bar, f"Frame {current + 1}/{len(frame_sets)}  Seq {fs.seq}  "
                    f"{status}  Delay: {delay_ms}ms",
                    (10, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.45, (200, 200, 200), 1)

        combined = np.vstack([field_img, bar, cam_resized])
        cv2.imshow(window_name, combined)

        # Input handling
        wait = delay_ms if playing else 0
        key = cv2.waitKey(max(wait, 1)) & 0xFF

        if key == ord('q') or key == 27:
            break
        elif key == ord(' '):
            playing = not playing
        elif key == 83 or key == ord('d') or key == 3:  # Right arrow
            current = min(current + 1, len(frame_sets) - 1)
            playing = False
        elif key == 81 or key == ord('a') or key == 2:  # Left arrow
            current = max(current - 1, 0)
            playing = False
        elif key == ord('+') or key == ord('='):
            delay_ms = max(10, delay_ms - 20)
        elif key == ord('-'):
            delay_ms = min(2000, delay_ms + 20)
        elif playing and (key == 255 or key == 0xFF):
            # No key pressed during playback — advance
            current += 1
            if current >= len(frame_sets):
                current = len(frame_sets) - 1
                playing = False

    cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
