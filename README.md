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
- baudrate 57600; configure with "AT+IPR=57600;&W"
- no command echo; configure with "ATE0&W"