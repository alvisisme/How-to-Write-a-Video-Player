all: out tutorial01 tutorial02

out:
	mkdir -p out

tutorial01: tutorial01.c
	cp samplempg.mp4 out/samplempg.mp4
	gcc -o out/tutorial01 tutorial01.c -lavutil -lavformat -lavcodec -lswscale -lm -lz

tutorial02: tutorial02.c
	cp samplempg.mp4 out/samplempg.mp4
	gcc -o out/tutorial02 tutorial02.c -lavutil -lavformat -lavcodec -lswscale -lm -lz -lSDL

clean:
	rm -rf out