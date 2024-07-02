# ClubNIKA
main.cpp is inside directory "system".

To format flash and put there all needed files:
- put all .wav-files to SD card and insert it into device
- #define INIT_FLASH in main.cpp and run application
- after init_flash() task ends - undefine INIT_FLASH and rerun application 

## Functionality
### SIM
- initiate calls(notify about alarm)
- accept/decline calls(to activate/deactivate alarm)

### Wired channels
- configurable activation on short(to ground) or break
- LCD allows to monitor currently activated zones

### Speaker
- give voice instructions

### Supply system
- activates siren and 12V supply for sensors
- in case over-current turns off siren and 12V supply, then turns on after several seconds
- charges battery

## SIM900 configuration
Firmware requires:
- baudrate 57600 - send 'AT+IPR=57600'
- no command echo - send 'ATE0&W'
- enable network time request - send 'AT+CLTS=1'
- SMS functions in "Text mode" - send 'AT+CMGF=1'
- used IRA charset - send 'AT+CSCS="IRA"'
- short presentation of incoming SMS - send 'AT+CNMI=2,1,0,0,0'
- report call state changes - send 'AT+CLCC=1'
- send 'AT&W' at the end to save configs