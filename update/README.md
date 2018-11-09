# SDrive<sup>2</sup> updates
## ATTENTION!
Please note: any update procedure is potentially dangerous and can damage your firmware. 

You always can fix firmware in case of issues if you have Cypress KitProg programmer. Easiest way is to buy CY8CKIT-043 ($10) or similar kit

So please READ THIS PAGE CAREFULLY! And **proceed at your own risk**!
## Update to version #3
### What's new
* Fix issues with FAT16
* Fix issues with big ATR images and 256byte sectors
* Apply HIAS patches
* Improved SD cards compatibility
### Versions?
All devices and kits sold before November 2018 goes with firmware version #1 - 20170405. You can check version in SDrive boot program by pressing 'H' key.
### Bootloader version
Unfortunately devices from first batch (sold in 2017) comes with 2 different bootloaders, and there is only one way to understand witch bootloader is flashed in your device - by MicroSD card that was in the box. If you have Kingston 2Gb card than you have bootloader version 2, otherwise - bootloader version 1. If you not sure - always try to update with FW update for BL v1. 
### Update procedure
1. Try to determine version of your bootloader. Refer to previous chapter to do it.
2. If you have bootloader version 1 or you not sure:
   * Put [SDRIVE.UPD](https://github.com/fintros/SDrive-ARM/blob/master/update/v3/bl_v01/SDRIVE.UPD) file to your microsd card root;
   * Insert card to SDrive<sup>2</sup>;
   * Insert SDrive<sup>2</sup> to Atari;
   * Power on Atari;
   * Update process should start - its started if you able to see yellow LEDs flashing. Check this [video](https://youtu.be/ZWT5XrAImsw) to see how update processing;
   * When update has finish standard startup LED sequence should proceeded.
   * You can remove SDRIVE.UPD from microsd card
3. If you have bootloader version 2 (or first variant fail and you unable to see yellow blinking)
   * Put [SDRIVE.UPD](https://github.com/fintros/SDrive-ARM/blob/master/update/v3/bl_v02/SDRIVE.UPD) file to your microsd card root;
   * Do same steps like in BL v1 variant
   * If your SDrive<sup>2</sup> switch off all LEDs after flashing - you have BL v1... and should proceed next sequence:
4. If you flash BL v2 update to BL v1 bootloader accidentally ;)
    * If you do it than your SDrive<sup>2</sup> switch off all LEDs after powerup;
    * Put [!SDRIVE.UPD](https://github.com/fintros/SDrive-ARM/blob/master/update/v3/bl_v01/!SDRIVE.UPD) file to your microsd card root;
    * Do same steps like in BL v1 variant

## Happy Flashing! And good luck
I've test these procedures on 10 devices with different bootloaders. Everything works fine. But Please note - 

**You proceed update on your own RISK**