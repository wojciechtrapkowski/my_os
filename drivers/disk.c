#include "disk.h"
#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "screen.h"
#include <stdint.h>
#include "pci.h"

static volatile int dma_transfer_complete = 0;


/** @brief Get the Bus Master IDE base address. We need it to access the DMA control registers.
 * BMIDE Registers (relative to base):
 * [Base + 0]   Command Register    (Start/Stop DMA)
 * [Base + 2]   Status Register     (DMA Status)
 * [Base + 4]   PRD Table Address   (DMA Buffer List)
 * 
 * @return uint32_t The base address of the Bus Master IDE
 */
uint32_t get_bmide_base() {
    uint32_t bar4 = pci_config_read_word(IDE_PCI_BUS, IDE_PCI_DEVICE, IDE_PCI_FUNCTION, PCI_BAR4);
    bar4 &= 0xFFF0;  // Mask off the lower 4 bits to get the base address
    return bar4;
}


/** @brief Interrupt handler for ATA DMA
 * 1. Stop the DMA transfer
 * 2. Check drive status
 * 3. Clear the interrupt bit in DMA status
 * 4. Signal completion
 */
static void ata_dma_interrupt_handler() {
    asm("cli");
    uint32_t bmide_base = get_bmide_base();

    // Read command register to check if it was read or write
    uint8_t dma_cmd = port_byte_in(bmide_base + ATA_DMA_PRIMARY_CMD);

    // Stop the DMA transfer
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0);
    
    if (dma_cmd & 0x08) {
        kprint("DMA Read completed\n");
    } else {
        kprint("DMA Write completed\n");
    }

    // Read drive status and DMA status
    uint8_t drive_status = port_byte_in(ATA_PRIMARY_IO + 7);
    uint8_t dma_status = port_byte_in(ATA_PRIMARY_CTRL + ATA_DMA_PRIMARY_STATUS);

    // Check drive status first
    if (drive_status & 0x01) {  // Error bit
        kprint("Drive error occurred!\n");
        uint8_t error = port_byte_in(ATA_PRIMARY_IO + 1);
        kprint("Error register: ");
        kprint_hex(error);
        kprint("\n");
        dma_transfer_complete = 0;
        return;
    }

    // Wait for Busy to clear and Ready to set
    int timeout = 1000;
    while (timeout > 0) {
        drive_status = port_byte_in(ATA_PRIMARY_IO + 7);
        if (!(drive_status & 0x80) && (drive_status & 0x40)) break;
        timeout--;
    }

    if (timeout == 0) {
        kprint("Drive timeout after interrupt!\n");
        dma_transfer_complete = 0;
        return;
    }

    // Clear the interrupt bit in DMA status
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_STATUS, dma_status | 0x04);

    // Signal completion
    dma_transfer_complete = 1;
    kprint("DMA transfer completed successfully\n");
    asm("sti");
}

/** @brief Identify the drive
 * 
 * Due to our approach - to simplify everything, we will only support
 * ATA drives, and we are interested only in one.
 * 
 * 1. Select the drive
 * 2. Send IDENTIFY command
 * 3. Wait for Busy to clear
 * 4. Check if it's an ATA drive
 * 5. Check if it supports DMA
 * 6. Check if it supports UDMA
 * @return 1 if drive is identified and supported, 0 otherwise
 */
int identify_drive() {
    uint8_t status;
    uint16_t identify_data[256];

    // ATA I/O ports (base 0x1F0):
    // ATA_PRIMARY_IO + 0    // Data Register
    // ATA_PRIMARY_IO + 1    // Error Register
    // ATA_PRIMARY_IO + 2    // Sector Count
    // ATA_PRIMARY_IO + 3    // LBA Low (Sector Number)
    // ATA_PRIMARY_IO + 4    // LBA Mid (Cylinder Low)
    // ATA_PRIMARY_IO + 5    // LBA High (Cylinder High)
    // ATA_PRIMARY_IO + 6    // Drive/Head Register
    // ATA_PRIMARY_IO + 7    // Status/Command Register

    // Select the drive (assuming primary master)
    // 1 = Always set
    // 0 = Drive 0 (master)
    // 1 = LBA mode
    // 0 = Reserved
    // 0000 = Reserved
    port_byte_out(ATA_PRIMARY_IO + 6, 0xA0);  // Drive 0, LBA mode
    
    // Zero out sector count and LBA registers
    port_byte_out(ATA_PRIMARY_IO + 2, 0);
    port_byte_out(ATA_PRIMARY_IO + 3, 0);
    port_byte_out(ATA_PRIMARY_IO + 4, 0);
    port_byte_out(ATA_PRIMARY_IO + 5, 0);
    
    // Send IDENTIFY command
    port_byte_out(ATA_PRIMARY_IO + 7, ATA_COMMAND_IDENTIFY);
    
    // Wait for BSY to clear
    status = port_byte_in(ATA_PRIMARY_IO + 7);
    if (status == 0) {
        kprint("Drive does not exist\n");
        return 0;
    }
    
    // Wait until ready
    while ((status & 0x80) != 0) {  // While Busy
        status = port_byte_in(ATA_PRIMARY_IO + 7);
    }
    
    // Check if it's an ATA drive
    uint8_t cl = port_byte_in(ATA_PRIMARY_IO + 4);
    uint8_t ch = port_byte_in(ATA_PRIMARY_IO + 5);
    
    if (cl == 0x14 && ch == 0xEB) {
        kprint("PATAPI drive detected, not supported\n");
        return 0;
    } else if (cl == 0x69 && ch == 0x96) {
        kprint("SATAPI drive detected, not supported\n");
        return 0;
    }
    
    // Wait for Data Ready or Error
    while (1) {
        status = port_byte_in(ATA_PRIMARY_IO + 7);
        if ((status & 0x08) || (status & 0x01)) break;  // DRQ or ERR set
    }
    
    if (status & 0x01) {
        kprint("Error during IDENTIFY\n");
        return 0;
    }
    
    // Read identification space
    for (int i = 0; i < 256; i++) {
        identify_data[i] = port_word_in(ATA_PRIMARY_IO + 0);
    }
    
    // Check capabilities
    uint16_t capabilities = identify_data[49];
    uint16_t dma_modes = identify_data[88];
    
    if (!(capabilities & ATA_CAP_DMA)) {
        kprint("Drive does not support DMA\n");
        return 0;
    }
    
    if (!(capabilities & ATA_CAP_LBA)) {
        kprint("Drive does not support LBA\n");
        return 0;
    }
    
    // Print some drive information
    kprint("Drive supports DMA: Yes\n");
    // Faster DMA check: (dma_modes & ATA_CAP_UDMA)
    
    return 1;
}

/** @brief Initialize ATA DMA
 *  1. Enable Bus Mastering 
 *  2. Verify drive supports DMA
 *  3. Enable interrupts
 */
void init_ata_dma() {
    // Enable Bus Master DMA in PCI configuration
    enable_bus_mastering(IDE_PCI_BUS, IDE_PCI_DEVICE, IDE_PCI_FUNCTION);
        
    // Verify drive supports DMA
    if (!identify_drive()) {
        kprint("Failed to initialize ATA DMA - drive not compatible\n");
        return;
    }
    
    //  Enable interrupts in device control register
    port_byte_out(ATA_PRIMARY_CTRL, 0x00); 
    
    // Read status register to clear any pending interrupts
    port_byte_in(ATA_PRIMARY_IO + 7);
    
    register_interrupt_handler(IRQ14, ata_dma_interrupt_handler);
    
    kprint("ATA DMA initialized successfully\n");
}

/** @brief Read sectors from ATA drive using DMA
 * 
 * @param lba LBA address
 * @param drive Drive number (0 or 1)
 * @param num_sectors Number of sectors to read
 * @param buffer Buffer to store the data
 */
void ata_dma_read(uint32_t lba, uint8_t drive, uint16_t num_sectors, void* buffer) {
    uint32_t bmide_base = get_bmide_base();
    
    kprint("BEGIN ATA DMA READ\n");
    dma_transfer_complete = 0;

    // Clear any pending interrupts
    port_byte_in(ATA_PRIMARY_IO + 7);
    
    // Clear DMA status
    uint8_t dma_status = port_byte_in(bmide_base + ATA_DMA_PRIMARY_STATUS);
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_STATUS, dma_status | 0x04 | 0x02);
    
    // Setup PRD table
    static PRDEntry prd_table[1] __attribute__((aligned(4)));
    prd_table[0].phys_addr = (uint32_t)buffer;
    prd_table[0].byte_count = num_sectors * 512;
    prd_table[0].end_of_table = 0x8000;

    // Load PRD table address
    port_dword_out(bmide_base + ATA_DMA_PRIMARY_PRD, (uint32_t)prd_table);
    
    // Stop the DMA engine and set read mode
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0x08); 

    // 0xE0 = LBA mode with drive 0
    // (drive & 1) << 4 = Select Drive 0 or 1 - for us it's always 0
    // ((lba >> 24) & 0x0F) = LBA high nibbles
    port_byte_out(ATA_PRIMARY_IO + 6, 0xE0 | ((drive & 1) << 4) | ((lba >> 24) & 0x0F));
    
    // Wait for drive to be ready
    uint8_t status;
    do {
        status = port_byte_in(ATA_PRIMARY_IO + 7);
    } while (status & 0x80);  // Wait while busy
    
    // Set up other registers
    port_byte_out(ATA_PRIMARY_IO + 2, num_sectors);
    port_byte_out(ATA_PRIMARY_IO + 3, (uint8_t)lba);
    port_byte_out(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 8));
    port_byte_out(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 16));

    kprint("Sending READ DMA command...\n");
    port_byte_out(ATA_PRIMARY_IO + 7, ATA_COMMAND_READ_DMA);

    // Wait a bit for drive to process command
    // This is a workaround for the drive not being ready immediately after sending the command
    for(int i = 0; i < 4; i++) {
        port_byte_in(ATA_PRIMARY_IO + 7);
    }

    // Start the DMA transfer
    kprint("Starting DMA transfer...\n");
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0x09);  // Start bit + Read bit
}

/** @brief Write sectors to ATA drive using DMA
 * 
 * @param lba LBA address
 * @param drive Drive number (0 or 1)
 * @param num_sectors Number of sectors to write
 * @param buffer Buffer to store the data
 */
void ata_dma_write(uint32_t lba, uint8_t drive, uint16_t num_sectors, void* buffer) {
    uint32_t bmide_base = get_bmide_base();
    
    kprint("BEGIN ATA DMA WRITE\n");
    dma_transfer_complete = 0;

    // Clear any pending interrupts
    port_byte_in(ATA_PRIMARY_IO + 7);
    
    // Clear DMA status
    uint8_t dma_status = port_byte_in(bmide_base + ATA_DMA_PRIMARY_STATUS);
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_STATUS, dma_status | 0x04 | 0x02);
    
    // Setup PRD table
    static PRDEntry prd_table[1] __attribute__((aligned(4)));
    prd_table[0].phys_addr = (uint32_t)buffer;
    prd_table[0].byte_count = num_sectors * 512;
    prd_table[0].end_of_table = 0x8000;

    // Load PRD table address
    port_dword_out(bmide_base + ATA_DMA_PRIMARY_PRD, (uint32_t)prd_table);
    
    // Stop the DMA engine and set write mode (note: no read bit set)
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0x00);  

    // Select drive and set up registers
    // Same as in read function
    port_byte_out(ATA_PRIMARY_IO + 6, 0xE0 | ((drive & 1) << 4) | ((lba >> 24) & 0x0F));
    
    // Wait for drive to be ready
    uint8_t status;
    do {
        status = port_byte_in(ATA_PRIMARY_IO + 7);
    } while (status & 0x80);  // Wait while busy
    
    // Set up other registers
    port_byte_out(ATA_PRIMARY_IO + 2, num_sectors);
    port_byte_out(ATA_PRIMARY_IO + 3, (uint8_t)lba);
    port_byte_out(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 8));
    port_byte_out(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 16));

    kprint("Sending WRITE DMA command...\n");
    port_byte_out(ATA_PRIMARY_IO + 7, ATA_COMMAND_WRITE_DMA);

    // Wait for drive to be ready to accept data
    do {
        status = port_byte_in(ATA_PRIMARY_IO + 7);
    } while ((status & 0x80) || !(status & 0x40));  // Wait while busy and not ready

    // Start the DMA transfer
    kprint("Starting DMA transfer...\n");
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0x01);  // Start bit only (no read bit)
}