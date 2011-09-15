/* dumb-frotz.h
 * $Id: dumb-frotz.h,v 1.1.1.1 2002/03/26 22:38:34 feedle Exp $
 * Frotz os functions for a standard C library and a dumb terminal.
 * Now you can finally play Zork Zero on your Teletype.
 *
 * Copyright 1997, 1998 Alembic Petrofsky <alembic@petrofsky.berkeley.ca.us>.
 * Any use permitted provided this notice stays intact.
 */
#include "frotz.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* from ../common/setup.h */
extern f_setup_t f_setup;
extern void spectrum_get_line(char *s);

/* From input.c.  */
int is_terminator (zchar);

/* dumb-input.c */
int dumb_handle_setting(const char *setting, int show_cursor, int startup);
void dumb_init_input(void);

/* dumb-output.c */
void dumb_init_output(void);
int dumb_output_handle_setting(const char *setting, int show_cursor,
				int startup);
void dumb_show_screen(int show_cursor);
void dumb_show_prompt(int show_cursor, char line_type);
void dumb_dump_screen(void);
void dumb_display_user_input(char *);
void dumb_discard_old_input(int num_chars);
void dumb_elide_more_prompt(void);
void dumb_set_picture_cell(int row, int col, char c);

/* dumb-pic.c */
void dumb_init_pictures(char *graphics_filename);
