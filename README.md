# ClubNIKA
## Functionality
### SIM
- initiate calls(notify about alarm)
- accept/decline calls(activate/deactivate alarm)
- talk during a call? respond on key?

### Keyboard+LCD
- accept password to activate/deactivate
- show notifications that something is wrong(over-current, state of channels)
- display battery voltage
- setup channel normal state?
- setup phone number of owner?

### Wired channels
- monitor if closed/opened
- read current(in current loop configuration)?

### RF extension
- LoRa

### Speaker
- give voice instructions

### Supply system
- activate alarm
- activate external 12V
- over-current protection
- charge battery

## SIM900 configuration
Firmware requires:
- baudrate 57600 - send 'AT+IPR=57600'
- no command echo - send 'ATE0&W'
- enable network time request - send 'AT+CLTS=1'
- SMS functions in "Text mode" - send 'AT+CMGF=1'
- used IRA charset - send 'AT+CSCS="IRA"'
- short present. of incoming SMS - send 'AT+CNMI=2,1,0,0,0'
- report call state changes - send 'AT+CLCC=1'
- send 'AT&W' at the end to save configs