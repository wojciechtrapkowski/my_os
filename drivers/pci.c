#include "pci.h"
#include "../cpu/ports.h"
#include <stdint.h>

/** @brief 
 * Enable Bus Mastering, which means it can directly access memory through DMA.
 * This is required for ATA DMA.
 * 
 * @param bus PCI bus number
 * @param slot PCI slot number
 * @param func PCI function number
 */
void enable_bus_mastering(uint8_t bus, uint8_t slot, uint8_t func) {
    // Read the current PCI Command register value
    uint16_t command = pci_config_read_word(bus, slot, func, PCI_COMMAND);
    
    // Set the Bus Master bit
    command |= PCI_BUS_MASTER;
    
    // Write back the updated command
    pci_config_write_word(bus, slot, func, PCI_COMMAND, command);
}

/**
 * @brief Read a 16-bit word from the PCI configuration space
 * 
 * @param bus PCI bus number
 * @param slot PCI slot number
 * @param func PCI function number
 * @param offset PCI configuration space offset
 * @return uint16_t The value read from the PCI configuration space
 */
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;

    // Create configuration address
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000) /* enable bit*/);

    // Write out the address
    port_dword_out(PCI_CONFIG_ADDRESS, address);

    // Read in the data
    // Shift the data to the right by the offset * 8 bits
    // So by 0 if offset & 2 is 0, or 8 if offset & 2 is 2
    // Then mask with 0xffff to get the lower 16 bits
    // Even though we read a dword, most PCI registers are 16 bits
    tmp = (uint16_t)((port_dword_in(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xffff);
    return tmp;
}

/**
 * @brief Write a 16-bit word to the PCI configuration space
 * 
 * @param bus PCI bus number
 * @param slot PCI slot number
 * @param func PCI function number
 * @param offset PCI configuration space offset
 * @param value The value to write to the PCI configuration space
 */
void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // Create configuration address
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000)) /* enable bit*/;

    // Write out the address
    port_dword_out(PCI_CONFIG_ADDRESS, address);

    // Write the data
    port_word_out(PCI_CONFIG_DATA, value);
}

