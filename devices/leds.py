#!/usr/bin/python3

import sys
import RPi.GPIO as GPIO
from time import sleep

PIN_SCL = 23
PIN_SDA = 25

IMAGE = [
    "        *       ",
    "       * *      ",
    "       * *      ",
    "       * *      ",
    "      ** **     ",
    "     *     *    ",
    "     *  *  *    ",
    "      ** **     ",
]

GPIO.setmode(GPIO.BCM)
GPIO.setup(PIN_SCL, GPIO.OUT)
GPIO.setup(PIN_SDA, GPIO.OUT)

def start():
    GPIO.output(PIN_SCL, 1)
    GPIO.output(PIN_SDA, 1)
    GPIO.output(PIN_SDA, 0)

def end():
    GPIO.output(PIN_SCL, 0)
    GPIO.output(PIN_SDA, 0)
    GPIO.output(PIN_SCL, 1)
    GPIO.output(PIN_SDA, 1)

def send_bit(value):
    GPIO.output(PIN_SCL, 0)
    if (value & 1) != 0:
        GPIO.output(PIN_SDA, 1)
    else:
        GPIO.output(PIN_SDA, 0)
    GPIO.output(PIN_SCL, 1)

def send_byte(value):
   for i in range(8):
       send_bit((value >> i) & 1)

start()
send_byte(0x40) #  set the address plus 1 automatically
end()

start()
send_byte(0xC0) # select address
for b in range(16):
    col = 0
    for bit in range(8):
       col = col >> 1
       if IMAGE[bit][b] != ' ': col |= 0x80
    send_byte(col)
end()

try:
    while True:
        for v in range(0x88, 0x8D, 1):
            start()
            send_byte(v)
            end()
            sleep(0.1)
        for v in range(0x8C, 0x87, -1):
            start()
            send_byte(v)
            end()
            sleep(0.1)
finally:
    GPIO.cleanup()
