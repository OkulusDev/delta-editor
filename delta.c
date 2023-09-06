/*
=====================================================================
 Creator      |  Okulus Dev (C) 2023
 Copyright    |  GNU GPL v3
 Name         |  Delta-Editor
 Language     |  C
 Description  |  Минималистичный консольный текстовый редактор на C
=====================================================================
*/
/** Includes/Инклуды библиотек **/
#include <termios.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

/*** Defines/Установки ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/** Данные **/
// Создаем структуру termios
struct termios orig_termios;

struct editorConfig {
	int screenrows;
	int screencols;
	struct termios orig_termios;
};
struct editorConfig E;

/** Терминал/консоль **/
void kill_process(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit(1);
}

void stopRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    kill_process("tcsetattr");
}

void startRawMode() {
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) kill_process("tcgetattr");
	
	atexit(stopRawMode);
	
	struct termios raw = E.orig_termios;
	
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
 	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) kill_process("tcsetattr");
}

char editorReadKey() {
	int nread;
	char key_pressed;
	
	while ((nread = read(STDIN_FILENO, &key_pressed, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) kill_process("read");
	}
	
	return key_pressed;
}

int getCursorPosition(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;
	
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
	
	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}
	
	buf[i] = '\0';
	
	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
	
	return 0;
}

int getWindowSize(int *rows, int *cols) {
	struct winsize ws;
	
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    	if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    	
    	return getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		
		return 0;
	}
}

/** Добавление в буфер **/

struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab) {
	free(ab->b);
}

/*** Вывод ***/
void editorDrawRows(struct abuf *ab) {
	int y;
	
	for (y = 0; y < E.screenrows; y++) {
		abAppend(ab, "~", 1);
		if (y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}

void editorRedrawScr() {
	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[2J", 4);
	abAppend(&ab, "\x1b[H", 3);
	editorDrawRows(&ab);
	abAppend(&ab, "\x1b[H", 3);
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

/** Ввод **/
void editorProcessKeypress() {
	char key_pressed = editorReadKey();
	
	switch (key_pressed) {
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
  	}
}

/** Инициализация **/
void initEditor() {
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) kill_process("getWindowSize");
}

int main() {
	startRawMode();
  	initEditor();
	while (1) {
		editorRedrawScr();
		editorProcessKeypress();
	}

	return 0;
}
