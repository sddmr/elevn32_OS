#pragma once
#include <stdint.h>
#include "pci.h"

namespace virtio_gpu {

struct GpuDevice {
    pci::Device* pci_dev;
    uint32_t bar0;
    bool initialized;
};

void init();
bool is_available();
void flush(int x, int y, int w, int h);

}
