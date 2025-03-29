#!/bin/sh

gcc main.c -o opengl_test1.elf -Wall -lGL -lGLU -lglut -lGLEW -lglfw -lXxf86vm -lXrandr -lXi -ldl -lXinerama -lXcursor -lm
