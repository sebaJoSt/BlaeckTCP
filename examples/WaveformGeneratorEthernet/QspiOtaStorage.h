/*
  Where a sketch sent over the network is kept until the board restarts into it.

  The Giga has no flash to spare for an update the way an AVR or an R4 has: the sketch area
  is one piece, and a second copy of a sketch does not fit beside the one running. What it
  does have is 16 MB of QSPI flash beside the processor, and a bootloader that looks there
  for a file called UPDATE.BIN. So the upload is written to that file, and the bootloader
  copies it into place on the next start.

  Requires the QSPI to have been partitioned once, with the QSPIFormat sketch in the core's
  STM32H747_System examples. Partition 2 is the one the bootloader reads.

  Arduino_Portenta_OTA's own begin() is not used: it also opens the WiFi certificates on
  partition 1, which are for downloading an update over HTTPS. Nothing is downloaded here -
  the sketch arrives over the wire - so the partition is mounted directly and the library is
  asked only for the last step, which is to record where the update is and reboot into it.
*/
#ifndef QSPI_OTA_STORAGE_H
#define QSPI_OTA_STORAGE_H

#include <Arduino_Portenta_OTA.h>
#include <BlockDevice.h>
#include <MBRBlockDevice.h>
#include <FATFileSystem.h>

#include "OTAStorage.h"

class QspiOtaStorageClass : public ExternalOTAStorage {

public:

  virtual int open(int length) {
    (void)length;

    if (!Arduino_Portenta_OTA::isOtaCapable()) {
      Serial.println("This board's bootloader cannot apply an update from QSPI.");
      return -1;
    }

    // Mounted once and left mounted: the bootloader is told about the file after it is
    // written, and unmounting in between would only be a chance to fail.
    if (!_mounted) {
      int err = _fs.mount(&_partition);
      if (err) {
        Serial.print("The OTA partition would not mount, error ");
        Serial.println(err);
        Serial.println("Run QSPIFormat once, from the core's STM32H747_System examples.");
        return -2;
      }
      _mounted = true;
    }

    _file = fopen("/fs/UPDATE.BIN", "wb");
    if (!_file) {
      Serial.println("UPDATE.BIN could not be opened for writing.");
      return -3;
    }

    return 1;
  }

  virtual size_t write(uint8_t c) {
    if (fwrite(&c, 1, 1, _file) != 1) {
      Serial.println("Writing the update to QSPI failed.");
      fclose(_file);
      _file = nullptr;
      return 0;
    }
    return 1;
  }

  virtual void close() {
    if (_file) {
      fclose(_file);
      _file = nullptr;
    }
  }

  // What is left behind when an upload does not finish. A half-written UPDATE.BIN would be
  // applied on the next start as though it were whole.
  virtual void clear() {
    remove("/fs/UPDATE.BIN");
  }

  virtual void apply() {
    // Records in the backup registers where the update is and how long it is, then restarts.
    // The bootloader reads those before anything else runs.
    Arduino_Portenta_OTA_QSPI ota(QSPI_FLASH_FATFS_MBR, 2);

    if (ota.update() != Arduino_Portenta_OTA::Error::None) {
      Serial.println("The update was written but the board would not be told about it.");
      return;
    }

    ota.reset();
  }

  // The partition, less a little room: an update is a file on a filesystem rather than a
  // region, so what fits is what the filesystem leaves.
  virtual long maxSize() {
    return 5 * 1024 * 1024 - 64 * 1024;
  }

private:

  mbed::MBRBlockDevice _partition{mbed::BlockDevice::get_default_instance(), 2};
  mbed::FATFileSystem _fs{"fs"};
  bool _mounted = false;
  FILE* _file = nullptr;
};

#endif
