#include <stdio.h>

#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <stdnoreturn.h>
#include <stdbool.h>
#include  <errno.h>

#include "raylib.h"
int main() {
	const int menuBarHeight = 60;
	const int windowBorder = 5;
	const int windowWidth = 800;
	const int windowHeight = 600;
	const Color colorBlack = (Color){ 52, 58, 64, 255 };
	const Color colorLightGray = (Color){ 233, 236, 239, 255 };
	const Color colorDarkGray = (Color){ 173, 181, 189, 255 };
	const Color colorWhite = WHITE;

	/* start */
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(windowWidth, windowHeight, "Pick Sticker");

	SetTargetFPS(60);
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(RAYWHITE);

		EndDrawing();
	}

	CloseWindow();

	return EXIT_SUCCESS;
}
