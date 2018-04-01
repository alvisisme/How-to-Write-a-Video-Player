all: tutorial01

tutorial01: 01/tutorial01.c
	mkdir -p 01/out
	cp 01/samplempg.mp4 01/out/samplempg.mp4
	gcc -o 01/out/tutorial01 01/tutorial01.c -lavutil -lavformat -lavcodec -lswscale -lm -lz
	
clean:
	rm -rf 01/out