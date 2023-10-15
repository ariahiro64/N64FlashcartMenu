#include <libdragon.h>

#include "ed64_ll.h"



/* ED64 configuration registers base address  */
#define ED64_CONFIG_REGS_BASE (0xA8040000)

typedef enum {
    // REG_CFG = 0,
    // REG_STATUS = 1,
    // REG_DMA_LENGTH = 2,
    // REG_DMA_RAM_ADDR = 3,
    // REG_MSG = 4,
    // REG_DMA_CFG = 5,
    // REG_SPI = 6,
    // REG_SPI_CFG = 7,
    // REG_KEY = 8,
    REG_SAV_CFG = 9,
    // REG_SEC = 10, /* Sectors?? */
    // REG_FPGA_VERSION = 11, /* Altera (Intel) MAX */
    // REG_GPIO = 12,

} ed64_registers_t;


#define SAV_EEP_ON 1
#define SAV_SRM_ON 2
#define SAV_EEP_SIZE 4
#define SAV_SRM_SIZE 8

#define SAV_RAM_BANK 128
#define SAV_RAM_BANK_APPLY 32768

uint32_t ed64_ll_reg_read(uint32_t reg);
void ed64_ll_reg_write(uint32_t reg, uint32_t data);

uint8_t ed64_ll_sram_bank;
ed64_save_type_t ed64_ll_save_type;


uint32_t ed64_ll_reg_read(uint32_t reg) {

    *(volatile uint32_t *) (ED64_CONFIG_REGS_BASE);
    return *(volatile uint32_t *) (ED64_CONFIG_REGS_BASE + reg * 4);
}

void ed64_ll_reg_write(uint32_t reg, uint32_t data) {

    *(volatile uint32_t *) (ED64_CONFIG_REGS_BASE);
    *(volatile uint32_t *) (ED64_CONFIG_REGS_BASE + reg * 4) = data;
    *(volatile uint32_t *) (ROM_ADDRESS);

}


ed64_save_type_t ed64_ll_get_save_type() {

    return ed64_ll_save_type;
}

void ed64_ll_set_save_type(ed64_save_type_t type) {

    uint16_t save_cfg;
    uint8_t eeprom_on, sram_on, eeprom_size, sram_size, ram_bank;
    ed64_ll_save_type = type;
    eeprom_on = 0;
    sram_on = 0;
    eeprom_size = 0;
    sram_size = 0;
    ram_bank = ed64_ll_sram_bank;


    switch (type) {
        case SAVE_TYPE_EEPROM_16K:
            eeprom_on = 1;
            eeprom_size = 1;
            break;
        case SAVE_TYPE_EEPROM_4K:
            eeprom_on = 1;
            break;
        case SAVE_TYPE_SRAM:
            sram_on = 1;
            break;
        case SAVE_TYPE_SRAM_128K:
            sram_on = 1;
            sram_size = 1;
            break;
        case SAVE_TYPE_FLASHRAM:
            sram_on = 0;
            sram_size = 1;
            break;
        default:
            sram_on = 0;
            sram_size = 0;
            ram_bank = 1;
            break;
    }

    save_cfg = 0;
    if (eeprom_on)save_cfg |= SAV_EEP_ON;
    if (sram_on)save_cfg |= SAV_SRM_ON;
    if (eeprom_size)save_cfg |= SAV_EEP_SIZE;
    if (sram_size)save_cfg |= SAV_SRM_SIZE;
    if (ram_bank)save_cfg |= SAV_RAM_BANK;
    save_cfg |= SAV_RAM_BANK_APPLY;

    ed64_ll_reg_write(REG_SAV_CFG, save_cfg);

}

void ed64_ll_set_sram_bank(uint8_t bank) {

    ed64_ll_sram_bank = bank == 0 ? 0 : 1;

}


#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include "types.h"


void PI_Init(void) {
	PI_DMAWait();
	IO_WRITE(PI_STATUS_REG, 0x03);
}

// Inits PI for sram transfer
void PI_Init_SRAM(void) {
	
	IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x05);
	IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x0C);
	IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x0D);
	IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x02);
	
}

void PI_DMAWait(void) {
	  
	while (IO_READ(PI_STATUS_REG) & (PI_STATUS_IO_BUSY | PI_STATUS_DMA_BUSY));
}


void PI_DMAFromSRAM(void *dest, u32 offset, u32 size) {
	

	IO_WRITE(PI_DRAM_ADDR_REG, K1_TO_PHYS(dest));
	IO_WRITE(PI_CART_ADDR_REG, (0xA8000000 + offset));
	 asm volatile ("" : : : "memory");
	IO_WRITE(PI_WR_LEN_REG, (size - 1));
	 asm volatile ("" : : : "memory");
 
}


void PI_DMAToSRAM(void *src, u32 offset, u32 size) { //void*
	PI_DMAWait();

	IO_WRITE(PI_STATUS_REG, 2);
	IO_WRITE(PI_DRAM_ADDR_REG, K1_TO_PHYS(src));
	IO_WRITE(PI_CART_ADDR_REG, (0xA8000000 + offset));
	IO_WRITE(PI_RD_LEN_REG, (size - 1));
}

void PI_DMAFromCart(void* dest, void* src, u32 size) {
	PI_DMAWait();

	IO_WRITE(PI_STATUS_REG, 0x03);
	IO_WRITE(PI_DRAM_ADDR_REG, K1_TO_PHYS(dest));
	IO_WRITE(PI_CART_ADDR_REG, K0_TO_PHYS(src));
	IO_WRITE(PI_WR_LEN_REG, (size - 1));
}


void PI_DMAToCart(void* dest, void* src, u32 size) {
	PI_DMAWait();

	IO_WRITE(PI_STATUS_REG, 0x02);
	IO_WRITE(PI_DRAM_ADDR_REG, K1_TO_PHYS(src));
	IO_WRITE(PI_CART_ADDR_REG, K0_TO_PHYS(dest));
	IO_WRITE(PI_RD_LEN_REG, (size - 1));
}


// Wrapper to support unaligned access to memory
void PI_SafeDMAFromCart(void *dest, void *src, u32 size) {
	if (!dest || !src || !size) return;

	u32 unalignedSrc  = ((u32)src)  % 2;
	u32 unalignedDest = ((u32)dest) % 8;

	//FIXME: Do i really need to check if size is 16bit aligned?
	if (!unalignedDest && !unalignedSrc && !(size % 2)) {
		PI_DMAFromCart(dest, src, size);
		PI_DMAWait();

		return;
	}

	void* newSrc = (void*)(((u32)src) - unalignedSrc);
	u32 newSize = (size + unalignedSrc) + ((size + unalignedSrc) % 2);

	u8 *buffer = memalign(8, newSize);
	PI_DMAFromCart(buffer, newSrc, newSize);
	PI_DMAWait();

	memcpy(dest, (buffer + unalignedSrc), size);

	free(buffer);
}

#include "types.h"

int getSRAM( uint8_t *buffer, int size){
    dma_wait();

    IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x05);
    IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x0C);
    IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x0D);
    IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x02);

    dma_wait();

    PI_Init();

    dma_wait();

    PI_DMAFromSRAM(buffer, 0, size) ;

    dma_wait();

    IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x40);
    IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x12);
    IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x07);
    IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x03);

    return 1;
}

int getEeprom(  uint8_t *buffer, int size){
    int blocks=size/8;
    for( int b = 0; b < blocks; b++ ) {
        eeprom_read( b, &buffer[b * 8] );
    }

    return 1;
}


/*
sram upload
*/
int setSRAM(  uint8_t *buffer, int size){
     //half working
    PI_DMAWait();
    //Timing
    PI_Init_SRAM();

    //Readmode
    PI_Init();

    data_cache_hit_writeback_invalidate(buffer,size);
    dma_wait();
    PI_DMAToSRAM(buffer, 0, size);
    data_cache_hit_writeback_invalidate(buffer,size);

    //Wait
    PI_DMAWait();
    //Restore evd Timing
    setSDTiming();

    return 1;
}


int setEeprom(uint8_t *buffer, int size){
    int blocks=size/8;
    for( int b = 0; b < blocks; b++ ) {
        eeprom_write( b, &buffer[b * 8] );
    }

    return 1;
}

void setSDTiming(void){

    // PI_DMAWait();
    IO_WRITE(PI_BSD_DOM1_LAT_REG, 0x40);
    IO_WRITE(PI_BSD_DOM1_PWD_REG, 0x12);
    IO_WRITE(PI_BSD_DOM1_PGS_REG, 0x07);
    IO_WRITE(PI_BSD_DOM1_RLS_REG, 0x03);

    IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x40);
    IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x12);
    IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x07);
    IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x03);
}


void restoreTiming(void) {
    //n64 timing restore :>
    IO_WRITE(PI_BSD_DOM1_LAT_REG, 0x40);
    IO_WRITE(PI_BSD_DOM1_PWD_REG, 0x12);
    IO_WRITE(PI_BSD_DOM1_PGS_REG, 0x07);
    IO_WRITE(PI_BSD_DOM1_RLS_REG, 0x03);

    IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x40);
    IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x12);
    IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x07);
    IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x03);
}