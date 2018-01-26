// PixelNutApp Bluetooth Communications
//
// Uses global variables: 'pixelNutSupport', 'cmdStr', 'pAppCmd'.
// Calls global routines: 'CheckExecCmd', 'ErrorHandler'.
//========================================================================================
/*
Copyright (c) 2015-2017, Greg de Valois
Software License Agreement (BSD License)
See license.txt for the terms of this license.
*/

#if BLUETOOTH_COMM

BluefruitStrs bfruit(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

extern void CheckExecCmd(char *instr); // defined in main.h
extern AppCommands *pAppCmd;           // pointer to current instance

#define MSECS_CHECK_CONNECT   500     // check for connection every 1/2 second

class Bluetooth : public CustomCode
{
public:

  void setup(void);
  bool control(void);

  bool setName(char *name);
  bool sendReply(char *instr);

  bool inSetup = false;
  bool isConnected = false;

private:

  unsigned long msecsConnect = 0;
};
Bluetooth bluetooth;

// this will be called upon any bluetooth communication errors
// upon return the hardware will be reset automatically for us
void notifyCB(NotifyMessage msgval, char *msgstr)
{
  DBGOUT((F("Bluetooth Notification %d: %s"), msgval, msgstr));

  DBGOUT((F("Resetting BLE controller...")));
  bfruit.reset(); // hardware reset, this clears cmdStr[] too
  bluetooth.isConnected = false;
  DBGOUT((F("Reset completed")));

  ErrorHandler(3, msgval, bluetooth.inSetup); // hangs here if in setup
}

// return false if failed to set name
bool setNameBLE(char *name)
{
  // 14 chars for command + terminator
  // + beginning "P!" to name
  char str[MAXLEN_DEVICE_NAME+2+14+1];
  strcpy(str, (char*)"AT+GAPDEVNAME=");
  strcpy((str+14), "P!");
  strcpy((str+16), name);

  DBGOUT((F("Setting BLE name: %s"), str));
  if (!bfruit.sendCmdStr(str, NULL))
  {
    DBGOUT((F("Setting name failed")));
    return false;
  }
  return true;
}

// response is in cmdStr
void getNameCB(void)
{
  if (!strcmp(cmdStr, "OK")) return; // ignore this

  DBGOUT((F("BLE DevName: \"%s\""), cmdStr));

  if (pAppCmd->setDeviceName(cmdStr))
    setNameBLE(cmdStr);
}

// does not return if fail because NotifyCB gets
// called which will then hang in ErrorHandler
void Bluetooth::setup(void)
{
  inSetup = true;
  isConnected = false;

  DBGOUT((F("Setting up bluetooth...")));
  bfruit.init(cmdStr, STRLEN_PATTERNS, notifyCB);

  // this seems to insure a clean start, otherwise
  // it sometimes gets stuck in connected mode:
  bfruit.sendCmdStr((char*)"ATZ", NULL);
  delay(1000); // Bluefruit takes 1 second to reboot

  if (!bfruit.sendCmdStr((char*)"AT+GAPDEVNAME", getNameCB))
  {
    DBGOUT((F("BLE get name failed")));
  }

  inSetup = false;
}

// return false if failed to set name
bool Bluetooth::setName(char *name)
{
  FlashSetName(name);
  return setNameBLE(name);
}

// return false if failed to send message
// upon exit cmdStr[] has garbage from calling writeDataStr()
bool Bluetooth::sendReply(char *instr)
{
  DBGOUT((F("BLE <-- \"%s\""), instr));

  bool success = true;

  // client expects all strings terminated with newline
  if (!bfruit.writeDataStr(instr) ||
      !bfruit.writeDataStr((char*)"\n"))
  {
    DBGOUT((F("BLE write data failed")));
    success = false;
  }

  delay(100); // hack to prevent data overflow with bluefruit
  msecsConnect = pixelNutSupport.getMsecs();

  return success;
}

static void ResponseCB(void)
{
  DBGOUT((F("BLE --> \"%s\""), cmdStr));
  if (pAppCmd->execCmd(cmdStr)) CheckExecCmd(cmdStr);
}

// return true if handling commands externally
bool Bluetooth::control(void)
{
  if ((pixelNutSupport.getMsecs() - msecsConnect) > MSECS_CHECK_CONNECT)
  {
    //DBGOUT((F("BLE: check for connection...")));
    isConnected = bfruit.isConnected();
    // NOTE: immediately sending data causes failures

    msecsConnect = pixelNutSupport.getMsecs();
  }

  if (isConnected)
  {
    //DBGOUT((F("BLE: reading...")));
    if (!bfruit.readDataStr(ResponseCB))
    {
      DBGOUT((F("BLE read data failed")));
    }
  }

  return false;  // allow for physical controls
};

#endif // BLUETOOTH_COMM
//========================================================================================
