/*
 * canbus.cpp
 *
 * Copyright (c) 2026 Varun
 * SPDX-License-Identifier: MIT
 */

#include "canbus.h"

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

CanBus::CanBus(const std::string& interface)
    : interfaceName(interface)
{
}

CanBus::~CanBus()
{
    close();
}

bool CanBus::open()
{
    if (fd >= 0) {
        return true;
    }

    // A CAN socket, raw, so we see whole frames.
    fd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        std::cerr << "CAN: socket failed: " << std::strerror(errno) << "\n";
        return false;
    }

    // Bind it to one interface, by name -> index.
    struct ifreq request {};
    std::strncpy(request.ifr_name, interfaceName.c_str(), IFNAMSIZ - 1);
    if (::ioctl(fd, SIOCGIFINDEX, &request) < 0) {
        std::cerr << "CAN: no interface called " << interfaceName << " ("
                  << std::strerror(errno) << ")\n"
                  << "     bring it up first: sudo ip link set can0 up "
                     "type can bitrate 500000\n";
        close();
        return false;
    }

    struct sockaddr_can address {};
    address.can_family = AF_CAN;
    address.can_ifindex = request.ifr_ifindex;

    if (::bind(fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "CAN: bind failed: " << std::strerror(errno) << "\n";
        close();
        return false;
    }

    return true;
}

void CanBus::close()
{
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

bool CanBus::send(std::uint32_t id, const std::uint8_t* data,
                  std::uint8_t length, bool extended)
{
    if (fd < 0 || data == nullptr || length > 8) {
        return false;
    }

    struct can_frame frame {};
    frame.can_id = id;
    if (extended) {
        frame.can_id |= CAN_EFF_FLAG;    // tells the kernel it is a 29-bit ID
    }
    frame.can_dlc = length;
    std::memcpy(frame.data, data, length);

    // One write = one frame. Short writes are not a thing here.
    if (::write(fd, &frame, sizeof(frame)) != static_cast<ssize_t>(sizeof(frame))) {
        std::cerr << "CAN: write failed: " << std::strerror(errno) << "\n";
        return false;
    }
    return true;
}

bool CanBus::receive(Frame& out, int timeoutMs)
{
    if (fd < 0) {
        return false;
    }

    if (timeoutMs >= 0) {
        struct pollfd waiting {};
        waiting.fd = fd;
        waiting.events = POLLIN;

        const int ready = ::poll(&waiting, 1, timeoutMs);
        if (ready <= 0) {
            return false;                // timed out, or interrupted
        }
    }

    struct can_frame frame {};
    if (::read(fd, &frame, sizeof(frame)) != static_cast<ssize_t>(sizeof(frame))) {
        return false;
    }

    out.extended = (frame.can_id & CAN_EFF_FLAG) != 0;
    out.id = frame.can_id & (out.extended ? CAN_EFF_MASK : CAN_SFF_MASK);
    out.length = frame.can_dlc > 8 ? 8 : frame.can_dlc;
    std::memcpy(out.data, frame.data, out.length);

    return true;
}
