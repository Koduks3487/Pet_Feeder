#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>
#include <linux/poll.h>
#include <signal.h>
#include <wiringPi.h>
#include <softPwm.h>

#define SENSOR_FILE_NAME "/dev/sensor_driver"
#define ULTRASONIC_TRIG_PIN 0
#define ULTRASONIC_ECHO_PIN 1
#define LED_PIN1 2
#define LED_PIN2 3
#define SERVO1 4
#define BUTTON1 21
#define T_BUTTON1 22
#define START_BUTTON 23

int sensor_fd;

void ultrasonicTrigger()
{
    digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
}

int ultrasonicRead()
{
    int distance;
    unsigned long startTime = 0;
    unsigned long travelTime = 0;

    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    delayMicroseconds(2);

    ultrasonicTrigger();

    while (digitalRead(ULTRASONIC_ECHO_PIN) == LOW)
        startTime = micros();

    while (digitalRead(ULTRASONIC_ECHO_PIN) == HIGH)
        travelTime = micros() - startTime;

    distance = travelTime / 58;

    return distance;
}

void servo_control(int n)
{
    softPwmWrite(SERVO1, 24);
    delay(400 * 2);
    softPwmWrite(SERVO1, 5);
    delay(400);
}

void servo_timer(int n, int time)
{
    if (time == 0)
    {
        servo_control(n);
    }
    else if (time >= 1)
    {
        delay(1800 * time);
        servo_control(n);
    }
}

int main()
{
    wiringPiSetup();

    pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
    pinMode(ULTRASONIC_ECHO_PIN, INPUT);
    pinMode(LED_PIN1, OUTPUT);
    pinMode(LED_PIN2, OUTPUT);
    pinMode(SERVO1, OUTPUT);
    pinMode(BUTTON1, INPUT);
    pinMode(T_BUTTON1, INPUT);
    pinMode(START_BUTTON, INPUT);
    pullUpDnControl(BUTTON1, PUD_UP);
    pullUpDnControl(T_BUTTON1, PUD_UP);
    pullUpDnControl(START_BUTTON, PUD_UP);
    softPwmCreate(SERVO1, 0, 200);

    sensor_fd = open(SENSOR_FILE_NAME, O_RDWR | O_NONBLOCK);
    if (sensor_fd < 0)
    {
        fprintf(stderr, "Can't open %s\n", SENSOR_FILE_NAME);
        return -1;
    }

    int last_button_B = HIGH;
    int last_button_T = HIGH;
    int last_button_S = HIGH;
    int current_button_B;
    int current_button_T;
    int current_button_S;

    int time = 0, n = 0;

    while (1)
    {
        char sensor_value;
        if (read(sensor_fd, &sensor_value, 1) != 1)
        {
            fprintf(stderr, "Failed to read sensor value\n");
            return -1;
        }

        int distance = ultrasonicRead();

        current_button_B = digitalRead(BUTTON1);
        current_button_T = digitalRead(T_BUTTON1);
        current_button_S = digitalRead(START_BUTTON);

        if (last_button_T == HIGH && current_button_T == LOW)
        {
            if (time >= 4)
                time = 0;
            else
                time++;
        }

        if (last_button_B == HIGH && current_button_B == LOW)
        {
            if (n >= 3)
                n = 0;
            else
                n++;
        }

        if (last_button_S == HIGH && current_button_S == LOW)
        {
            servo_timer(n, time);
            time = 0;
            n = 0;
        }

        last_button_T = current_button_T;
        last_button_B = current_button_B;
        last_button_S = current_button_S;

        if (sensor_value == 1 && distance <= 20)
        {
            printf("수위 센서와 초음파 센서가 감지되었습니다!\n");
            digitalWrite(LED_PIN1, LOW);
            digitalWrite(LED_PIN2, LOW);
        }
        else if (sensor_value == 1)
        {
            printf("수위 센서가 감지되었습니다!\n");
            digitalWrite(LED_PIN1, LOW);
            digitalWrite(LED_PIN2, HIGH);
        }
        else if (distance <= 20)
        {
            printf("초음파 센서로 물체가 감지되었습니다!\n");
            digitalWrite(LED_PIN1, HIGH);
            digitalWrite(LED_PIN2, LOW);
        }
        else
        {
            digitalWrite(LED_PIN1, HIGH);
            digitalWrite(LED_PIN2, HIGH);
        }

        usleep(100000);
    }

    close(sensor_fd);

    return 0;
}
