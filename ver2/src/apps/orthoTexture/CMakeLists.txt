# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

SET(APP_NAME orthoTexture)

PROJECT(${APP_NAME})
set(AppPath ${CMAKE_BINARY_DIR}/bin/Apps/${APP_NAME})

set(EXECUTABLE_OUTPUT_PATH ${AppPath})

ADD_EXECUTABLE(${APP_NAME} main.cpp)
TARGET_LINK_LIBRARIES(${APP_NAME} sgct)
