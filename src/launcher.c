#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "raylib.h"

int main() {
	const Color colorBlack = (Color){52, 58, 64, 255};
	const Color colorLightGray = (Color){233, 236, 239, 255};
	const Color colorDarkGray = (Color){173, 181, 189, 255};
	const Color colorWhite = WHITE;

	SetTraceLogLevel(LOG_ERROR);

	int scale = 2;
	const int windowWidth = 400 * scale;
	const int windowHeight = 600 * scale;
	InitWindow(windowWidth, windowHeight, "Launcher");

	Font fontTtf = LoadFontEx("/usr/share/launcher/fonts/Rubik-Regular.ttf", 48 * scale, 0, 250);

	struct Entry {
		char *name;
		char **argv;
	};
	struct Entry entries[] = {
		{
			.name = "Brain",
			.argv = (char *[]){"xdg-open", "http://localhost:52001/", NULL}
		},
		{
			.name = "Edit Brain",
			.argv = (char *[]){"zed", "/home/edwin/Dropbox-Maestral/Brain", NULL}
		},
		{
			.name = "Obsidian",
			.argv = (char *[]){"xdg-open", "obsidian://open?vault=KnowledgeObsidian", NULL}
		},
		{
			.name = "Terminal",
			.argv = (char *[]){"x-terminal-emulator", NULL}
		}
	};

	int const entryHeight = 100;
	int currentEntry = 0;

	SetTargetFPS(60);
	while (!WindowShouldClose()) {
		Vector2 mousePosition = GetMousePosition();

		BeginDrawing();
		ClearBackground(RAYWHITE);

		if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_J)) {
			if (currentEntry < (sizeof(entries) / sizeof(entries[0])) - 1) {
				currentEntry += 1;
			}
		}
		if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_K)) {
			if (currentEntry > 0) {
				currentEntry -= 1;
			}
		}

		if (IsKeyPressed(KEY_ENTER)) {
			struct Entry entry = entries[currentEntry];
			printf("Launching %s\n", entry.name);
			printf("Running prog: %s\n", entry.argv[0]);
			if (execvp(entry.argv[0], entry.argv) == -1) {
				perror("Failed to execvp");
				exit(1);
			}
			exit(0);
		}

		for (int i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
			DrawRectangle(0, i * entryHeight, windowWidth, entryHeight, i % 2 == 0 ? colorLightGray : colorDarkGray);
			DrawTextEx(fontTtf, entries[i].name, (Vector2){ 25, (i * entryHeight + 10) - 5 }, (float)fontTtf.baseSize, 2, colorBlack);
			if (i == currentEntry) {
				DrawTriangle((Vector2){ 0, (i * entryHeight) }, (Vector2){ 0, ((i + 1) * entryHeight)}, (Vector2){ 15, (i * entryHeight) + (entryHeight / 2) }, colorBlack);
			}
		}

		EndDrawing();
	}

	CloseWindow();
}
