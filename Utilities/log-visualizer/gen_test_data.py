#!/usr/bin/env python3
"""
Generate realistic test data for the log visualizer.

Simulates a robot driving a path on the 2026 Rebuilt field with 4 cameras,
producing a framelog and matching pose file. Tags are projected onto each
camera using a simple pinhole model based on robot position and heading.
"""

import json
import math
import sys
import os

# --- Camera model ---

CAM_W = 1280
CAM_H = 800
HFOV_DEG = 70.0  # horizontal field of view
FOCAL_PX = (CAM_W / 2) / math.tan(math.radians(HFOV_DEG / 2))

TAG_SIZE_IN = 6.5  # AprilTag size in inches

# 4 cameras, each facing a different direction relative to robot heading
# (angle offset from robot heading, in degrees)
# Cam 0: front, Cam 1: right, Cam 2: back, Cam 3: left
CAM_ANGLES_DEG = [0, 90, 180, 270]


def load_field(json_path):
    with open(json_path, 'r') as f:
        data = json.load(f)
    tags = []
    for t in data['tags']:
        tid = t['ID']
        tx = t['pose']['translation']['x'] * 1000.0 / 25.4  # meters -> inches
        ty = t['pose']['translation']['y'] * 1000.0 / 25.4
        tz = t['pose']['translation']['z'] * 1000.0 / 25.4
        qw = t['pose']['rotation']['quaternion']['W']
        qz = t['pose']['rotation']['quaternion']['Z']
        yaw = math.atan2(2.0 * qw * qz, 1.0 - 2.0 * qz * qz)
        tags.append({'id': tid, 'x': tx, 'y': ty, 'z': tz, 'yaw': yaw})
    return tags


def project_tag(robot_x, robot_y, robot_yaw, cam_angle_deg, tag):
    """
    Project a tag onto a camera by projecting all 4 physical 3D corners
    through a pinhole camera model. Returns 4 corner pixel coords or None.
    """
    cam_yaw = robot_yaw + math.radians(cam_angle_deg)
    cos_c = math.cos(-cam_yaw)
    sin_c = math.sin(-cam_yaw)

    # Camera height in inches
    cam_height = 24.0

    # Tag center in field coords
    tag_x = tag['x']
    tag_y = tag['y']
    tag_z = tag['z']
    tag_yaw = tag['yaw']

    # Tag is a square in 3D, oriented by tag_yaw (rotation about Z axis).
    # The tag face normal points in the tag_yaw direction.
    # Tag corners in field coordinates (the tag is vertical, so corners
    # spread in the tag's local Y and Z axes):
    #   local Y = perpendicular to face normal (horizontal)
    #   local Z = up
    half = TAG_SIZE_IN / 2
    # Normal direction of tag face
    nx = math.cos(tag_yaw)
    ny = math.sin(tag_yaw)
    # Perpendicular (horizontal, along tag surface)
    px = -ny
    py = nx

    # 4 corners in field 3D coords: TL, TR, BR, BL
    # (as seen when looking at the tag face)
    corners_3d = [
        (tag_x + px * (-half), tag_y + py * (-half), tag_z + half),   # top-left
        (tag_x + px * ( half), tag_y + py * ( half), tag_z + half),   # top-right
        (tag_x + px * ( half), tag_y + py * ( half), tag_z - half),   # bottom-right
        (tag_x + px * (-half), tag_y + py * (-half), tag_z - half),   # bottom-left
    ]

    # Project each corner through camera
    projected = []
    for (wx, wy, wz) in corners_3d:
        # Relative to robot
        dx = wx - robot_x
        dy = wy - robot_y
        dz = wz - cam_height

        # Transform to camera frame
        # Camera looks along +X, +Y is left, +Z is up
        cam_fwd  =  dx * cos_c - dy * sin_c   # depth
        cam_left = -(dx * sin_c + dy * cos_c)  # horizontal
        cam_up   = dz                           # vertical

        # Must be in front of camera
        if cam_fwd < 6:
            return None

        # Project through pinhole
        u = CAM_W / 2 + FOCAL_PX * cam_left / cam_fwd
        v = CAM_H / 2 - FOCAL_PX * cam_up / cam_fwd
        projected.append((u, v))

    # Distance check
    dx = tag_x - robot_x
    dy = tag_y - robot_y
    dist = math.sqrt(dx*dx + dy*dy)
    if dist > 25 * 12:
        return None

    # Check if tag center is roughly in FOV
    center_u = sum(p[0] for p in projected) / 4
    center_v = sum(p[1] for p in projected) / 4
    if center_u < -100 or center_u > CAM_W + 100:
        return None
    if center_v < -100 or center_v > CAM_H + 100:
        return None

    # Skip tiny tags
    # Compute approximate pixel size from corner spread
    min_u = min(p[0] for p in projected)
    max_u = max(p[0] for p in projected)
    min_v = min(p[1] for p in projected)
    max_v = max(p[1] for p in projected)
    tag_size_px = max(max_u - min_u, max_v - min_v)
    if tag_size_px < 8:
        return None

    # Check at least partially in frame
    if max_u < 0 or min_u > CAM_W or max_v < 0 or min_v > CAM_H:
        return None

    return projected


def generate_path():
    """
    Generate a realistic robot path on the 2026 field.
    Returns list of (x, y, heading_rad, seq) tuples.
    Coordinates in inches.
    """
    path = []
    seq = 100

    # Segment helper
    def add_segment(x0, y0, x1, y1, heading, n_frames):
        nonlocal seq
        for i in range(n_frames):
            t = i / max(n_frames - 1, 1)
            x = x0 + (x1 - x0) * t
            y = y0 + (y1 - y0) * t
            path.append((x, y, heading, seq))
            seq += 1

    def add_arc(cx, cy, r, start_deg, end_deg, n_frames):
        nonlocal seq
        for i in range(n_frames):
            t = i / max(n_frames - 1, 1)
            angle = math.radians(start_deg + (end_deg - start_deg) * t)
            x = cx + r * math.cos(angle)
            y = cy + r * math.sin(angle)
            heading = angle + math.pi / 2  # tangent to arc
            if end_deg < start_deg:
                heading = angle - math.pi / 2
            path.append((x, y, heading, seq))
            seq += 1

    # 2026 Rebuilt field layout (from field manual Figure 3-36):
    #   Blue alliance wall at x=0 (left), Red alliance wall at x=651 (right)
    #   Scoring table at bottom (y=0 side)
    #
    #   Red Hub (tags 2-5 inner, 8-11 outer): ~x=469, y=159
    #   Blue Hub (tags 18-21 inner, 24-27 outer): ~x=182, y=159
    #   Red Tower/Depot tags: 6,7 at top (y=25); 1,12 at bottom (y=292)
    #   Blue Tower/Depot tags: 17,28 at top (y=25); 22,23 at bottom (y=292)
    #   Trench tags (blue wall): 29,30 at top (y=26-43); 31,32 mid (y=148-165)
    #   Outpost tags (red wall): 13,14 at bottom (y=275-292); 15,16 mid (y=153-170)
    #   Bump areas between the two hubs (yellow zones)

    # === Start at blue alliance wall, near trench ===
    add_segment(30, 35, 30, 35, 0, 3)

    # Drive away from blue wall past trench tags (29,30)
    add_segment(30, 35, 80, 50, 0, 10)

    # Head toward blue tower/depot area (tags 17,28 at top)
    add_segment(80, 50, 170, 30, math.radians(-5), 12)

    # Near blue tower (tags 17,28)
    add_segment(170, 30, 182, 40, math.radians(10), 8)

    # Drive toward blue hub from the top
    add_segment(182, 40, 182, 100, math.radians(90), 12)

    # Orbit blue hub (center ~182,159) at ~60 inch radius, facing inward
    # Hub is ~24in radius, robot stays well outside
    add_arc(182, 159, 60, -90, 270, 40)

    # Head toward blue tower at bottom (tags 22,23 at y=292)
    add_segment(182, 99, 182, 250, math.radians(90), 15)

    # Near blue tower bottom (tags 22,23)
    add_segment(182, 250, 182, 280, math.radians(90), 8)

    # Cross the bump toward center field
    add_segment(182, 280, 300, 250, math.radians(-20), 15)

    # Mid-field crossing over bump — sparse tags here, single-tag scenarios
    add_segment(300, 250, 325, 159, math.radians(-80), 18)

    # Continue crossing toward red side
    add_segment(325, 159, 400, 100, math.radians(-30), 15)

    # Approach red hub area
    add_segment(400, 100, 469, 99, math.radians(0), 12)

    # Orbit red hub (center ~480,159) at ~60 inch radius, facing inward
    add_arc(480, 159, 60, -90, 270, 40)

    # Head toward red tower at top (tags 6,7 at y=25)
    add_segment(480, 99, 469, 40, math.radians(-90), 12)

    # Near red tower top (tags 6,7)
    add_segment(469, 40, 469, 30, math.radians(-90), 6)

    # Drive toward outpost on red wall (tags 15,16 at y=153-170)
    add_segment(469, 30, 620, 100, math.radians(-20), 15)

    # Approach outpost tags (15,16)
    add_segment(620, 100, 638, 160, math.radians(70), 10)

    # Along red wall toward other outpost tags (13,14 at y=275-292)
    add_segment(638, 160, 638, 270, math.radians(90), 15)

    # Near outpost tags (13,14)
    add_segment(638, 270, 640, 283, math.radians(85), 6)

    # Head to red tower at bottom (tags 1,12 at y=292)
    add_segment(640, 283, 469, 285, math.radians(175), 18)

    # Near red tower bottom (tags 1,12)
    add_segment(469, 285, 469, 280, math.radians(-90), 6)

    # Drive back across field toward blue side
    add_segment(469, 280, 300, 200, math.radians(160), 20)

    # Through mid-field — sparse tags again
    add_segment(300, 200, 200, 159, math.radians(170), 15)

    # Back toward trench (tags 31,32 at y=148-165 on blue wall)
    add_segment(200, 159, 40, 155, math.radians(180), 18)

    # Close to trench tags (31,32)
    add_segment(40, 155, 15, 155, math.radians(180), 8)

    # Drive up to trench tags (29,30 at y=26-43)
    add_segment(15, 155, 15, 40, math.radians(-90), 12)

    # End near blue wall (29,30)
    add_segment(15, 40, 15, 30, math.radians(-90), 6)

    return path


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    field_json = os.path.join(script_dir, '..', '..', '2026', '2026AprilTag', '2026-rebuilt-welded.json')

    if not os.path.exists(field_json):
        print(f"Field JSON not found at {field_json}")
        sys.exit(1)

    tags = load_field(field_json)
    print(f"Loaded {len(tags)} tags from field")

    path = generate_path()
    print(f"Generated {len(path)} frames")

    out_dir = os.path.join(script_dir, 'test_data')
    os.makedirs(out_dir, exist_ok=True)

    framelog_path = os.path.join(out_dir, 'sample_framelog.txt')
    poses_path = os.path.join(out_dir, 'sample_poses.txt')

    n_cams = 4
    total_detections = 0

    with open(framelog_path, 'w') as flog, open(poses_path, 'w') as fpose:
        for robot_x, robot_y, robot_yaw, seq in path:
            flog.write(f"S:{seq}\n")
            flog.write(f"Cams:{n_cams}\n")

            frame_tags = 0
            for cam_idx in range(n_cams):
                cam_angle = CAM_ANGLES_DEG[cam_idx]
                detections = []

                for tag in tags:
                    corners = project_tag(robot_x, robot_y, robot_yaw, cam_angle, tag)
                    if corners:
                        detections.append((tag['id'], corners))

                flog.write(f"T:{len(detections)}\n")
                for tid, corners in detections:
                    flog.write(f"{tid},{corners[0][0]:8.3f},{corners[0][1]:8.3f},"
                               f"{corners[1][0]:8.3f},{corners[1][1]:8.3f},"
                               f"{corners[2][0]:8.3f},{corners[2][1]:8.3f},"
                               f"{corners[3][0]:8.3f},{corners[3][1]:8.3f}\n")
                    frame_tags += 1

                total_detections += len(detections)

            # Write pose (simulate bEstimate + bSolve output)
            # Add small noise to make bEstimate slightly different from bSolve
            q0 = math.cos(robot_yaw / 2)
            qz = math.sin(robot_yaw / 2)

            fpose.write(f"{seq}\n")
            if frame_tags >= 2:
                # bEstimate
                fpose.write(f"{q0:.4f} 0.0000 0.0000 {qz:.4f} : {robot_x + 0.5:.1f} {robot_y - 0.3:.1f} 0.0\n")
                # bSolve (more accurate)
                fpose.write(f"{q0:.4f} 0.0000 0.0000 {qz:.4f} : {robot_x:.1f} {robot_y:.1f} 0.0\n")
            elif frame_tags == 1:
                # Single tag — simulate the 10-inch jump bug
                # Occasionally flip to a bad pose
                if seq % 3 == 0:
                    bad_x = robot_x + 10.0  # 10-inch jump!
                    bad_y = robot_y - 8.0
                    fpose.write(f"{q0:.4f} 0.0000 0.0000 {qz:.4f} : {bad_x:.1f} {bad_y:.1f} 0.0\n")
                    fpose.write(f"{q0:.4f} 0.0000 0.0000 {qz:.4f} : {bad_x:.1f} {bad_y:.1f} 0.0\n")
                else:
                    fpose.write(f"{q0:.4f} 0.0000 0.0000 {qz:.4f} : {robot_x:.1f} {robot_y:.1f} 0.0\n")
                    fpose.write(f"{q0:.4f} 0.0000 0.0000 {qz:.4f} : {robot_x:.1f} {robot_y:.1f} 0.0\n")
            # else: no tags, no pose output

    print(f"Wrote {framelog_path}")
    print(f"Wrote {poses_path}")
    print(f"Total tag detections across all frames: {total_detections}")

    # Stats
    single_tag_frames = 0
    multi_tag_frames = 0
    no_tag_frames = 0
    for robot_x, robot_y, robot_yaw, seq in path:
        n = 0
        for cam_idx in range(n_cams):
            for tag in tags:
                corners = project_tag(robot_x, robot_y, robot_yaw, CAM_ANGLES_DEG[cam_idx], tag)
                if corners:
                    n += 1
        if n == 0:
            no_tag_frames += 1
        elif n == 1:
            single_tag_frames += 1
        else:
            multi_tag_frames += 1

    print(f"\nFrame breakdown:")
    print(f"  No tags visible:  {no_tag_frames}")
    print(f"  Single tag only:  {single_tag_frames}")
    print(f"  Multiple tags:    {multi_tag_frames}")


if __name__ == '__main__':
    main()
