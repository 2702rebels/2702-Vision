#!/usr/bin/env python3
"""Headless test - verifies parsing and rendering without a display."""

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

from log_visualizer import load_field, load_framelog, load_poses, FieldView, draw_camera_views

def main():
    test_dir = os.path.join(os.path.dirname(__file__), 'test_data')
    field_json = os.path.join(os.path.dirname(__file__), '..', '..', '2025', '2025AprilTag', '2025-reefscape-welded.json')
    framelog = os.path.join(test_dir, 'sample_framelog.txt')
    posefile = os.path.join(test_dir, 'sample_poses.txt')

    print("Loading field...")
    field_tags = load_field(field_json)
    print(f"  {len(field_tags)} tags loaded")

    print("Loading frame log...")
    frame_sets = load_framelog(framelog)
    print(f"  {len(frame_sets)} frame sets loaded")

    print("Loading poses...")
    poses = load_poses(posefile)
    print(f"  {len(poses)} poses loaded")

    # Verify frame data
    for i, fs in enumerate(frame_sets):
        total_tags = sum(len(f.detections) for f in fs.frames)
        has_pose = fs.seq in poses
        print(f"  Frame {i}: seq={fs.seq}, cams={len(fs.frames)}, "
              f"tags={total_tags}, pose={'yes' if has_pose else 'NO'}")

    # Test rendering
    print("\nRendering frames...")
    fv = FieldView()
    n_cams = max(len(fs.frames) for fs in frame_sets)

    for i, fs in enumerate(frame_sets):
        field_img = fv.draw(field_tags, fs, poses, frame_sets, i)
        cam_img = draw_camera_views(fs, n_cams)
        print(f"  Frame {i}: field={field_img.shape}, cameras={cam_img.shape}")

    # Save last frame as a test image
    import cv2
    import numpy as np
    field_w = field_img.shape[1]
    cam_h = int(cam_img.shape[0] * field_w / cam_img.shape[1])
    cam_resized = cv2.resize(cam_img, (field_w, cam_h))
    combined = np.vstack([field_img, cam_resized])

    out_path = os.path.join(test_dir, 'test_output.png')
    cv2.imwrite(out_path, combined)
    print(f"\nSaved test image to {out_path}")
    print("ALL TESTS PASSED")

if __name__ == '__main__':
    main()
