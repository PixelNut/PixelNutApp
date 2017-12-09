// PixelNutApp Flash Storage Handling
// Stores the pattern number, effect properties, and pattern strings in (eeprom flash).
//
// Uses global variables: 'pPixelNutEngine'.
//========================================================================================
/*
Copyright (c) 2015-2017, Greg de Valois
Software License Agreement (BSD License)
See license.txt for the terms of this license.
*/

#if !defined(EEPROM_BYTES)
#if defined(__arm__) && defined(__MKL26Z64__)         // Teensy LC
#define EEPROM_BYTES            128
#elif defined(__arm__) && defined(__MK20DX256__)      // Teensy 3.x
#define EEPROM_BYTES            2048
#elif defined(__AVR__)                                // all other AVR processors
#define EEPROM_BYTES            1024
#else                                                 // disable for unknown processor
#define EEPROM_BYTES            0
#endif
#endif // !defined(EEPROM_BYTES)

#if (EEPROM_BYTES <= 0)
#define EEPROM_FREE_BYTES 0
void FlashSetStr(char *str, int offset) {}
void FlashGetStr(char *str) { *str = 0; }
void FlashSetBright() {}
void FlashSetDelay()  {}
byte FlashGetPattern(void) { return 1; }
void FlashSetPattern(byte pattern)  {}
void FlashSetXmode(bool enable) {}
void FlashSetExterns(uint16_t hue, byte wht, byte cnt) {}
void FlashSetForce(short force) {}
short FlashGetForce(void) { return 0; }
void FlashSetProperties(void) {}

#else

#include <EEPROM.h>

// for each segment:
#define FLASH_SEG_LENGTH        10

// offsets within each segment space:
#define FLASH_SEG_BRIGHTNESS    0
#define FLASH_SEG_DELAYMSECS    1
#define FLASH_SEG_PATTERN       2
#define FLASH_SEG_XT_MODE       3
#define FLASH_SEG_XT_HUE        4 // 2 bytes
#define FLASH_SEG_XT_WHT        6
#define FLASH_SEG_XT_CNT        7
#define FLASH_SEG_FORCE         8 // 2 bytes

#define FLASHOFF_PATTERN_START  (SEGMENT_COUNT * FLASH_SEG_LENGTH)
#define FLASHOFF_PATTERN_END    (FLASHOFF_PATTERN_START + (STRLEN_PATTERNS * STRAND_COUNT))

#if (FLASHOFF_PATTERN_END > EEPROM_BYTES)
#error("Not enough flash space to store external pattern string(s)");
#endif

#define EEPROM_FREE_START  FLASHOFF_PATTERN_END
#define EEPROM_FREE_BYTES  (EEPROM_BYTES - EEPROM_FREE_START)

static byte valOffset = 0;
static uint16_t strOffset = FLASHOFF_PATTERN_START;

static void SetFlashValue(uint16_t offset, byte value) { EEPROM.write(valOffset + offset, value); }
static byte GetFlashValue(uint16_t offset) { return EEPROM.read(valOffset + offset); }

void FlashSetSegment(byte seg)
{
  valOffset = ((seg-1) * FLASH_SEG_LENGTH);
  strOffset = FLASHOFF_PATTERN_START + ((seg-1) * STRLEN_PATTERNS);
}

void FlashSetStr(char *str, int offset)
{
  DBGOUT((F("FlashSetStr(@%d): \"%s\""), (strOffset + offset), str));
  for (int i = 0; ; ++i)
  {
    if ((strOffset + offset + i) >= EEPROM_BYTES) break; // prevent overrun
    EEPROM.write((strOffset + offset + i), str[i]);
    if (!str[i]) break;
  }
}

void FlashGetStr(char *str)
{
  for (int i = 0; ; ++i)
  {
    if ((i >= (STRLEN_PATTERNS-1)) ||
        ((strOffset + i) >= EEPROM_BYTES))
         str[i] = 0; // prevent overrun
    else str[i] = EEPROM.read(strOffset + i);
    if (!str[i]) break;
  }
  DBGOUT((F("FlashGetStr(@%d): \"%s\""), strOffset, str));
}

void FlashSetBright() { SetFlashValue(FLASH_SEG_BRIGHTNESS, pPixelNutEngine->getMaxBrightness());   }
void FlashSetDelay()  { SetFlashValue(FLASH_SEG_DELAYMSECS, pPixelNutEngine->getDelayOffset());     }

byte FlashGetPattern(void)          { return GetFlashValue(FLASH_SEG_PATTERN);        }
void FlashSetPattern(byte pattern)  { SetFlashValue(FLASH_SEG_PATTERN, pattern);      }
void FlashSetXmode(bool enable)     { SetFlashValue(FLASH_SEG_XT_MODE, enable);       }

void FlashSetExterns(uint16_t hue, byte wht, byte cnt)
{
  SetFlashValue(FLASH_SEG_XT_HUE,   hue&0xFF);
  SetFlashValue(FLASH_SEG_XT_HUE+1, hue>>8);
  SetFlashValue(FLASH_SEG_XT_WHT,   wht);
  SetFlashValue(FLASH_SEG_XT_CNT,   cnt);
}

void FlashSetForce(short force)
{
  SetFlashValue(FLASH_SEG_FORCE, force);
  SetFlashValue(FLASH_SEG_FORCE+1, (force >> 8));
}

short FlashGetForce(void)
{
  return GetFlashValue(FLASH_SEG_FORCE) + (GetFlashValue(FLASH_SEG_FORCE+1) << 8);
}

void FlashSetProperties(void)
{
  uint16_t hue = GetFlashValue(FLASH_SEG_XT_HUE) + (GetFlashValue(FLASH_SEG_XT_HUE+1) << 8);

  pPixelNutEngine->setPropertyMode(      GetFlashValue(FLASH_SEG_XT_MODE));
  pPixelNutEngine->setColorProperty(hue, GetFlashValue(FLASH_SEG_XT_WHT));
  pPixelNutEngine->setCountProperty(     GetFlashValue(FLASH_SEG_XT_CNT));
}

void FlashStartup(void)
{
  // retrieve saved control values from flash
  byte bright = GetFlashValue(FLASH_SEG_BRIGHTNESS);
  if (bright == 0) SetFlashValue(FLASH_SEG_BRIGHTNESS, bright=60); // set to 60% if cleared

  pPixelNutEngine->setMaxBrightness(bright);
  pPixelNutEngine->setDelayOffset((int8_t)GetFlashValue(FLASH_SEG_DELAYMSECS));
  // make sure 8 bit value is taken as a signed integer

  DBGOUT((F("Flash: max brightness=%d%%"), bright));
  DBGOUT((F("Flash: delay offset=%d msecs"), (int8_t)GetFlashValue(FLASH_SEG_DELAYMSECS)));

  FlashSetProperties();
}

void FlashFormat(void)
{
  #if EEPROM_FORMAT
  for (int i = 0; i < EEPROM_BYTES; ++i) SetFlashValue(i, 0);
  DBGOUT((F("Cleared %d bytes of EEPROM"), EEPROM_BYTES));
  ErrorHandler(1, 0, true);
  #endif
}

#endif // (EEPROM_BYTES > 0)

//========================================================================================
