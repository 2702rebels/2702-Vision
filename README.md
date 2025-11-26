2702-Vision
===============

Code for vision processing including game-piece detection, apriltags, and utilities.

### Project overview / structure
**lib**
Libraries used throughout, such as color conversions, RPICamera helpers.etc
**2025/Object**
The main binary, contains object(production binary), replay, logtostdvideo and more.
**Utilities**
Helper binaries that only use lib, such as rpisample which records a video and saves it in a .log file

### Installing dependencies
**Linux**
2. Run ```chmod +x installpackages.sh```
3. Run ```./installpackages.sh```


### Building the program
Currently only supported on linux
**Linux**
1. ```cd lib```
2. ```make```
3. ```cd ..```
4. ```cd 2025/Object```
5. ```make```

### Building the docs

1. Install doxygen from their website (Doxygen | Install)[https://www.doxygen.nl/download.html]
2. Run ```doxygen``` in the project root directory(the directory where this file is)
3. In file explorer, look for and open index.html in the /docs/ folder.

### C++ Compiler tips
## Syntax errors
1. Read the error closely and look for the file + line number
2. Check for missing semicolons on that line, and a few lines above / below it
3. Make sure the parameters and names of your function definitions in .cpp files match the definitions in .h files
4. Check variable names, variables must start with a letter.
5. Check you have specified a type for all function / variable definitions
6. Check the scope of your variables, anything that is in a deeper set of brackets cannot be used outside of those brackets.
7. Check for typos, eg swapping ( with { or something funny like that
8. Google, you usually aren't the first person and won't be the last
## Include / Link errors
1. Read the error to find out which package is having a problem.
3. Have you included the path of the header in the Makefile? Check for a .pc file for the package though first and prefer using that over manually including the directory
5. Are the names of your -l flags correct? -ltest123 will search for libtest123.a, but prefer using a .pc file from the library over manually including the .a files with -l flags, to find .a files for your library try ```find . -name "*yourlibname*.a"```
6. Is the order of your -l flags correct? Libraries should be linked from the top out, meaning the library which has the most dependencies should be linked first, so for example if b depends on a, b should be linked first.

### Standard logic debugging loop / troubleshooting tips
1. Get logs either from object using the isLogging flag in AdvantageScope or through rpi-sample
2. Compile replay program in 2025/Object folder and run it on the .log file
3. Examine the raw video window, if something is wrong here then the fixed exposure & gain values should be your first suspects.
4. Examine the mask video window and the raw video side-by-side, if white is showing up on objects other than the object you want to detect, try adjusting the hue, saturation and value min/maxes either in AdvantageScope or in the processorSettings.txt file
5. Examine the filtered output window, if there is no contour(green line) around your object, then likely something in fit_cylinder_contour is causing the detection to be thrown out by returning {{}, {}}
6. If the values are incorrect, double check the constants defined at the top of the object.h file to make sure that they still match the position and orientation of the camera on the robot.
7. If that still doesn't solve the issue, check the math and make sure a direction didn't get flipped or something.