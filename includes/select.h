// PixelNutApp Pattern Selection Handling
// All pattern strings are stored in the code (eeprom flash).
//
// Sets global variables: 'cmdStr', 'curPattern', 'codePatterns'.
// Uses global variables: 'pPixelNutEngine', 'customPatterns[]'.
// Calls global routines: 'FlashGetStr', 'ErrorHandler'.
//========================================================================================
/*
Copyright (c) 2015-2017, Greg de Valois
Software License Agreement (BSD License)
See license.txt for the terms of this license.
*/

#if EXTERN_PATTERNS
// patterns are sent from external client and stored in flash
// the pattern number is meaningful only to the client

void GetCurPattern(char *instr)
{
  FlashGetStr(instr);
  DBGOUT((F("Retrieved pattern #%d"), curPattern));
}

#elif CUSTOM_PATTERNS

void GetCurPattern(char *instr)
{
  if ((curPattern > 0) && (curPattern <= codePatterns))
  {
    strcpy_P(instr, customPatterns[curPattern-1]);
    DBGOUT((F("Retrieved pattern #%d"), curPattern));
    pPixelNutEngine->popPluginStack(); // clear stack to prepare for new cmds
  }
}

#endif

#if CUSTOM_PATTERNS

// never returns if fails to find any stored pattern strings
void CheckForPatterns(void)
{
  DBGOUT((F("Read all stored patterns:")));

  codePatterns = 0;
  for (int i = 0; ; ++i)
  {
    if (customPatterns[i] == NULL)
    {
      codePatterns = i;
      break;
    }

    strcpy_P(cmdStr, customPatterns[i]);
    DBGOUT((F("  %2d: \"%s\""), i+1, cmdStr));
  }

  // cannot continue if cannot find any valid pattern strings
  if (!codePatterns) ErrorHandler(1, 1, true);
}

void GetNextPattern(void)
{
  // curPattern must be 1...codePatterns
  if (++curPattern > codePatterns)
    curPattern = 1;

  GetCurPattern(cmdStr);
}

void GetPrevPattern(void)
{
  // curPattern must be 1...codePatterns
  if (curPattern <= 1) curPattern = codePatterns;
  else --curPattern;

  GetCurPattern(cmdStr);
}

#else

void CheckForPatterns(void) {}

#endif // CUSTOM_PATTERNS

//========================================================================================
