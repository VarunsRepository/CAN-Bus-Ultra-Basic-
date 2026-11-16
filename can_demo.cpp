/*
 * can_demo.cpp
 *
 * Sends one frame, then prints everything that arrives.
 *
 *   ./can_demo              uses can0
 *   ./can_demo can1         uses can1
 *
 * Copyright (c) 2026 Varun
 * SPDX-License-Identifier: MIT
 */

#include <cstdio>
#include <string>

#include "canbus.h"

int main(int argc, char* argv[])
{
    const std::string interface = (argc > 1) ? argv[1] : "can0";

    CanBus bus(interface);
    if (!bus.open()) {
        return 1;
    }

    // Send one frame: ID 0x123, two bytes of payload.
    const std::uint8_t payload[2] = {0xDE, 0xAD};
    if (bus.send(0x123, payload, sizeof(payload))) {
        std::printf("sent 0x123 DE AD\n");
    }

    std::printf("listening on %s, Ctrl-C to stop\n", interface.c_str());

    CanBus::Frame frame;
    while (true) {
        if (!bus.receive(frame, 1000)) {
            continue;                      // nothing in the last second
        }

        std::printf("id 0x%03X  len %u  data", frame.id, frame.length);
        for (std::uint8_t i = 0; i < frame.length; ++i) {
            std::printf(" %02X", frame.data[i]);
        }
        std::printf("\n");
    }
}
