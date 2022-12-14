![Bekant PCB](/bekant_pcb.jpg)
![Schematic](/schematic.png)
# Instructions
- set up Rasperry Pi Pico environment as described [here](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
- prepare Bekant PCB as described [here](https://github.com/robin7331/IKEA-Hackant)
- install firmware onto Raspberry Pi Pico
  - mkdir build
  - cd build
  - cmake ..
  - make
  - boot Rasperry Pi Pico in storage mode and copy `main.uf2` onto it
- create & connect circuit board as in `schematic.png`
  - `TP1 LIN` corresponds to the `TP1` header on the Bekant PCB
  - `H5` and `H6` corresponds to the 5th and 6th header pin on the Bekant PCB as in `bekant_pcb.jpg`
- a long press on `BUTTON_UP` / `BUTTON_DOWN` will manually move the desk up / down
- a short press on `BUTTON_UP` / `BUTTON_DOWN` will move the desk to the pre-defined `POSITION_STAND` / `POSITION_SIT`
- the desk will also report its height (as reported on the LIN bus and converted to millimeters) over the USB serial interface
