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
 * @file color_print.h
 * @brief C Terminal Color Printing API
 *
 * Extends printf and fprintf to enable color ANSI printing to the terminal.
 *
 */

#ifndef COLOR_PRINT_H
#define COLOR_PRINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <pthread.h>

#define BUFFLEN 1024
#define ANSIBUFFLEN 20

/* GREEN theme */
#define CP_GREEN_COLOR_HEADING   "\033[32m"          ///< Heading color
#define CP_GREEN_COLOR_ERROR     "\033[91m"          ///< For error messages
#define CP_GREEN_COLOR_HIGHLIGHT "\033[92m"          ///< Extra bright for highlights
#define CP_GREEN_COLOR_BULLET    "\033[93m"          ///< Outstanding messages
#define CP_GREEN_COLOR_DEFAULT   "\033[39m\033[49m"  ///< Return to default terminal color
#define CP_GREEN_COLOR_DEBUG     "\033[33m"          ///< Debug messages

/* BLUE theme */
#define CP_BLUE_COLOR_HEADING   "\033[94m"           ///< Heading color
#define CP_BLUE_COLOR_ERROR     "\033[91m"           ///< For error messages
#define CP_BLUE_COLOR_HIGHLIGHT "\033[96m"           ///< Extra bright for highlights
#define CP_BLUE_COLOR_BULLET    "\033[95m"           ///< Outstanding messages
#define CP_BLUE_COLOR_DEFAULT   "\033[39m\033[49m"   ///< Return to default terminal color
#define CP_BLUE_COLOR_DEBUG     "\033[33m"           ///< Debug messages

/* RED theme */
#define CP_RED_COLOR_HEADING   "\033[31m"            ///< Heading color
#define CP_RED_COLOR_ERROR     "\033[96m"            ///< For error messages
#define CP_RED_COLOR_HIGHLIGHT "\033[91m"            ///< Extra bright for highlights
#define CP_RED_COLOR_BULLET    "\033[93m"            ///< Outstanding messages
#define CP_RED_COLOR_DEFAULT   "\033[39m\033[49m"    ///< Return to default terminal color
#define CP_RED_COLOR_DEBUG     "\033[33m"            ///< Debug messages

/* PURPLE theme */
#define CP_PURPLE_COLOR_HEADING   "\033[35m"         ///< Heading color
#define CP_PURPLE_COLOR_ERROR     "\033[93m"         ///< For error messages
#define CP_PURPLE_COLOR_HIGHLIGHT "\033[95m"         ///< Extra bright for highlights
#define CP_PURPLE_COLOR_BULLET    "\033[96m"         ///< Outstanding messages
#define CP_PURPLE_COLOR_DEFAULT   "\033[39m\033[49m" ///< Return to default terminal color
#define CP_PURPLE_COLOR_DEBUG     "\033[33m"         ///< Debug messages

typedef enum {
    THEME_GREEN,
    THEME_BLUE,
    THEME_RED,
    THEME_PURPLE
} cp_theme_t;

void color_init         (const int a_nocolor, const int a_debug);
void color_set_theme    (cp_theme_t a_theme);
void color_set_nocolor  (const int a_nocolor);
void color_set_debug    (const int a_debug);
void color_free         ();
void color_progress     (uint32_t a_sofar, uint32_t a_total);
void color_printf       (const char *format, ...);
void color_err_printf   (int a_strerror, const char *format, ...);
void color_debug        (const char *format, ...);
char *color_256         (unsigned int a_color);
char *color_256_bg      (unsigned int a_color);
char *color_rgb         (uint8_t a_red, uint8_t a_green, uint8_t a_blue);
char *color_rgb_bg      (uint8_t a_red, uint8_t a_green, uint8_t a_blue);
char *color_rgb_blend   (const char *a_str, uint8_t a_begin_red, uint8_t a_begin_green, uint8_t a_begin_blue, uint8_t a_end_red, uint8_t a_end_green, uint8_t a_end_blue, int a_background);
char *color_gs          (int a_color);
char *color_gs_bg       (int a_color);

#ifdef __cplusplus
}
#endif

#endif // COLOR_PRINT_H
