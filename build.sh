#!/bin/sh
clang `pkg-config --cflags raylib` -o pen main.c `pkg-config --libs raylib` -lm
