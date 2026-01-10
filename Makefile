CC = gcc
CFLAGS = -municode -Wall -Werror -Os -s
TARGET = ssh-shell.exe
INSTALL_DIR = /c/msys64
REG_FILE = ssh-shell.reg

all: $(TARGET)

$(TARGET): ssh-shell.c
	env MSYSTEM=UCRT64 bash -lc "$(CC) $< -o $@ $(CFLAGS)"

clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) $(REG_FILE) $(INSTALL_DIR)/
	(cd $(INSTALL_DIR) && reg import $(REG_FILE))

install-deps:
	pacman -S --needed --noconfirm base-devel mingw-w64-ucrt-x86_64-gcc

.PHONY: all clean install install-deps
