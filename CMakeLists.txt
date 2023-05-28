CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(QRCSequence)

INCLUDE_DIRECTORIES(/usr/local/include/opencv4)

LINK_DIRECTORIES(/usr/local/lib)
LINK_LIBRARIES(opencv_core opencv_imgproc opencv_imgcodecs opencv_highgui opencv_videoio opencv_objdetect)

ADD_EXECUTABLE(${PROJECT_NAME} "main.cpp")
