# ECE-4840 Senior Design

## set up host device

you need to run the `setup-prism-usb.sh` script on any new host to
connect to the pi over usb.
after running the script you can connect via `ssh prism-usb`

## Reminders

1. camera uses CVBS output, need some kind of converter to digital format.
prototype using CVBS to USB dongle, will add latency due to usb latency as well as shared bus with WiFi module
for next iteration, use a CVBS to CSI chip.

2. the wifi chip does not have a datasheet, the 2x6 connector is almost definitely USB however, i will find the pins via probing with a multimeter

### Wifi Chip Pins

left: header side
right: ipex connector side

| left | right |
|------|-------|
| x    | x     |
| x    | x     |
| x    | x     |
| x    | x     |
| x    | x     |
| x    | x     |
