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


static flashcart_err_t dd64_init (void) {
    return FLASHCART_OK;
}

static flashcart_err_t dd64_deinit (void) {
    return FLASHCART_OK;
}

static bool d64_has_feature (flashcart_features_t feature) {
    switch (feature) {
        case FLASHCART_FEATURE_64DD: return false;
        case FLASHCART_FEATURE_RTC: return false;
        case FLASHCART_FEATURE_USB: return false;
        default: return false;
    }
}

static flashcart_err_t d64_load_rom (char *rom_path, flashcart_progress_callback_t *progress) {
    FIL fil;
    UINT br;

    if (f_open(&fil, strip_sd_prefix(rom_path), FA_READ) != FR_OK) {
        return FLASHCART_ERR_LOAD;


    daisyDrive_uploadRom(rom_path, offsets, sizeof(rom_path) );
    
    if (f_close(&fil) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    return FLASHCART_OK;
}



static flashcart_t flashcart_d64 = {
    .init = d64_init,
    .deinit = d64_deinit,
    .has_feature = d64_has_feature,
    .load_rom = d64_load_rom,
    .load_file = NULL,
    .load_save = NULL,
    .load_64dd_ipl = NULL,
    .load_64dd_disk = NULL,
    .set_save_type = NULL,
    .set_save_writeback = NULL,
};


flashcart_t *d64_get_flashcart (void) {
    return &flashcart_dd64;
}
