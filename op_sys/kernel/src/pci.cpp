#include "pci.h"
#include "io.h"

namespace pci {

static Device devices[32];
static int device_count = 0;

uint32_t read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((uint32_t)bus << 16) | (uint32_t)((uint32_t)slot << 11) |
                       (uint32_t)((uint32_t)func << 8) | (uint32_t)(offset & 0xFC) | ((uint32_t)0x80000000);
    outl(0xCF8, address);
    return inl(0xCFC);
}

void write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((uint32_t)bus << 16) | (uint32_t)((uint32_t)slot << 11) |
                       (uint32_t)((uint32_t)func << 8) | (uint32_t)(offset & 0xFC) | ((uint32_t)0x80000000);
    outl(0xCF8, address);
    outl(0xCFC, value);
}

void init() {
    device_count = 0;
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor_device = read_config(bus, slot, func, 0x00);
                if ((vendor_device & 0xFFFF) == 0xFFFF) continue;
                
                if (device_count < 32) {
                    Device& d = devices[device_count++];
                    d.bus = bus; d.slot = slot; d.func = func;
                    d.vendor_id = vendor_device & 0xFFFF;
                    d.device_id = vendor_device >> 16;
                    
                    uint32_t class_rev = read_config(bus, slot, func, 0x08);
                    d.class_code = class_rev >> 24;
                    d.subclass = (class_rev >> 16) & 0xFF;
                    d.prog_if = (class_rev >> 8) & 0xFF;
                    
                    for (int i=0; i<6; i++) d.bar[i] = read_config(bus, slot, func, 0x10 + i*4);
                }
                
                // If not multi-function, skip other functions
                if (func == 0 && !(read_config(bus, slot, 0, 0x0C) & 0x800000)) break;
            }
        }
    }
}

Device* find_device(uint16_t vendor_id, uint16_t device_id) {
    for (int i=0; i<device_count; i++) {
        if (devices[i].vendor_id == vendor_id && devices[i].device_id == device_id) return &devices[i];
    }
    return nullptr;
}

}
