/*
  ifshare.h: Common ifshare API
  Copyright (C) 2025 Gonzalo José Carracedo Carballal
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, version 3.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#ifndef _IFSHARE_H
#define _IFSHARE_H

#include <defs.h>
#include <util.h>
#include <log.h>
#include <stdlib.h>
#include <stdint.h>

struct ifshare_pdu {
  uint32_t is_magic;
  uint32_t is_size;
  uint8_t  is_data[0];
};

#define IFSHARE_SERVER_PORT 5665
#define IFSHARE_MAX_MTU     4096
#define IFSHARE_MAGIC       0x1f5543aa

#endif /* _IFSHARE_H */
