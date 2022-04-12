/*
	Copyright 2012-2014 Benjamin Vedder	benjamin@vedder.se

	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
   buffer.c

    Created on: 13 maj 2013
        Author: benjamin
*/

#include "buffer.h"

int16_t buffer_get_int16(const uint8_t *buffer, int32_t *index) {
  int16_t res =	((uint16_t) buffer[*index]) << 8 |
                ((uint16_t) buffer[*index + 1]);
  *index += 2;
  return res;
}

int32_t buffer_get_int32(const uint8_t *buffer, int32_t *index) {
  int32_t res =	((uint32_t) buffer[*index]) << 24 |
                ((uint32_t) buffer[*index + 1]) << 16 |
                ((uint32_t) buffer[*index + 2]) << 8 |
                ((uint32_t) buffer[*index + 3]);
  *index += 4;
  return res;
}

float buffer_get_float16(const uint8_t *buffer, float scale, int32_t *index) {
  return (float)buffer_get_int16(buffer, index) / scale;
}

float buffer_get_float32(const uint8_t *buffer, float scale, int32_t *index) {
  return (float)buffer_get_int32(buffer, index) / scale;
}
