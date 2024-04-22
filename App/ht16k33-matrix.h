/**
 *
 * Microvisor Weather Device Demo
 *
 * Copyright © 2024, KORE Wireless
 * Licence: MIT
 *
 */
#ifndef _HT16K33_MATRIX_HEADER_
#define _HT16K33_MATRIX_HEADER_


/*
 * CONSTANTS
 */
#define     HT16K33_I2C_ADDR                0x70
#define     HT16K33_CMD_POWER_ON            0x21
#define     HT16K33_CMD_DISPLAY_ON          0x81
#define     HT16K33_CMD_BRIGHTNESS          0xE0


#ifdef __cplusplus
extern "C" {
#endif


/*
 * PROTOTYPES
 */
void        HT16K33_init(uint8_t angle);
void        HT16K33_draw(void);
void        HT16K33_clear_buffer(void);
void        HT16K33_set_brightness(uint8_t brightness);
void        HT16K33_plot(uint8_t x, uint8_t y, bool is_set);
void        HT16K33_print(const char *text, uint32_t delay_ms);
void        HT16K33_define_character(const char* sprite, uint8_t index);
void        HT16K33_draw_def_char(uint8_t v);


#ifdef __cplusplus
}
#endif


#endif  // _HT16K33_MATRIX_HEADER_
