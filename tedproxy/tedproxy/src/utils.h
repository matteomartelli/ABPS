/*****************************************************************************/
/* Transmission Error Detector test application                              */
/* This program is a part of the ABPS software suite.                        */
/*                                                                           */
/* Copyright (C) 2015                                                        */
/* Gabriele Di Bernardo, Alessandro Mengoli, Matteo Martelli                 */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU General Public License               */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or any later version.                                     */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,*/
/* USA.                                                                      */
/*****************************************************************************/

#ifndef UTILS_H
#define UTILS_H

#include <inttypes.h>
#include "structs.h"

extern struct conf_s conf;
extern void (*print_dbg)(const char * fmt, ...);

void print_err(const char *fmt, ...);
void exit_err(const char *fmt, ...);
void utils_default_conf(void);
int utils_get_opt(int argc, char **argv);
void utils_gen_random(char *s, const int len);
int utils_get_rtpid(char *buf, ssize_t len, uint16_t *rtp_id);
uint64_t utils_get_timestamp(void);


#endif
