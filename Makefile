all: main.c
	# gcc main.c -o exe -lwiringPi -lwiringPiPca9685 -lws2811 -lm
	gcc test.c -o exe -lwiringPi -lwiringPiPca9685 -lws2811 -lm
	gcc kill.c -o off -lwiringPi -lwiringPiPca9685 -lws2811 -lm

clean:
	rm exe off
