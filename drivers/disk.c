#include "disk.h"
#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "screen.h"
#include <stdint.h>

// Define the PCI Command register offset and Bus Master bit
#define PCI_COMMAND 0x04
#define PCI_BUS_MASTER 0x04

// PCI Configuration Space offsets
#define PCI_BAR4           0x20    // Bus Master Base Address Register
#define PCI_COMMAND        0x04    // Command Register
#define PCI_BUS_MASTER     0x04    // Bus Master bit in Command Register

// IDE Controller typical PCI location
#define IDE_PCI_BUS        0x00
#define IDE_PCI_DEVICE     0x01    // Try 0x01, 0x1F, or 0x0C
#define IDE_PCI_FUNCTION   0x01    // Try 0x00 or 0x01

// Function to read from PCI configuration space
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;

    // Create configuration address
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

    // Write out the address
    port_dword_out(0xCF8, address);

    // Read in the data
    tmp = (uint16_t)((port_dword_in(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return tmp;
}

// Function to write to PCI configuration space
void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // Create configuration address
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

    // Write out the address
    port_dword_out(0xCF8, address);

    // Write the data
    port_word_out(0xCFC, value);
}

// Function to enable bus mastering for a specific PCI device
void enable_bus_mastering(uint8_t bus, uint8_t slot, uint8_t func) {
    // Read the current PCI Command register value
    uint16_t command = pci_config_read_word(bus, slot, func, PCI_COMMAND);
    
    // Set the Bus Master bit
    command |= PCI_BUS_MASTER;
    
    // Write back the updated command
    pci_config_write_word(bus, slot, func, PCI_COMMAND, command);
}

// Function to get the Bus Master IDE base address
uint32_t get_bmide_base() {
    uint32_t bar4 = pci_config_read_word(IDE_PCI_BUS, IDE_PCI_DEVICE, IDE_PCI_FUNCTION, PCI_BAR4);
    bar4 &= 0xFFF0;  // Mask off the lower 4 bits
    return bar4;
}

// Example usage
void setup_ata_device() {
    uint8_t ata_bus = 0;   // Replace with actual bus number
    uint8_t ata_slot = 1;  // Replace with actual slot number
    uint8_t ata_func = 0;  // Replace with actual function number

    enable_bus_mastering(ata_bus, ata_slot, ata_func);
}

static volatile int dma_transfer_complete = 0;

static void ata_dma_interrupt_handler() {
    kprint("ATA DMA interrupt received!\n");

    uint8_t drive_status = port_byte_in(ATA_PRIMARY_IO + 7);
    uint8_t dma_status = port_byte_in(ATA_PRIMARY_CTRL + ATA_DMA_PRIMARY_STATUS);

    kprint("Drive Status: ");
    kprint_hex(drive_status);
    kprint("\nDMA Status: ");
    kprint_hex(dma_status);
    kprint("\n");

    // First, stop the DMA transfer
    port_byte_out(ATA_PRIMARY_CTRL + ATA_DMA_PRIMARY_CMD, 0x00);

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

    // Wait for BSY to clear and RDY to set
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
    port_byte_out(ATA_PRIMARY_CTRL + ATA_DMA_PRIMARY_STATUS, dma_status | 0x04);

    // Signal completion
    dma_transfer_complete = 1;
    kprint("DMA transfer completed successfully\n");

    uint32_t* ptr = (uint32_t*)0xFFFF;
    kprint_hex(*ptr);

}

void clear_dma_status(uint16_t base_address) {
    // Read the current status
    uint8_t status = port_byte_in(base_address + ATA_DMA_PRIMARY_STATUS);

    // Clear the Error and Interrupt bits by writing a 1 to them
    port_byte_out(base_address + ATA_DMA_PRIMARY_STATUS, status | 0x06);
}

int identify_drive() {
    uint8_t status;
    uint16_t identify_data[256];  // 512 bytes buffer for IDENTIFY data

    // Select the drive (assuming primary master)
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
    while ((status & 0x80) != 0) {  // While BSY
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
    
    // Wait for DRQ or ERR
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
    if (dma_modes & ATA_CAP_UDMA) {
        kprint("Drive supports UDMA\n");
    }
    
    return 1;
}

void init_ata_dma() {
    // Enable Bus Master DMA in PCI configuration
    enable_bus_mastering(IDE_PCI_BUS, IDE_PCI_DEVICE, IDE_PCI_FUNCTION);
    
    // Get the correct Bus Master IDE base address
    uint32_t bmide_base = get_bmide_base();
    kprint("BMIDE Base: ");
    kprint_hex(bmide_base);
    kprint("\n");
        
    // Verify drive supports DMA
    if (!identify_drive()) {
        kprint("Failed to initialize ATA DMA - drive not compatible\n");
        return;
    }
    
    //  Enable interrupts in device control register
    port_byte_out(ATA_PRIMARY_CTRL, 0x00); 
    
    // Read status register to clear any pending interrupts
    port_byte_in(ATA_PRIMARY_IO + 7);
    
    // Enable IRQ14 in the PIC
    port_byte_out(0x21, port_byte_in(0x21) & ~(1 << 6));
    // Enable IRQ14 in slave PIC
    port_byte_out(0xA1, port_byte_in(0xA1) & ~(1 << 6));
    // Configure cascade in master PIC
    port_byte_out(0x21, port_byte_in(0x21) & ~(1 << 2));  // Enable cascade

    register_interrupt_handler(IRQ14, ata_dma_interrupt_handler);
    
    kprint("ATA DMA initialized successfully\n");
}

// Add these definitions if you don't have them
#define BM_STATUS_DMA_ACTIVE    (1 << 0)  // Bit 0: DMA active
#define BM_STATUS_ERROR         (1 << 1)  // Bit 1: Error
#define BM_STATUS_IRQ          (1 << 2)  // Bit 2: Interrupt
#define BM_CMD_START_BIT       (1 << 0)  // Bit 0: Start/Stop
#define BM_CMD_READ_BIT        (1 << 3)  // Bit 3: Read/Write

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
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0);
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0x08);  // Read operation

    // Select drive and set up registers
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
    for(int i = 0; i < 4; i++) {
        port_byte_in(ATA_PRIMARY_IO + 7);
    }

    // Start the DMA transfer
    kprint("Starting DMA transfer...\n");
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0x09);  // Start bit + Read bit

    // Wait for completion with timeout
    int timeout = 1000000;
    while (!dma_transfer_complete && timeout > 0) {
        if ((timeout % 100000) == 0) {
            status = port_byte_in(ATA_PRIMARY_IO + 7);
            dma_status = port_byte_in(bmide_base + ATA_DMA_PRIMARY_STATUS);
            kprint("Waiting... Drive Status: ");
            kprint_hex(status);
            kprint(" DMA Status: ");
            kprint_hex(dma_status);
            kprint("\n");
        }
        timeout--;
    }

    // Stop DMA engine regardless of outcome
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0);

    if (timeout == 0) {
        kprint("DMA read timeout!\n");
        return;
    }

    kprint("DMA READ COMPLETE\n");
    
    // Verify data was transferred
    uint8_t* data = (uint8_t*)buffer;
    kprint("First few bytes: ");
    for(int i = 0; i < 4; i++) {
        kprint_hex(data[i]);
        kprint(" ");
    }
    kprint("\n");
}


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
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0);
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0x00);  // Write operation (no bit 3)

    // Select drive and set up registers
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

    // Wait for completion with timeout
    int timeout = 1000000;
    while (!dma_transfer_complete && timeout > 0) {
        if ((timeout % 100000) == 0) {
            status = port_byte_in(ATA_PRIMARY_IO + 7);
            dma_status = port_byte_in(bmide_base + ATA_DMA_PRIMARY_STATUS);
            kprint("Waiting... Drive Status: ");
            kprint_hex(status);
            kprint(" DMA Status: ");
            kprint_hex(dma_status);
            kprint("\n");
        }
        timeout--;
    }

    // Stop DMA engine
    port_byte_out(bmide_base + ATA_DMA_PRIMARY_CMD, 0);

    if (timeout == 0) {
        kprint("DMA write timeout!\n");
        return;
    }

    // Wait for drive to finish internal write operations
    timeout = 1000000;
    do {
        status = port_byte_in(ATA_PRIMARY_IO + 7);
        if (timeout-- == 0) {
            kprint("Drive busy timeout after write!\n");
            return;
        }
    } while (status & 0x80);  // Wait while busy

    if (status & 0x01) {  // Check for error
        kprint("Write error occurred! Status: ");
        kprint_hex(status);
        kprint("\n");
        return;
    }

    kprint("DMA WRITE COMPLETE\n");
}


int check_dma_transfer_complete() {
    return dma_transfer_complete;
}