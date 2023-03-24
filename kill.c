#include <wiringPi.h>
#include <ws2811/ws2811.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <softPwm.h>
#include <pca9685.h>
#include <unistd.h>

#define MOTOR_A_EN 17
#define MOTOR_A_PIN1 18
#define MOTOR_A_PIN2 27

#define PIN_BASE 300
#define FREQ 49

#define TARGET_FREQ WS2811_TARGET_FREQ
#define LED_PIN 12
#define DMA 5
#define STRIP_TYPE WS2811_STRIP_RGB
#define LED_COUNT 12
#define RED 0xFF0000

#define MIN_PULSE_WIDTH 450
#define MAX_PULSE_WIDTH 2100
#define FREQ_PWM 49

ws2811_t ledstring = {
	.freq = WS2811_TARGET_FREQ,
	.dmanum = 10,
	.channel = {
		[0] = {
			.gpionum = LED_PIN,
			.count = LED_COUNT,
			.invert = 0,
			.brightness = 255,
			.strip_type = WS2811_STRIP_GRB,
		}}};

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void forward(int speed)
{
	digitalWrite(MOTOR_A_PIN1, HIGH);
	digitalWrite(MOTOR_A_PIN2, LOW);
	softPwmWrite(MOTOR_A_EN, speed);
}

int angle(int degree)
{
	int pulse_wide, analog_value;
	pulse_wide = map(degree, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
	analog_value = (int)((float)pulse_wide / 1000000 * FREQ_PWM * 4096);

	return analog_value;
}

int main(int argc, char **argv)
{
	if (wiringPiSetupGpio() == -1)
	{
		printf("nique les gpio");
	}

	pinMode(MOTOR_A_EN, PWM_OUTPUT);
	pinMode(MOTOR_A_PIN1, OUTPUT);
	pinMode(MOTOR_A_PIN2, OUTPUT);

	if (softPwmCreate(MOTOR_A_EN, 0, 100) != 0)
	{
		printf("nique le moteur");
	}

	int servo = pca9685Setup(PIN_BASE, 0x40, FREQ);
	if (servo < 0)
	{
		printf("nique l'i2c");
	}

	pwmWrite(PIN_BASE, angle(100));
	delay(500);

	if (ws2811_init(&ledstring) < 0)
	{
		fprintf(stderr, "ws2811_init failed\n");
		return -1;
	}

	int i;
	for (i = 0; i < LED_COUNT; i++)
	{
		ledstring.channel[0].leds[i] = 0x000000;
	}
	ws2811_render(&ledstring);

	usleep(500000);

	forward(0);

	ws2811_fini(&ledstring);

	return 0;
}
