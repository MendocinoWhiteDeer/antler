/*

This file is part of antler.
Copyright (C) 2024 Taylor Wampler 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
  
*/

#include "antler.h"
#include <stdio.h>
#include <stdarg.h>

#define LOGGER_CHARACTER_COUNT 2048

#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"

static char loggerBuffer[LOGGER_CHARACTER_COUNT];
static const char* loggerNames[] =
{
  "TRACE", "DEBUG", "INFO",
  "WARN",  "ERROR", "FATAL"
};
static const char* loggerColors[] =
{
  ANSI_COLOR_BLUE,   ANSI_COLOR_CYAN,    ANSI_COLOR_GREEN,
  ANSI_COLOR_YELLOW, ANSI_COLOR_MAGENTA, ANSI_COLOR_RED
};

void atlrLog(AtlrLoggerType t, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vsnprintf(loggerBuffer, LOGGER_CHARACTER_COUNT, format, args);
  va_end(args);
  printf("%s[%s]: %s%s\n", loggerColors[t], loggerNames[t], loggerBuffer, ANSI_COLOR_RESET);
}
