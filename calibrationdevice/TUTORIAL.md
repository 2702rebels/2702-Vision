# Camera Calibration System Guide

This document explains how to set up the camera hardware, capture calibration images, and process those images to generate good calibration data. It also includes troubleshooting steps for common issues in that process.

---

## Overview

Camera calibration determines the intrinsic parameters and distortion characteristics of a camera so that images can be corrected and used for accurate measurement. This workflow involves three major stages:

- Physically preparing the camera and calibration target
- Capturing a diverse set of calibration images
- Running a post‑processing tool to compute calibration parameters

Everything you need for this setup should be in a box labelled Vision or Stargate

---

## Hardware Setup and Physical Preparation

The whole setup should go on a chair that has a flat seat or other movable platform.
Set up a tape measure at the center of the chessboard along the floor under the chair to measure the current distance from the board.

### Stargate setup
- Make sure that there are no loose connections of the jumper wires connecting the servos to the arduino
- Try to avoid turning the stargate while setting this up, as you can damage the motors
- Connect the arduino to the jetson or raspberry pi using a USB-A to microUSB cable

### Camera setup
- Find a 5/64 hex allen key and make sure it is actually a 5/64 key using a caliper
- Make sure the camera is labelled with a number or other identifier so you know which data goes with which camera
- Attach the camera to the stargate using the 2-56 1in bolts (tighten snugly, but not more than 1/2 turn after resistance starts)
- Do not tighten one bolt at a time, move them all forward in turn to avoid bending the board which can break the camera
- Note that 2 cameras must be connected to the jetson at all times or the video driver will not work properly

### Calibration target setup
- Get a chessboard on a target and try to make it as perfectly flat as possible
- Use a non-PWM (Pulse Width Modulating) light to light the area
    - A Google branded conference room light was used in 2026 at max brightness for this purpose
- Adjust the light to not be in the way of the view of the camera as much as possible
- Try to center the light on the checkerboard

---

## Running the Image Capture Process

### Starting the capture tool
- Run the `make clean` and `make -j$(nproc)` commands in this directory to build the cameracalib executable for the jetson
- For the raspberry pi, it will be built in the cmake build
- Run the `cameracalib` executable
- Try `/dev/video0` as the camera path, and then check `http://jetsonip:1181` to see the video feed after entering your integration value and gain (5000 integration 30 gain was used in 2026)
    - If it's not the right camera, then use `/dev/video1` and check again.
- The width of the checkerboard is measured as the number of squares wide including a terminating white square. To convert from the opencv checkerboard size measurement, add one to both the width and the height
- The name of the USB port for the stargate should be `/dev/ttyUSB0` assuming it is the only thing plugged into usb
- Choose a directory name to save images to. It should include the identifier of the camera (eg 1) so you can find it later

### Capturing calibration images
- Calibration images will be taken at 3 different distances (50in, 60in, 70in used in 2025, but can choose your own)
- Move the chair to an image, enter the current distance into the program
- It will begin to detect the FOV of the camera alongside the height and width of the chessboard
- It will then create a set of positions that it will take images at from that data. This is when you will see updates showing Step x out of x progress updates.
- Once it completes, it will prompt you for another distance. Move the chair to the next distance, then enter it and it will go again or enter 0 to indicate that there are no more distances left.

---

## Post‑Processing and Calibration

### Loading images into the calibration tool
- Use scp to copy the saved images to your desktop machine. This command will depend on where you saved the images. Eg `scp -r username@jetsonip:/home/username/2702-Vision/cameracalib/MyImagesFolder/* ./MyImagesFolder`
- If you have not already done so, extract the GML program from the `GML Camera Calibration 0.75.zip` file in this directory
- Load the GML program
- Create a new project with one template, click ok
- Enter the height of the checkerboard in the first box and the width in the second. This will be the width and height that you inputted into the cameracalib program. The width of the individual squares can be ignored.
- Click the green plus button, open the directory where you saved the images, go into a distance, Ctrl+A then click open. Repeat for all distances until all the images have been added to the list of images to use to calibrate.
- Click the multiple chessboard icon which will show a "Detect All" tooltip when hovered over and wait for it to go through the entire list of images
- Check the point density tab and make sure that there is a good spread of points throughout the entire image
- Click the camera icon to calculate the calibration parameters. This will take a while and while it is calculating them, GML will appear to hang but do not close it. You can use task manager to check whether it is still using cpu if you are unsure whether it has stopped.

### Interpreting calibration results
- The principal point should be about at the center of the lens in pixels
    - Eg for 1280x800, the principal point should be about 640, 400
- Check the rms error for the principal point, it should be <5 pixels
- If you do multiple trials with the same camera, the variance in the principal point between runs should be very small

### Exporting calibration outputs
- Use the Ctrl+S shortcut to save the calibration output into GML's XML format. You can also just copy the parameters that you care about into wherever you want to use them
- Again when saving the outputs make sure you label them with the camera they are intended for to prevent mismatching calibration parameters which will throw off the picture
---

## Troubleshooting

### Chessboard not found
- Improve lighting to make sure that the chessboard is visible from the web view at `http://jetsonip:1181`
- Ensure the entire target is visible in the frame.
- Verify that your width and height dimensions are correct

### Failure to capture images
- Verify your ribbon cable connections
- 2 cameras should be plugged into the jetson at all times
- Make sure no other programs are running that are using the camera

### High calibration error
- Look at the point density and make sure that it covers the entire image
- Check for defects in the stargate servos that would prevent it from moving correctly