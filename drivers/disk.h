#ifndef DISK_H
#define DISK_H

#include <stdint.h> 

#define ATA_PRIMARY_IO        0x1F0
#define ATA_SECONDARY_IO      0x170
#define ATA_PRIMARY_CTRL      0x3F6
#define ATA_DMA_PRIMARY_CMD   0x00
#define ATA_DMA_PRIMARY_STATUS 0x02
#define ATA_DMA_PRIMARY_PRD   0x04
#define ATA_COMMAND_READ_DMA  0xC8
#define ATA_COMMAND_WRITE_DMA 0xCA 

// Identify drive
#define ATA_COMMAND_IDENTIFY 0xEC
#define ATA_CAP_DMA        (1 << 8)  // Word 49, bit 8
#define ATA_CAP_LBA        (1 << 9)  // Word 49, bit 9
#define ATA_CAP_UDMA       (1 << 13) // Word 88, bit 13

typedef struct {
    uint32_t phys_addr;
    uint32_t byte_count;
    uint16_t end_of_table;
} __attribute__((packed)) PRDEntry;

void init_ata_dma();

void ata_dma_read(uint32_t lba, uint8_t drive, uint16_t num_sectors, void* buffer);

void ata_dma_write(uint32_t lba, uint8_t drive, uint16_t num_sectors, void* buffer);

int check_dma_transfer_complete();

#endif