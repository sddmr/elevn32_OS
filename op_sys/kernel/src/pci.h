#pragma once
#include <stdint.h>

namespace pci {

struct Device {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint32_t bar[6];
};

void init();
Device* find_device(uint16_t vendor_id, uint16_t device_id);
uint32_t read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);

}
