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
		char *run;
		// char *exec;
		// char **args;
	};
	struct Entry *entries[4];
	entries[0] = malloc(sizeof(struct Entry));
	entries[0]->name = "Obsidian";
	entries[0]->run = "xdg-open obsidian://open?vault=Knowledge &";
	// entries[0]->exec = "xdg-open";
	// entries[0]->args = malloc(2 * sizeof(char *));
	// char *str1 = "obsidian://open?vault=Knowledge";
	// entries[0]->args[0] = str1;
	// entries[0]->args[1] = NULL;

	entries[1] = malloc(sizeof(struct Entry));
	entries[1]->name = "My Knowledge";
	entries[1]->run = "xdg-open http://localhost:52001/ &";

	entries[2] = malloc(sizeof(struct Entry));
	entries[2]->name = "Hub";
	entries[2]->run = "xdg-open http://localhost:49501/ &";
	// entries[1]->exec = "xdg-open";
	// entries[1]->args = malloc(2 * sizeof(char *));
	// char *str2 = "http://localhost:49501/";
	// entries[1]->args[0] = str2;
	// entries[1]->args[1] = NULL;

	entries[3] = malloc(sizeof(struct Entry));
	entries[3]->name = "Terminal";
	entries[3]->run = "x-terminal-emulator &";

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
			struct Entry *entry = entries[currentEntry];
			printf("Launching %s\n", entry->name);
			printf("Running: %s\n", entry->run);
			int pid = fork();
			if (pid < 0) {
				exit(EXIT_FAILURE);
			} else if (pid > 0) {
				printf("Launching child process: %d\n", pid);
				int status;
				waitpid(pid, &status, 0);
				exit(status);
			} else {

				// execvp(entry->exec, entry->args);
				// perror("execvp");
				// exit(EXIT_FAILURE);
				system(entry->run);
				EndDrawing();
				CloseWindow();
				printf("Exiting launcher system\n");
				exit(EXIT_SUCCESS);
			}
		}

		for (int i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
			DrawRectangle(0, i * entryHeight, windowWidth, entryHeight, i % 2 == 0 ? colorLightGray : colorDarkGray);
			DrawTextEx(fontTtf, entries[i]->name, (Vector2){ 25, (i * entryHeight + 10) - 5 }, (float)fontTtf.baseSize, 2, colorBlack);
			if (i == currentEntry) {
				DrawTriangle((Vector2){ 0, (i * entryHeight) }, (Vector2){ 0, ((i + 1) * entryHeight)}, (Vector2){ 15, (i * entryHeight) + (entryHeight / 2) }, colorBlack);
			}
		}

		EndDrawing();
	}

	CloseWindow();

	return EXIT_SUCCESS;
}
