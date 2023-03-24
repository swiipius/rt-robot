#include <wiringPi.h>
#include <ws2811/ws2811.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <softPwm.h>
#include <pca9685.h>
#include <unistd.h>
#include <time.h>

#define MOTOR_A_EN 17
#define MOTOR_A_PIN1 18
#define MOTOR_A_PIN2 27

#define LINE_LEFT 19
#define LINE_RIGHT 20
#define LINE_CENTER 16

#define PIN_BASE 300
#define FREQ 49
#define SPEED 40

#define TARGET_FREQ WS2811_TARGET_FREQ
#define LED_PIN 12
#define DMA 5
#define STRIP_TYPE WS2811_STRIP_RGB
#define LED_COUNT 12

#define MIN_PULSE_WIDTH 450
#define MAX_PULSE_WIDTH 2100
#define FREQ_PWM 50

#define TAU 0.00007

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

void greenLight()
{
    for (int i = 6; i < LED_COUNT; i++)
    {
        ledstring.channel[0].leds[i] = 0x00FF00;
    }
    ws2811_render(&ledstring);
}

void redLight()
{
    for (int i = 6; i < LED_COUNT; i++)
    {
        ledstring.channel[0].leds[i] = 0xFF0000;
    }
    ws2811_render(&ledstring);
}

int last_dir = 0; // 0 = center, 1 = left, 2 = right

float P_CONTROL(int r, int l, int c)
{
    if (r + l + c == 0)
    {
        return -1;
    }
    else if ((r + l == 0) && (c == 1))
    {
        last_dir = 0;
        return 100;
    }
    else if ((r + c == 2) && (l == 0))
    {
        last_dir = 0;
        return 90;
    }
    else if (r && (l + c == 0))
    {
        last_dir = 2;
        return 80;
    }
    else if ((l + c == 2) && (r == 0))
    {
        last_dir = 0;
        return 110;
    }
    else if (l && (r + c == 0))
    {
        last_dir = 1;
        return 120;
    }
    return 1;
}

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

void backward(int speed)
{
    digitalWrite(MOTOR_A_PIN1, LOW);
    digitalWrite(MOTOR_A_PIN2, HIGH);
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
        return -1;
    }

    pinMode(MOTOR_A_EN, PWM_OUTPUT);
    pinMode(MOTOR_A_PIN1, OUTPUT);
    pinMode(MOTOR_A_PIN2, OUTPUT);

    pinMode(LINE_LEFT, INPUT);
    pinMode(LINE_RIGHT, INPUT);
    pinMode(LINE_CENTER, INPUT);

    if (softPwmCreate(MOTOR_A_EN, 0, 100) != 0)
    {
        printf("nique le moteur");
        return -1;
    }

    int servo = pca9685Setup(PIN_BASE, 0x40, FREQ);
    if (servo < 0)
    {
        printf("nique l'i2c");
        return -1;
    }

    pwmWrite(PIN_BASE, angle(0));
    delay(500);
    pwmWrite(PIN_BASE, angle(90 + 10));
    delay(500);

    if (ws2811_init(&ledstring) < 0)
    {
        fprintf(stderr, "nique les leds");
        return -1;
    }

    for (int i = 0; i < LED_COUNT; i++)
    {
        ledstring.channel[0].leds[i] = 0x000000;
    }
    ws2811_render(&ledstring);

    for (int i = 0; i < 6; i++)
    {
        ledstring.channel[0].leds[i] = 0x000018;
    }
    ws2811_render(&ledstring);

    greenLight();
    ws2811_render(&ledstring);

    forward(30);

    clock_t start, end;
    double cpu_time_used;
    int r, l, c;
    int last_dir = 0;

    while (1)
    {
        start = clock();
        r = digitalRead(LINE_RIGHT);
        l = digitalRead(LINE_LEFT);
        c = digitalRead(LINE_CENTER);

        if (cpu_time_used > TAU)
        {
            redLight();
        }
        else
        {
            greenLight();
        }

        if ((r == 0) && (l == 0) && (c == 0) && (last_dir != 0))
        {
            backward(SPEED + 10);
            if (last_dir == 1)
            {
                pwmWrite(PIN_BASE, angle(85));
                delay(200);
            }
            else
            {
                pwmWrite(PIN_BASE, angle(115));
                delay(200);
            }
        }
        else
        {
            forward(SPEED);
            if ((r == 0) && (c == 1) && (l == 0))
            {
                last_dir = 0;
                pwmWrite(PIN_BASE, angle(100));
            }
            else if ((r == 1) && (c == 0) && (l == 0))
            {
                last_dir = 2;
                pwmWrite(PIN_BASE, angle(80));
            }
            else if ((r == 1) && (c == 1) && (l == 0))
            {
                last_dir = 0;
                pwmWrite(PIN_BASE, angle(90));
            }
            else if ((r == 0) && (c == 1) && (l == 1))
            {
                last_dir = 0;
                pwmWrite(PIN_BASE, angle(110));
            }
            else if ((r == 0) && (c == 0) && (l == 1))
            {
                last_dir = 1;
                pwmWrite(PIN_BASE, angle(120));
            }
        }

        end = clock();

        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
        //printf("%f\n", cpu_time_used);
    }

    return 0;
}
