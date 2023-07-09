#!/bin/sh
clang `pkg-config --cflags raylib` -o pen src/pen.c src/main.c `pkg-config --libs raylib` -lm
clang -nostdlib --target=wasm32 -Wl,--no-entry -Wl,--export=penUpdate -Wl,--export=penRender -Wl,--allow-undefined -o web/pen.wasm src/pen.c
