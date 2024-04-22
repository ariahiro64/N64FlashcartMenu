#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <fatfs/ff.h>
#include <libdragon.h>
#include <libcart/cart.h>

#include "utils/fs.h"
#include "utils/utils.h"

#include "../flashcart_utils.h"
#include "daisydrive64_ll.h"
#include "daisydrive64.h"
#include "daisydrive64_state.h"


static daisydrive64_pseudo_writeback_t current_state;

extern int ed_exit (void);

static flashcart_err_t daisydrive64_init (void) {
    daisydrive64_state_load(&current_state);

    return FLASHCART_OK;
}

static flashcart_err_t daisydrive64_deinit (void) {

    // For the moment, just use libCart exit.
    ed_exit();

    return FLASHCART_OK;
}

static bool daisydrive64_has_feature (flashcart_features_t feature) {
            return false;
}

static flashcart_err_t daisydrive64_load_rom (char *rom_path, flashcart_progress_callback_t *progress) {

    FIL fil;
    UINT br;

    if (f_open(&fil, strip_sd_prefix(rom_path), FA_READ) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    fix_file_size(&fil);

    size_t rom_size = f_size(&fil);
    
    size_t sdram_size = rom_size;

    size_t chunk_size = MiB(1);
    for (int offset = 0; offset < sdram_size; offset += chunk_size)
    {
        size_t block_size = MIN(sdram_size - offset, chunk_size);
        if (f_read(&fil, (void *)(ROM_ADDRESS + offset), block_size, &br) != FR_OK) {
            f_close(&fil);
            return FLASHCART_ERR_LOAD;
        }
        if (progress) {
            progress(f_tell(&fil) / (float)(f_size(&fil)));
        }
    }

    if (f_close(&fil) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    return FLASHCART_OK;
}

static flashcart_err_t daisydrive64_load_file (char *file_path, uint32_t rom_offset, uint32_t file_offset)
{
    FIL fil;
    UINT br;

    if (f_open(&fil, strip_sd_prefix(file_path), FA_READ) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    fix_file_size(&fil);

    size_t file_size = f_size(&fil) - file_offset;

    // FIXME: if the cart is not V3 or X5 or X7, we need probably need to - 128KiB for save compatibility.
    // Or somehow warn that certain ROM's will have corruption due to the address space being used for saves.

    if (file_size > (MiB(64) - rom_offset)) {
        f_close(&fil);
        return FLASHCART_ERR_ARGS;
    }

    if (f_lseek(&fil, file_offset) != FR_OK) {
        f_close(&fil);
        return FLASHCART_ERR_LOAD;
    }

    if (f_read(&fil, (void *)(ROM_ADDRESS + rom_offset), file_size, &br) != FR_OK) {
        f_close(&fil);
        return FLASHCART_ERR_LOAD;
    }
    if (br != file_size) {
        f_close(&fil);
        return FLASHCART_ERR_LOAD;
    }

    if (f_close(&fil) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    return FLASHCART_OK;
}

static flashcart_err_t daisydrive64_load_save (char *save_path) {



    daisydrive64_state_save(&current_state);

    return FLASHCART_OK;
}

static flashcart_err_t daisydrive64_set_save_type (flashcart_save_type_t save_type) {
    daisydrive64_save_type_t type;

    switch (save_type) {
    case FLASHCART_SAVE_TYPE_NONE:
        type = SAVE_TYPE_NONE;
        break;
    case FLASHCART_SAVE_TYPE_EEPROM_4K:
        type = SAVE_TYPE_EEPROM_4K;
        break;
    case FLASHCART_SAVE_TYPE_EEPROM_16K:
        type = SAVE_TYPE_EEPROM_16K;
        break;
    case FLASHCART_SAVE_TYPE_SRAM:
        type = SAVE_TYPE_SRAM;
        break;
    case FLASHCART_SAVE_TYPE_SRAM_BANKED:
    case FLASHCART_SAVE_TYPE_SRAM_128K:
        type = SAVE_TYPE_SRAM_128K;
        break;
    case FLASHCART_SAVE_TYPE_FLASHRAM_PKST2:
    case FLASHCART_SAVE_TYPE_FLASHRAM:
        type = SAVE_TYPE_FLASHRAM;
        break;
    default:
        return FLASHCART_ERR_ARGS;
    }

    daisydrive64_ll_set_save_type(type);

    return FLASHCART_OK;
}

static flashcart_t flashcart_daisydrive64 = {
    .init = daisydrive64_init,
    .deinit = daisydrive64_deinit,
    .has_feature = daisydrive64_has_feature,
    .load_rom = daisydrive64_load_rom,
    .load_file = daisydrive64_load_file,
    .load_save = daisydrive64_load_save,
    .load_64dd_ipl = NULL,
    .load_64dd_disk = NULL,
    .set_save_type = daisydrive64_set_save_type,
    .set_save_writeback = NULL,
};

flashcart_t *daisydrive64_get_flashcart (void) {
    return &flashcart_daisydrive64;
}