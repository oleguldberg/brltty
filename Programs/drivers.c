/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "system.h"
#include "drivers.h"

int
isDriverAvailable (const char *code, const char *codes) {
  int length = strlen(code);
  const char *string;

  while ((string = strstr(codes, code))) {
    if (((string == codes) || (string[-1] == ' ')) &&
        (!string[length] || (string[length] == ' '))) {
      return 1;
    }

    string += length;
  }

  return 0;
}

int
isDriverIncluded (const char *code, const DriverEntry *table) {
  while (table->address) {
    if (strcmp(code, table->definition->code) == 0) return 1;
    ++table;
  }
  return 0;
}

int
haveDriver (const char *code, const char *codes, const DriverEntry *table) {
  return (table && table->address)? isDriverIncluded(code, table):
                                    isDriverAvailable(code, codes);
}

const char *
getDefaultDriver (const DriverEntry *table) {
  return (table && table[0].address && !table[1].address)? table[0].definition->code: NULL;
}

const void *
loadDriver (
  const char *driverCode, void **driverObject,
  const char *driverDirectory, const DriverEntry *driverTable,
  const char *driverType, char driverCharacter, const char *driverSymbol,
  const void *nullAddress, const DriverDefinition *nullDefinition
) {
  const void *driverAddress = NULL;
  *driverObject = NULL;

  if (!driverCode || !*driverCode) {
    if (driverTable)
      if (driverTable->address)
        return driverTable->address;
    return nullAddress;
  }

  if (strcmp(driverCode, nullDefinition->code) == 0) return nullAddress;

  if (driverTable) {
    const DriverEntry *driverEntry = driverTable;
    while (driverEntry->address) {
      if (strcmp(driverCode, driverEntry->definition->code) == 0) {
        return driverEntry->address;
      }
      ++driverEntry;
    }
  }

  {
    char *libraryPath;
    const int libraryNameLength = strlen(MODULE_NAME) + strlen(driverCode) + strlen(MODULE_EXTENSION) + 3;
    char libraryName[libraryNameLength];
    snprintf(libraryName, libraryNameLength, "%s%c%s.%s",
             MODULE_NAME, driverCharacter, driverCode, MODULE_EXTENSION);

    if ((libraryPath = makePath(driverDirectory, libraryName))) {
      void *libraryHandle = loadSharedObject(libraryPath);

      if (libraryHandle) {
        const int symbolNameLength = strlen(driverSymbol) + strlen(driverCode) + 2;
        char symbolName[symbolNameLength];
        snprintf(symbolName, symbolNameLength, "%s_%s",
                 driverSymbol, driverCode);

        if (findSharedSymbol(libraryHandle, symbolName, &driverAddress)) {
          *driverObject = libraryHandle;
        } else {
          LogPrint(LOG_ERR, "Cannot find %s driver symbol: %s", driverType, symbolName);
          unloadSharedObject(libraryHandle);
          driverAddress = NULL;
        }
      } else {
        LogPrint(LOG_ERR, "Cannot load %s driver: %s", driverType, libraryPath);
      }

      free(libraryPath);
    }
  }

  return driverAddress;
}

void
identifyDriver (
  const char *type,
  const DriverDefinition *definition,
  int full
) {
  if (definition->version && *definition->version)
    LogPrint(LOG_NOTICE, "%s %s Driver: version %s, compiled on %s at %s", definition->name, type, definition->version, definition->date, definition->time);
  else
    LogPrint(LOG_NOTICE, "%s %s Driver: compiled on %s at %s", definition->name, type, definition->date, definition->time);

  if (full) {
    if (definition->copyright && *definition->copyright) LogPrint(LOG_INFO, "   %s", definition->copyright);
  }
}
