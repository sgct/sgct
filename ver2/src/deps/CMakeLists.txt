# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(SGCT_Deps)

ADD_SUBDIRECTORY(freetype)
ADD_SUBDIRECTORY(glew)
ADD_SUBDIRECTORY(glfw)
#ADD_SUBDIRECTORY(zlib)
ADD_SUBDIRECTORY(lpng)
ADD_SUBDIRECTORY(TinyThread++/source)
ADD_SUBDIRECTORY(tinyxml2)
ADD_SUBDIRECTORY(vrpn)
