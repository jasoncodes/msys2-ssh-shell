CC = gcc
CFLAGS = -municode -Wall -Werror -Os -s
TARGET = ssh-shell.exe
SERVER_TARGET = ssh-server.exe
SERVER_LDFLAGS = -mwindows
INSTALL_DIR = /c/msys64
REG_FILE = ssh-shell.reg

all: $(TARGET) $(SERVER_TARGET)

$(TARGET): ssh-shell.c
	BUILD_DIR="$$PWD" env MSYSTEM=UCRT64 bash -lc 'cd "$$BUILD_DIR" && $(CC) $< -o $@ $(CFLAGS)'

$(SERVER_TARGET): ssh-server.c
	BUILD_DIR="$$PWD" env MSYSTEM=UCRT64 bash -lc 'cd "$$BUILD_DIR" && $(CC) $< -o $@ $(CFLAGS) $(SERVER_LDFLAGS)'

clean:
	rm -f $(TARGET) $(SERVER_TARGET)

install: $(TARGET) $(SERVER_TARGET)
	cp $(TARGET) $(SERVER_TARGET) $(REG_FILE) $(INSTALL_DIR)/
	(cd $(INSTALL_DIR) && reg import $(REG_FILE))

install-deps:
	pacman -S --needed --noconfirm base-devel mingw-w64-ucrt-x86_64-gcc

.PHONY: all clean install install-deps
