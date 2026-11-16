# CAN on a Raspberry Pi — setup

## The Pi has no CAN controller

Neither does your Nucleo F401RE. So before any code runs you need hardware:

| Option | Notes |
|---|---|
| **MCP2515 module** (+ TJA1050 or MCP2551 transceiver) | The cheap classic, ~£5, SPI. Check the crystal: 8 MHz or 16 MHz, it matters below |
| **MCP2518FD hat** | CAN-FD, newer, costs more |
| **USB-CAN adapter** | No GPIO wiring at all, shows up as `can0` the same way |

A transceiver is not optional. CAN is differential — the controller speaks logic levels, the transceiver turns them into CAN_H / CAN_L. A bare MCP2515 chip with no transceiver cannot drive a bus.

Also: **120 Ω terminating resistors at both ends of the bus**, nowhere else. Most modules have one on board with a jumper.

## Wiring an MCP2515 to a Pi 4

| Module | Pi 4 pin | BCM |
|---|---|---|
| VCC | 2 (5 V) | — |
| GND | 6 | — |
| CS | 24 | GPIO 8 (CE0) |
| SCK | 23 | GPIO 11 |
| SI | 19 | GPIO 10 |
| SO | 21 | GPIO 9 |
| INT | 22 | GPIO 25 |

## Enable the kernel driver

Add to `/boot/firmware/config.txt`:

```
dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25
```

Change `oscillator` to match the crystal printed on your module. **An 8 MHz value on a 16 MHz board gives you exactly half the bitrate you asked for**, which looks like a perfectly working interface that no other node understands. It is the single most common CAN-on-Pi mistake.

Reboot, then:

```bash
sudo ip link set can0 up type can bitrate 500000
ip -details link show can0
```

Every node on the bus must agree on the bitrate. 500 kbit/s is the usual default.

## Prove it before writing code

`can-utils` does for CAN what `gpiomon` does for GPIO:

```bash
sudo apt install can-utils

candump can0                   # watch every frame on the bus
cansend can0 123#DEAD          # send ID 0x123 with bytes DE AD
cangen can0 -g 100             # generate random traffic every 100 ms
```

If `candump` sees traffic and your program doesn't, it's your code. If it sees nothing, it's bitrate, wiring or termination.

## Test with no hardware at all

Linux has a virtual CAN interface, which is how you develop on a laptop:

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

./can_demo vcan0               # in one terminal
cansend vcan0 456#0102         # in another
```

## Build

```bash
g++ -std=c++17 -O2 -Wall -Wextra can_demo.cpp canbus.cpp -o can_demo
```

No library to link. SocketCAN is part of the kernel and its headers are in libc.
