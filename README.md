# DG24064-50
Arduino library for driving DataVision DG-24064-50 S2MBLY-H display

The DG-24064-50 S2MBLY-H display has no controller or memory but only LCD segment drivers, so Arduino is used as display controller with font table, video memory and procedures for drawing graphic primitives, text, sprites etc.

It was planned to use SPI SRAM chips for additional memory but this feature is not implemented.

Arduino 168/328 chips have low RAM, so it's almost entirely used for video memory leaving zero space for other actions. Then SPI SRAM memory feature should be implemented or using PROGMEM instead of RAM, with both options affecting performance negatively.

Arduino Mega 1280/2560 is recommended, as it's capable to drive the display and have memory space for other activities. 
