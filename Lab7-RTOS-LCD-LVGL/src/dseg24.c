/*******************************************************************************
 * Size: 24 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/

#define LV_LVGL_H_INCLUDE_SIMPLE
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef DSEG24
#define DSEG24 1
#endif

#if DSEG24

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0043 "C" */
    0x3f, 0xf8, 0xff, 0xcc, 0x0, 0x30, 0x0, 0xc0,
    0x3, 0x0, 0xc, 0x0, 0x30, 0x0, 0xc0, 0x3,
    0x0, 0xc, 0x0, 0x3f, 0xfc, 0xbf, 0xfc,

    /* U+004D "M" */
    0xff, 0xff, 0x7f, 0xf7, 0x0, 0x1e, 0x0, 0x3c,
    0x0, 0x78, 0x0, 0xf0, 0x1, 0xe0, 0x3, 0xc0,
    0x7, 0x80, 0xf, 0x0, 0xe, 0x0, 0x0, 0x0,
    0x78, 0x0, 0xf0, 0x1, 0xe0, 0x3, 0xc0, 0x7,
    0x80, 0xf, 0x0, 0x1e, 0x0, 0x3c, 0x0, 0x78,
    0x0, 0xf0, 0x0, 0xc0, 0x0,

    /* U+006E "n" */
    0x3f, 0xf8, 0x7f, 0xff, 0x0, 0x1e, 0x0, 0x3c,
    0x0, 0x78, 0x0, 0xf0, 0x1, 0xe0, 0x3, 0xc0,
    0x7, 0x80, 0xf, 0x0, 0x1e, 0x0, 0x18, 0x0,
    0x0,

    /* U+006F "o" */
    0x3f, 0xf8, 0x7f, 0xff, 0x0, 0x1e, 0x0, 0x3c,
    0x0, 0x78, 0x0, 0xf0, 0x1, 0xe0, 0x3, 0xc0,
    0x7, 0x80, 0xf, 0x0, 0x1f, 0xff, 0xdb, 0xff,
    0xe0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 313, .box_w = 14, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 23, .adv_w = 313, .box_w = 15, .box_h = 24, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 313, .box_w = 15, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 93, .adv_w = 313, .box_w = 15, .box_h = 13, .ofs_x = 2, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0xa, 0x2b, 0x2c
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 67, .range_length = 45, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 4, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t dseg24 = {
#else
lv_font_t dseg24 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 24,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -3,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if DSEG24*/

