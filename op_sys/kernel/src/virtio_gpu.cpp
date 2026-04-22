#include "virtio_gpu.h"
#include "serial.h"
#include "io.h"
#include "framebuffer.h"

namespace virtio_gpu {

static GpuDevice gpu;

// VirtIO GPU Commands
#define VIRTIO_GPU_CMD_RESOURCE_FLUSH 0x0102

void init() {
    gpu.initialized = false;
    pci::init();
    gpu.pci_dev = pci::find_device(0x1AF4, 0x1050);
    if (gpu.pci_dev) {
        serial::print("VirtIO-GPU Hardware Found\n");
        // Simple initialization: enable bus mastering
        uint32_t cmd = pci::read_config(gpu.pci_dev->bus, gpu.pci_dev->slot, gpu.pci_dev->func, 0x04);
        pci::write_config(gpu.pci_dev->bus, gpu.pci_dev->slot, gpu.pci_dev->func, 0x04, cmd | 0x07);
        gpu.initialized = true;
    } else {
        serial::print("VirtIO-GPU Hardware NOT found.\n");
    }
}

bool is_available() { return gpu.initialized; }

void flush(int x, int y, int w, int h) {
    if (!gpu.initialized) return;
    // Real VirtIO-GPU flush requires virtqueues and command buffers.
    // For now, we perform a targeted swap that the emulator can optimize.
    fb::swap_buffers_rect(x, y, w, h);
}

}
