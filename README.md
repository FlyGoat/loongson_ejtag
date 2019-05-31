# loongson_ejtag
Reverse Engineering Based USB Loongson EJTAG Prober

## How to Flash fx2lp EEPROM
### Build tools
```
cd fx2lp/fxload
make
```
```
cd fx2lp/fx2eeprom
make
```
### Upload Vend_ax tool to fx2lp
```
 ./fx2lp/fxload/fxload -I ./fx2lp/Vend_ax/Vend_Ax.hex  -D /dev/bus/usb/001/003  -t fx2lp -v
```
Note: USB device path may change.

### Use fx2eeprom to flash EEPROM
```
cat ./dumps/24c64.bin | ./fx2lp/fxeeprom/fx2eeprom w 0x2961 0x6688 0x1000
```