all: out tutorial01 tutorial02 tutorial03 tutorial04 tutorial05 tutorial06 tutorial07

out:
	mkdir -p out
	cp sample.mpg out/sample.mpg

tutorial01: tutorial01.c
	gcc -o out/tutorial01 tutorial01.c -lavutil -lavformat -lavcodec -lswscale -lm -lz

tutorial02: tutorial02.c
	gcc -o out/tutorial02 tutorial02.c -lavutil -lavformat -lavcodec -lswscale -lm -lz -lSDL

tutorial03: tutorial03.c
	gcc -o out/tutorial03 tutorial03.c -lavutil -lavformat -lavcodec -lswscale -lm -lz -lSDL

tutorial04: tutorial04.c
	gcc -o out/tutorial04 tutorial04.c -lavutil -lavformat -lavcodec -lswscale -lm -lz -lSDL

tutorial05: tutorial05.c
	gcc -o out/tutorial05 tutorial05.c -lavutil -lavformat -lavcodec -lswscale -lm -lz -lSDL

tutorial06: tutorial06.c
	gcc -o out/tutorial06 tutorial06.c -lavutil -lavformat -lavcodec -lswscale -lm -lz -lSDL

tutorial07: tutorial07.c
	gcc -o out/tutorial07 tutorial07.c -lavutil -lavformat -lavcodec -lswscale -lswresample -lm -lz -lSDL

clean:
	rm -rf out