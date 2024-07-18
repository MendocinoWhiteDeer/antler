#pragma once
#include <stdint.h>

typedef enum
{
  LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
  
} AtlrLoggerType;

typedef uint8_t  AtlrU8;
typedef uint16_t AtlrU16;
typedef uint32_t AtlrU32;
typedef uint64_t AtlrU64;
typedef int8_t   AtlrI8;
typedef int16_t  AtlrI16;
typedef int32_t  AtlrI32;
typedef int64_t  AtlrI64;
typedef float    AtlrF32;
typedef double   AtlrF64;

void AtlrLogMsg(AtlrLoggerType t, const char* format, ...);
