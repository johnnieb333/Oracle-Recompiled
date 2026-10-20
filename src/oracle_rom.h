#ifndef ORACLE_EXTERNAL_ROM_H
#define ORACLE_EXTERNAL_ROM_H
#include <stddef.h>
#include <stdint.h>
void oracle_sha1(const uint8_t* bytes,size_t size,char hex[41]);
int oracle_read_rom(const char* path,const char* expected,uint8_t** data,size_t* size,char* error,size_t error_size);
#endif
