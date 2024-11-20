#ifndef DEBUG_H
#define DEBUG_H

#include <stddef.h>  // For size_t
#include <stdio.h>   // For printf and snprintf

// Function declarations
void DebugPrintf(const char* message, size_t line, const char* file);
void* DebugMalloc(size_t size, size_t line, const char* file);
void* DebugRealloc(void* ptr, size_t size, size_t line, char* file);
void DebugFree(void* ptr, size_t line, const char* file);
void DebugStart(size_t line, const char* file);
void DebugEnd();
size_t DebugGetSizeBytes(void* ptr);
void DebugPrintMemory();

#ifdef DEBUG

#define debug(code)        DebugStart(__LINE__, __FILE__); code; DebugEnd();
#define printf(fmt, ...)   do {char buffer[512]; snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); DebugPrintf(buffer, __LINE__, __FILE__); } while (0)

#else

#define debug(code)        code

#endif

#endif // DEBUG_H
