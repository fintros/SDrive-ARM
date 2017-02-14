# SDrive-ARM
Atari800 disk drives emulator based on C.P.U. project SDrive

![image](docs/photos/build07.jpg)

## Specs
- SDrive was developed originally by C.P.U. team in 2008 and use AT Mega 8 processor;
- It is extension of previous project with following features:
  - 5V compatible 32bit Cypress CY8C4245AXI ARM processor with:
    - 48 MHz core
    - 32Kb Flash
    - 4Kb RAM
  - MicroSD card reader
  - Capacitive buttons (Next drive, Prev drive, Boot drive)
  - 6 LEDs
  - Reset button
  - Each SIO pin connected to ARM I/O pin (Possibility for functionality extension)
  - One RW/RO switch for microSD card
  - original drive ID's switchng DIPs excluded
  
## Software needed to open and compile source code
- PCBs created in Eagle software (http://www.autodesk.com/products/eagle/overview)
- ARM firmware created in Cypress PSoC Creator 4.0 (http://www.cypress.com/products/psoc-creator-integrated-design-environment-ide)
- Box sketches created in Sketchup (http://www.sketchup.com/)

## ToDo list
- Fix an issue with daisy-chain connection (add switchng diode to SIO data in line)
- Apply Hias patches to firmware
- Increase number of usable drives up to 8
- Implement cassete recorder functionality

## Contacts
- Please contact me in case of questions by E-Mail: extmail __at__ alsp.net
- Donations are welcomed to the same PayPal address ;)
