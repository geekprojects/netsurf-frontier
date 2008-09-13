/*
 * Copyright 2008 Chris Young <chris@unsatisfactorysoftware.co.uk>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AMIGA_OPTIONS_H
#define AMIGA_OPTIONS_H
#include "desktop/options.h"

extern bool option_verbose_log;
extern char *option_url_file;
extern char *option_hotlist_file;
extern bool option_use_wb;
extern int option_modeid;
extern char *option_toolbar_images;
extern bool option_no_iframes;
extern bool option_utf8_clipboard;
extern int option_throbber_frames;

#define EXTRA_OPTION_DEFINE \
bool option_verbose_log = false; \
char *option_url_file = 0; \
char *option_hotlist_file = 0; \
bool option_use_wb = false; \
int option_modeid = 0; \
char *option_toolbar_images = 0; \
bool option_no_iframes = false; \
bool option_utf8_clipboard = false; \
int option_throbber_frames = 1;

#define EXTRA_OPTION_TABLE \
{ "verbose_log",	OPTION_BOOL,	&option_verbose_log}, \
{ "url_file",		OPTION_STRING,	&option_url_file }, \
{ "hotlist_file",		OPTION_STRING,	&option_hotlist_file }, \
{ "use_workbench",	OPTION_BOOL,	&option_use_wb}, \
{ "screen_modeid",	OPTION_INTEGER,	&option_modeid}, \
{ "toolbar_images",		OPTION_STRING,	&option_toolbar_images }, \
{ "no_iframes",	OPTION_BOOL,	&option_no_iframes}, \
{ "clipboard_write_utf8",	OPTION_BOOL,	&option_utf8_clipboard}, \
{ "throbber_frames",	OPTION_INTEGER,	&option_throbber_frames},
#endif
