/*
 * canbus.h
 *
 * CAN on Linux via SocketCAN.
 *
 * There is no driver to write here. The kernel already has one, and it
 * presents the CAN controller as a network interface called can0 - so you
 * talk to the bus with ordinary Berkeley sockets, the same API as TCP.
 * A CAN frame goes in and out like a datagram.
 *
 * Copyright (c) 2026 Varun
 * SPDX-License-Identifier: MIT
 */

#ifndef CANBUS_H
#define CANBUS_H

#include <cstdint>
#include <string>

class CanBus {
public:
    struct Frame {
        std::uint32_t id = 0;        // 11-bit standard, or 29-bit extended
        std::uint8_t data[8] = {0};
        std::uint8_t length = 0;     // 0..8 bytes
        bool extended = false;       // true for a 29-bit ID
    };

    explicit CanBus(const std::string& interface = "can0");
    ~CanBus();

    CanBus(const CanBus&) = delete;
    CanBus& operator=(const CanBus&) = delete;

    bool open();
    void close();
    bool isOpen() const { return fd >= 0; }

    bool send(std::uint32_t id, const std::uint8_t* data, std::uint8_t length,
              bool extended = false);

    // timeoutMs: -1 blocks forever, 0 polls, >0 waits that long.
    // Returns true when a frame was read into out.
    bool receive(Frame& out, int timeoutMs = -1);

private:
    std::string interfaceName;
    int fd = -1;
};

#endif // CANBUS_H
