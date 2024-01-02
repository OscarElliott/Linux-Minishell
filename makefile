default:
	gcc shell.c -Wall -Wconversion -pedantic -o Shell.exe

all:
	gcc shell.c -Wall -Wconversion -pedantic -o Shell.exe

run: all
	./Shell.exe

clean: