#ifndef ACPI_H
#define ACPI_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

struct XSDP_t {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;      // deprecated since version 2.0

 uint32_t Length;
 uint64_t XsdtAddress;
 uint8_t ExtendedChecksum;
 uint8_t reserved[3];
} __attribute__ ((packed));



struct ACPISDTHeader {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
} __attribute__ ((packed));


struct XSDT {
  struct ACPISDTHeader h;
  uint64_t PointerToOtherSDT[];
} __attribute__ ((packed));





void validate_rsdp(void* rsdp_pointer);
void* find_MADT(void* rsdp_pointer, uint64_t hhdm_offset);

#endif