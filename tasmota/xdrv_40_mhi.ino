#ifdef USE_MHI_SPI
/*********************************************************************************************\
 * Mitsubishi Heavy Industries Tasmota library driver
\*********************************************************************************************/

#define XDRV_40                    40

#include <MHI_AC_Ctrl.h>

const PROGMEM char* kResponseOk = "true";
const PROGMEM char* kResponseFail = "false";

const char kMhiCommands[] PROGMEM = "|"  // No prefix
        D_CMND_MHI_POWER "|" D_CMND_MHI_MODE "|" D_CMND_MHI_TSETPOINT "|" D_CMND_MHI_FAN "|" D_CMND_MHI_VANES "|"
        D_CMND_MHI_ERROPDATA;

void (* const MhiCommand[])(void) PROGMEM = {
  &CmndMhiPower, &CmndMhiMode, &CmndMhiTsetpoint, &CmndMhiSetFanSpeed, &CmndMhiSetVanes, &CmndMhiErrOpData};

class MhiEventHandler : public CallbackInterface {
public:
    bool cbiMhiEventHandlerFunction(const char* key, const char* value) {
      Response_P(PSTR("%s"), value);
      MqttPublishPrefixTopic_P(TELE, key);
    }
};

MhiEventHandler mhiEventHandler;
MHIAcCtrl mhiAc = MHIAcCtrl(&mhiEventHandler);

bool MhiInit(void)
{
  bool result = true;
  AddLog_P2(LOG_LEVEL_DEBUG,PSTR("MHI SPI Initializing..."));
  result = true;
  pinMode(SCK, INPUT);
  pinMode(MOSI, INPUT);
  pinMode(MISO, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);  // indicates that a frame was received, active low
  digitalWrite(LED_BUILTIN, HIGH);
  AddLog_P2(LOG_LEVEL_ERROR,PSTR("MHI SPI Init result: %s"), result? kResponseOk : kResponseFail);
  return result;
}

/*********************************************************************************************\
 * Commands
\*********************************************************************************************/
void CmndMhiPower(void)
{
  bool result = false;
  if(XdrvMailbox.payload == 0) {
      result |= mhiAc.powerOff();
  } else if(XdrvMailbox.payload == 1) {
      result |= mhiAc.powerOn();
  } else {
      return;
  }
  Response_P(result ? kResponseOk : kResponseFail);
}

const char kMode_Commands[] PROGMEM = "Auto|Dry|Cool|Fan|Heat|";
char _parsedCommand[20];
void CmndMhiMode(void)
{
    int command_code = GetCommandCode(_parsedCommand, sizeof(_parsedCommand), XdrvMailbox.data, kMode_Commands);
    AddLog_P2(LOG_LEVEL_INFO,PSTR("mode payload=%d data='%s', cmnd_code=%d (%d) command=%s"), XdrvMailbox.payload,
            XdrvMailbox.data, XdrvMailbox.command_code, command_code, _parsedCommand);
    if (command_code > -1 && mhiAc.setMode(static_cast<MhiMode>(command_code))) {
        Response_P(kResponseOk);
    }
}

void CmndMhiTsetpoint(void)
{
    if (mhiAc.tSetpoint(XdrvMailbox.payload)) {
        Response_P(kResponseOk);
    }
}

const char kFanSpeed_Commands[] PROGMEM = "Auto|Low|Med|High|";
void CmndMhiSetFanSpeed(void)
{
    int command_code = GetCommandCode(_parsedCommand, sizeof(_parsedCommand), XdrvMailbox.data, kFanSpeed_Commands);
    if (command_code > -1 && mhiAc.setMode(static_cast<MhiMode>(command_code+1))) {
        Response_P(kResponseOk);
    }
}

const char kVanes_Commands[] PROGMEM = "Down|DownMiddle|UpMiddle|Up|Swing|";
void CmndMhiSetVanes(void)
{
    int command_code = GetCommandCode(_parsedCommand, sizeof(_parsedCommand), XdrvMailbox.data, kVanes_Commands);
    if (command_code > -1 && mhiAc.setVanes(static_cast<MhiVanes>(command_code+1))) {
        Response_P(kResponseOk);
    }
}

void CmndMhiErrOpData(void)
{
    bool result = mhiAc.getErrOpData();
    Response_P(result ? kResponseOk : kResponseFail);
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/
bool Xdrv40(uint8_t function)
{
  bool result = false;

    switch (function) {
      case FUNC_LOOP:
        mhiAc.loop();
        break;
      case FUNC_PRE_INIT:
        break;
      case FUNC_INIT:
        result = MhiInit();
        break;
      case FUNC_COMMAND:
        result = DecodeCommand(kMhiCommands, MhiCommand);
        break;
    }

  return result;
}

#endif // USE_MHI_SPI
