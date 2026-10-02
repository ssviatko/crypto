/**
 *
 * C Terminal Color Printing API
 * 2025/Nov/23 - Revision 0.80 alpha
 *
 * Created by: Stephen Sviatko
 *
 * (C) 2025 Good Neighbors LLC - All Rights Reserved, except where noted
 *
 * This file and any intellectual property (designs, algorithms, formulas,
 * procedures, trademarks, and related documentation) contained herein are
 * property of Good Neighbors, an Arizona Limited Liability Company.
 *
 * LICENSING INFORMATION
 *
 * This file may not be distributed in any modified form without expressed
 * written permission of Good Neighbors LLC or its regents. Permission is
 * granted to use this file in any non-commercial, non-governmental capacity
 * (such as student projects, hobby projects, etc) without an official
 * licensing agreement as long as the original author(s) are credited in any
 * derivative work.
 *
 * Commercial licensing of this content is available, any agreement must
 * include consulting services as part of a deployment strategy. For more
 * information, please contact Stephen Sviatko at the following email address:
 *
 * ssviatko@gmail.com
 *
 * @file color_print.c
 * @brief C Terminal Color Printing API
 *
 * Extends printf and fprintf to enable color ANSI printing to the terminal.
 *
 */

#include "color_print.h"

static char g_ansi_highlight[ANSIBUFFLEN];
static char g_ansi_heading[ANSIBUFFLEN];
static char g_ansi_error[ANSIBUFFLEN];
static char g_ansi_default[ANSIBUFFLEN];
static char g_ansi_bullet[ANSIBUFFLEN];
static char g_ansi_debug[ANSIBUFFLEN];

static int g_nocolor; ///< Set to 1 to disable color printing
static int g_debug;   ///< Set to 1 to enable debug printing
static pthread_mutex_t g_debug_mtx; ///< protect debug messages in multithreaded environment

static uint16_t paren_opts[8];
static uint16_t paren_count;
static char paren_char;
static char paren_assembly[BUFFLEN];
static uint16_t paren_assembly_len;

char g_output[BUFFLEN];
char g_blend[8192];

const uint8_t gs_standard_colors[][3] = {
    { 0x00, 0x00, 0x00 }, // Black
    { 0xdd, 0x00, 0x33 }, // Deep Red
    { 0x00, 0x00, 0x99 }, // Dark Blue
    { 0xdd, 0x22, 0xdd }, // Purple
    { 0x00, 0x77, 0x22 }, // Dark Green
    { 0x55, 0x55, 0x55 }, // Dark Gray
    { 0x22, 0x22, 0xff }, // Medium Blue
    { 0x66, 0xaa, 0xff }, // Light Blue
    { 0x88, 0x55, 0x00 }, // Brown
    { 0xff, 0x66, 0x00 }, // Orange
    { 0xaa, 0xaa, 0xaa }, // Light Gray
    { 0xff, 0x99, 0x88 }, // Pink
    { 0x11, 0xdd, 0x00 }, // Light Green
    { 0xff, 0xff, 0x00 }, // Yellow
    { 0x44, 0xff, 0x99 }, // Aquamarine
    { 0xff, 0xff, 0xff }  // White
};

void color_init(const int a_nocolor, const int a_debug)
{
    g_nocolor = a_nocolor;
    g_debug = a_debug;
    pthread_mutex_init(&g_debug_mtx, NULL);
}

void color_set_nocolor(const int a_nocolor)
{
    g_nocolor = a_nocolor;
}

void color_set_debug(const int a_debug)
{
    g_debug = a_debug;
}

void color_set_theme(cp_theme_t a_theme)
{
    g_ansi_highlight[0] = 0;
    g_ansi_heading[0] = 0;
    g_ansi_error[0] = 0;
    g_ansi_debug[0] = 0;
    g_ansi_bullet[0] = 0;
    g_ansi_default[0] = 0;

    switch (a_theme) {
        default:
        case THEME_GREEN:
        {
            strcpy(g_ansi_highlight, CP_GREEN_COLOR_HIGHLIGHT);
            strcpy(g_ansi_heading, CP_GREEN_COLOR_HEADING);
            strcpy(g_ansi_error, CP_GREEN_COLOR_ERROR);
            strcpy(g_ansi_default, CP_GREEN_COLOR_DEFAULT);
            strcpy(g_ansi_debug, CP_GREEN_COLOR_DEBUG);
            strcpy(g_ansi_bullet, CP_GREEN_COLOR_BULLET);
        }
        break;
        case THEME_BLUE:
        {
            strcpy(g_ansi_highlight, CP_BLUE_COLOR_HIGHLIGHT);
            strcpy(g_ansi_heading, CP_BLUE_COLOR_HEADING);
            strcpy(g_ansi_error, CP_BLUE_COLOR_ERROR);
            strcpy(g_ansi_default, CP_BLUE_COLOR_DEFAULT);
            strcpy(g_ansi_debug, CP_BLUE_COLOR_DEBUG);
            strcpy(g_ansi_bullet, CP_BLUE_COLOR_BULLET);
        }
        break;
        case THEME_RED:
        {
            strcpy(g_ansi_highlight, CP_RED_COLOR_HIGHLIGHT);
            strcpy(g_ansi_heading, CP_RED_COLOR_HEADING);
            strcpy(g_ansi_error, CP_RED_COLOR_ERROR);
            strcpy(g_ansi_default, CP_RED_COLOR_DEFAULT);
            strcpy(g_ansi_debug, CP_RED_COLOR_DEBUG);
            strcpy(g_ansi_bullet, CP_RED_COLOR_BULLET);
        }
        break;
        case THEME_PURPLE:
        {
            strcpy(g_ansi_highlight, CP_PURPLE_COLOR_HIGHLIGHT);
            strcpy(g_ansi_heading, CP_PURPLE_COLOR_HEADING);
            strcpy(g_ansi_error, CP_PURPLE_COLOR_ERROR);
            strcpy(g_ansi_default, CP_PURPLE_COLOR_DEFAULT);
            strcpy(g_ansi_debug, CP_PURPLE_COLOR_DEBUG);
            strcpy(g_ansi_bullet, CP_PURPLE_COLOR_BULLET);
        }
        break;
    }
}

void color_free()
{
    pthread_mutex_destroy(&g_debug_mtx);
}

void color_progress(uint32_t a_sofar, uint32_t a_total)
{
    static size_t l_lastsize = 0;
    int i;
    char l_txt[BUFFLEN];

    if (a_sofar == 0)
        l_lastsize = 0; // start fresh

    // cover over our previous message
    for (i = 0; i < l_lastsize; ++i)
            printf("\b");
    for (i = 0; i < l_lastsize; ++i)
        printf(" ");
    for (i = 0; i < l_lastsize; ++i)
        printf("\b");

    // print our message to l_txt to gauge the size on screen
    sprintf(l_txt, "(%u of %u) ", a_sofar, a_total);
    l_lastsize = strlen(l_txt);
    // now print it on screen in color with ansi escape codes
    color_printf("(*h%u*d of *h%u*d) ", a_sofar, a_total);
}

char *color_rgb(uint8_t a_red, uint8_t a_green, uint8_t a_blue)
{
    g_output[0] = 0;
    sprintf(g_output, "\033[38;2;%d;%d;%dm", a_red, a_green, a_blue);
    return g_output;
}

char *color_rgb_bg(uint8_t a_red, uint8_t a_green, uint8_t a_blue)
{
    g_output[0] = 0;
    sprintf(g_output, "\033[48;2;%d;%d;%dm", a_red, a_green, a_blue);
    return g_output;
}

char *color_gs(int a_color)
{
    return color_rgb(gs_standard_colors[a_color][0], gs_standard_colors[a_color][1], gs_standard_colors[a_color][2]);
}

char *color_gs_bg(int a_color)
{
    return color_rgb_bg(gs_standard_colors[a_color][0], gs_standard_colors[a_color][1], gs_standard_colors[a_color][2]);
}

char *color_rgb_blend(const char *a_str, uint8_t a_begin_red, uint8_t a_begin_green, uint8_t a_begin_blue, uint8_t a_end_red, uint8_t a_end_green, uint8_t a_end_blue, int a_background)
{
    unsigned int l_strlen = strlen(a_str);
    g_blend[0] = 0;
    if (l_strlen == 0) {
       return g_blend;
    }
    if (l_strlen == 1) {
        if (!g_nocolor) strcat(g_blend, color_rgb(a_begin_red, a_begin_green, a_begin_blue));
        strcat(g_blend, a_str);
        if (!g_nocolor) strcat(g_blend, CP_GREEN_COLOR_DEFAULT);
        return g_blend;
    }
    // at least 2 chars long so blend it
    for (unsigned int i = 0; i < l_strlen; ++i) {
        double l_red = (double)a_begin_red + ((((double)a_end_red - (double)a_begin_red) / (double)(l_strlen - 1)) * (double)i);
        double l_green = (double)a_begin_green + ((((double)a_end_green - (double)a_begin_green) / (double)(l_strlen - 1)) * (double)i);
        double l_blue = (double)a_begin_blue + ((((double)a_end_blue - (double)a_begin_blue) / (double)(l_strlen - 1)) * (double)i);
        if (!g_nocolor) {
            if (a_background)
                strcat(g_blend, color_rgb_bg((uint8_t)l_red, (uint8_t)l_green, (uint8_t)l_blue));
            else
                strcat(g_blend, color_rgb((uint8_t)l_red, (uint8_t)l_green, (uint8_t)l_blue));
        }
        strncat(g_blend, a_str + i, 1);
    }
    if (!g_nocolor) strcat(g_blend, CP_GREEN_COLOR_DEFAULT);
    return g_blend;
}

char *color_256(unsigned int a_color)
{
    g_output[0] = 0;
    sprintf(g_output, "\033[38;5;%dm", a_color);
    return g_output;
}

char *color_256_bg(unsigned int a_color)
{
    g_output[0] = 0;
    sprintf(g_output, "\033[48;5;%dm", a_color);
    return g_output;
}

void color_printf(const char *format, ...)
{
    char original_format[BUFFLEN * 8];
    char edited_format[BUFFLEN * 8];
    char c;
    size_t i = 0, j = 0;
    enum { COLLECT, FINDSTAR, PAREN_OPEN, PAREN_VALUE, PAREN_OPTION } state = COLLECT;

    // cache format string
    original_format[0] = 0;
    va_list args;
    va_start(args);
    vsprintf(original_format, format, args);
    va_end(args);

    while (i < strlen(original_format)) {
        c = original_format[i];
        if (state == COLLECT) {
            if (c == '*') {
                // found first star
                state = FINDSTAR;
            } else {
                edited_format[j++] = c;
            }
            ++i;
            continue;
        } else if (state == PAREN_OPEN) {
            if (c != '[') {
                edited_format[j] = 0;
                strcat(edited_format, "???");
                j += 3;
                state = COLLECT;
                ++i;
                continue;
            } else {
                paren_count = 0;
                paren_assembly_len = 0;
                paren_assembly[0] = 0;
                state = PAREN_VALUE;
                ++i;
                continue;
            }
        } else if (state == PAREN_VALUE) {
            if ((c == ',') || (c == ']')) {
                paren_opts[paren_count] = atoi(paren_assembly);
                paren_count++;
                paren_assembly_len = 0;
                paren_assembly[0] = 0;
                if (c == ']') {
                    state = PAREN_OPTION;
                } else {
                    state = PAREN_VALUE;
                }
                ++i;
                continue;
            } else {
                // some other character, so assemble an integer
                paren_assembly[paren_assembly_len] = c;
                paren_assembly[paren_assembly_len + 1] = 0;
                paren_assembly_len++;
                ++i;
                continue;
            }
        } else if (state == PAREN_OPTION) {
            if (paren_char == 'c') {
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, color_gs(paren_opts[0]));
                    j += strlen(g_output);
                }
                state = COLLECT;
                continue;
            } else if (paren_char == 'g') {
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, color_gs_bg(paren_opts[0]));
                    j += strlen(g_output);
                }
                state = COLLECT;
                continue;
            } else if (paren_char == '2') {
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, color_256(paren_opts[0]));
                    j += strlen(g_output);
                }
                state = COLLECT;
                continue;
            } else if (paren_char == '3') {
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, color_256_bg(paren_opts[0]));
                    j += strlen(g_output);
                }
                state = COLLECT;
                continue;
            } else if (paren_char == '5') {
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, color_rgb(paren_opts[0], paren_opts[1], paren_opts[2]));
                    j += strlen(g_output);
                }
                state = COLLECT;
                continue;
            } else if (paren_char == '6') {
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, color_rgb_bg(paren_opts[0], paren_opts[1], paren_opts[2]));
                    j += strlen(g_output);
                }
                state = COLLECT;
                continue;
            } else {
                edited_format[j] = 0;
                strcat(edited_format, "???");
                j += 3;
                state = COLLECT;
                continue;
            }
        } else if (state == FINDSTAR) {
            if (c == '*') {
                // double star
                edited_format[j++] = '*';
            } else if (c == 'c') {
                state = PAREN_OPEN;
                paren_char = 'c';
                ++i;
                continue;
            } else if (c == 'g') {
                state = PAREN_OPEN;
                paren_char = 'g';
                ++i;
                continue;
            } else if (c == '2') {
                state = PAREN_OPEN;
                paren_char = '2';
                ++i;
                continue;
            } else if (c == '3') {
                state = PAREN_OPEN;
                paren_char = '3';
                ++i;
                continue;
            } else if (c == '5') {
                state = PAREN_OPEN;
                paren_char = '5';
                ++i;
                continue;
            } else if (c == '6') {
                state = PAREN_OPEN;
                paren_char = '6';
                ++i;
                continue;
            } else if (c == 'h') {
                // output highlight
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, g_ansi_highlight);
                    j += strlen(g_ansi_highlight);
                }
            } else if (c == 'a') {
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, g_ansi_heading);
                    j += strlen(g_ansi_heading);
                }
            } else if (c == 'b') {
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, g_ansi_bullet);
                    j += strlen(g_ansi_bullet);
                }
            } else if (c == 'e') {
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, g_ansi_error);
                    j += strlen(g_ansi_error);
                }
            } else if (c == 'd') {
                if (!g_nocolor) {
                    edited_format[j] = 0;
                    strcat(edited_format, g_ansi_default);
                    j += strlen(g_ansi_default);
                }
            } else {
                // unknown escape sequence
                edited_format[j] = 0;
                strcat(edited_format, "*?");
                j += 2;
            }
            state = COLLECT;
            ++i;
            continue;
        }
    }
    edited_format[j] = 0;
    printf(edited_format);
}

void color_err_printf(int a_strerror, const char *format, ...)
{
    // call this without a linefeed at the end
    char edited_format[BUFFLEN];
    edited_format[0] = 0;
    if (!g_nocolor) strcat(edited_format, g_ansi_error);
    strcat(edited_format, format);
    if (a_strerror) {
        sprintf(edited_format + strlen(edited_format), " : %s\n", strerror(errno));
    } else {
        sprintf(edited_format + strlen(edited_format), "\n");
    }
    if (!g_nocolor) strcat(edited_format, g_ansi_default);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, edited_format, args);
    va_end(args);
}

void color_debug(const char *format, ...)
{
    if (g_debug == 0)
        return; // don't print anything if debug isn't turned on
    pthread_mutex_lock(&g_debug_mtx);
    char edited_format[BUFFLEN];
    edited_format[0] = 0;
    if (!g_nocolor)
        strcat(edited_format, g_ansi_debug);
    strcat(edited_format, format);
    if (!g_nocolor)
        strcat(edited_format, g_ansi_default);
    va_list args;
    va_start(args, format);
    vprintf(edited_format, args);
    va_end(args);
    pthread_mutex_unlock(&g_debug_mtx);
}
