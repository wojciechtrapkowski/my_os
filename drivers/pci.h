#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// PCI Configuration Space offsets
#define PCI_COMMAND        0x04    // Command Register
#define PCI_BUS_MASTER     0x04    // Bus Master bit in Command Register

void enable_bus_mastering(uint8_t bus, uint8_t slot, uint8_t func);
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);