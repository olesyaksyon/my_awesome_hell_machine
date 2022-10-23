#!/usr/bin/python3
import os

try:
    from smbus2 import SMBus
except:
    from smbus import SMBus

from struct import pack, unpack
from collections import namedtuple
from time import sleep

BUS_ID = 1
DEVICE_ID = 0x6B

REG_FUNC_CFG_ACCESS = 0x01
REG_PIN_CTRL = 0x02
REG_FIFO_CTRL1 = 0x07
REG_FIFO_CTRL2 = 0x08
REG_FIFO_CTRL3 = 0x09
REG_FIFO_CTRL4 = 0x0A
REG_COUNTER_BDR_REG1 = 0x0B
REG_COUNTER_BDR_REG2 = 0x0C
REG_INT1_CTRL = 0x0D
REG_INT2_CTRL = 0x0E
REG_WHO_AM_I = 0x0F
REG_CTRL1_XL = 0x10
REG_CTRL2_G = 0x11
REG_CTRL3_C = 0x12
REG_CTRL4_C = 0x13
REG_CTRL5_C = 0x14
REG_CTRL6_C = 0x15
REG_CTRL7_G = 0x16
REG_CTRL8_XL = 0x17
REG_CTRL9_XL = 0x18
REG_CTRL10_C = 0x19
REG_ALL_INT_SRC = 0x1A
REG_WAKE_UP_SRC = 0x1B
REG_RESERVED = 0x1C
REG_D6D_SRC = 0x1D
REG_STATUS_REG = 0x1E
REG_RESERVED = 0x1F
REG_OUT_TEMP_L = 0x20
REG_OUT_TEMP_H = 0x21
REG_OUTX_L_G = 0x22
REG_OUTX_H_G = 0x23
REG_OUTY_L_G = 0x24
REG_OUTY_H_G = 0x25
REG_OUTZ_L_G = 0x26
REG_OUTZ_H_G = 0x27
REG_OUTX_L_A = 0x28
REG_OUTX_H_A = 0x29
REG_OUTY_L_A = 0x2A
REG_OUTY_H_A = 0x2B
REG_OUTZ_L_A = 0x2C
REG_OUTZ_H_A = 0x2D
REG_EMB_FUNC_STATUS_MAINPAGE = 0x35
REG_FSM_STATUS_A_MAINPAGE = 0x36
REG_FSM_STATUS_B_MAINPAGE = 0x37
REG_MLC_STATUS_MAINPAGE = 0x38
REG_STATUS_MASTER_MAINPAGE = 0x39
REG_FIFO_STATUS1 = 0x3A
REG_FIFO_STATUS2 = 0x3B
REG_TIMESTAMP0_REG = 0x40
REG_TIMESTAMP1_REG = 0x41
REG_TIMESTAMP2_REG = 0x42
REG_TIMESTAMP3_REG = 0x43
REG_RESERVED = 0x44-55
REG_INT_CFG0 = 0x56
REG_RESERVED = 0x57
REG_INT_CFG1 = 0x58
REG_THS_6D = 0x59
REG_RESERVED = 0x5A
REG_WAKE_UP_THS = 0x5B
REG_WAKE_UP_DUR = 0x5C
REG_FREE_FALL = 0x5D
REG_MD1_CFG = 0x5E
REG_MD2_CFG = 0x5F
REG_RESERVED = 0x60-61
REG_I3C_BUS_AVB = 0x62
REG_INTERNAL_FREQ_FINE = 0x63
REG_RESERVED = 0x64-72
REG_X_OFS_USR = 0x73
REG_Y_OFS_USR = 0x74
REG_Z_OFS_USR = 0x75
REG_RESERVED = 0x76-77
REG_FIFO_DATA_OUT_TAG = 0x78
REG_FIFO_DATA_OUT_X_L = 0x79
REG_FIFO_DATA_OUT_X_H = 0x7A
REG_FIFO_DATA_OUT_Y_L = 0x7B
REG_FIFO_DATA_OUT_Y_H = 0x7C
REG_FIFO_DATA_OUT_Z_L = 0x7D
REG_FIFO_DATA_OUT_Z_H = 0x7E

bus = SMBus(BUS_ID)

bus.write_byte_data(DEVICE_ID, REG_CTRL1_XL, 0x48) # acc 104Hz +-4G
bus.write_byte_data(DEVICE_ID, REG_CTRL2_G, 0x48)  # gyr 104Hz +-1000dps

ACC_RES = 4
GYR_RES = 1000

def print_percent(perc, l=30, char='*'):
   print('[', end='')
   v = int(l / 2 + l / 2 * perc / 100)
   if v > l: v = l
   if v < 0: v = 0
   print("".join(v * [char]), end='')
   print("".join((l - v) * [' ']), end='')
   print(']', end='')

while True:
    try:
       data = bytes(bus.read_i2c_block_data(DEVICE_ID, REG_OUTX_L_G, 12))
       temp = bytes(bus.read_i2c_block_data(DEVICE_ID, REG_OUT_TEMP_L, 2))
    except OSError:
       continue
    v = unpack("<hhhhhh", data)
    t = unpack("<h", temp)
    gx, gy, gz = v[0] * GYR_RES / 32767, v[1] * GYR_RES / 32767, v[2] * GYR_RES / 32767
    ax, ay, az = v[3] * ACC_RES / 32767, v[4] * ACC_RES / 32767, v[5] * ACC_RES / 32767
    t = t[0] / 256 + 25

    print(f"AX={ax:+05.2f} ", end='')
    print_percent(ax * 80)
    print(f"  AY={ay:+05.2f} ", end='')
    print_percent(ay * 80)
    print(f"  AZ={az:+05.2f} ", end='')
    print_percent(az * 80)

    print(f"  GX={gx:+07.2f} ", end='')
    print_percent(gx / 5)
    print(f"  GY={gy:+07.2f} ", end='')
    print_percent(gy / 5)
    print(f"  GZ={gz:+07.2f} ", end='')
    print_percent(gz / 5)

    print(f"  T={t:+07.3f} ", end='')

    print()

    sleep(0.01)