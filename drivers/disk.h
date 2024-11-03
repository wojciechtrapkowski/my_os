#ifndef DISK_H
#define DISK_H

#include <stdint.h> 

/** @details
 * We are using legacy IDE mode - default one provided by QEMU, 
 * because it is much easier to implement driver for it. 
 * 
 * IDE is part of the chipset which come with motherboard. It can be considered
 * as a device, that can be detected on PCI bus. This device manages IDE drives, which
 * can be Hard-Disk, Floooy-Disk drives, etc. It can allow up to 4 drives to be connected to it. 
 * 
 * We will use only one drive, so we will be using only primary channel.
 */

#define ATA_PRIMARY_IO        0x1F0
#define ATA_SECONDARY_IO      0x170
#define ATA_PRIMARY_CTRL      0x3F6
#define ATA_SECONDARY_CTRL    0x374
#define ATA_DMA_PRIMARY_CMD   0x00
#define ATA_DMA_PRIMARY_STATUS 0x02
#define ATA_DMA_PRIMARY_PRD   0x04
#define ATA_COMMAND_READ_DMA  0xC8
#define ATA_COMMAND_WRITE_DMA 0xCA 

#define BM_STATUS_DMA_ACTIVE    (1 << 0)  // Bit 0: DMA active
#define BM_STATUS_ERROR         (1 << 1)  // Bit 1: Error
#define BM_STATUS_IRQ          (1 << 2)   // Bit 2: Interrupt
#define BM_CMD_START_BIT       (1 << 0)   // Bit 0: Start/Stop
#define BM_CMD_READ_BIT        (1 << 3)   // Bit 3: Read/Write

// Identify drive
#define ATA_COMMAND_IDENTIFY 0xEC
#define ATA_CAP_DMA        (1 << 8)  // Word 49, bit 8
#define ATA_CAP_LBA        (1 << 9)  // Word 49, bit 9
#define ATA_CAP_UDMA       (1 << 13) // Word 88, bit 13

// PCI Related
#define PCI_COMMAND 0x04
#define PCI_BUS_MASTER 0x04
#define PCI_BAR4           0x20    // Bus Master Base Address Register
#define IDE_PCI_BUS        0x00
#define IDE_PCI_DEVICE     0x01    // Other possible values: 0x01, 0x1F, or 0x0C
#define IDE_PCI_FUNCTION   0x01    // Other possible values: 0x00 or 0x01


/** @brief PRD (Physical Region Descriptor) entry
 * Fields:
 * - phys_addr: Physical address of the data buffer
 * - byte_count: Number of bytes to transfer
 * - end_of_table: End of the PRD table
 */
typedef struct {
    uint32_t phys_addr;
    uint32_t byte_count;
    uint16_t end_of_table;
} __attribute__((packed)) PRDEntry;

void init_ata_dma();

void ata_dma_read(uint32_t lba, uint8_t drive, uint16_t num_sectors, void* buffer);

void ata_dma_write(uint32_t lba, uint8_t drive, uint16_t num_sectors, void* buffer);

#endif