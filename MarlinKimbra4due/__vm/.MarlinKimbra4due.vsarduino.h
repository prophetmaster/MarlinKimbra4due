/* 
	Editor: http://www.visualmicro.com
	        visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
	        the contents of the Visual Micro sketch sub folder can be deleted prior to publishing a project
	        all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
	        note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: Arduino Due (Programming Port), Platform=sam, Package=arduino
*/

#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
#define printf iprintf
#define F_CPU 84000000L
#define ARDUINO 160
#define ARDUINO_SAM_DUE
#define ARDUINO_ARCH_SAM
#define __SAM3X8E__
#define USB_VID 0x2341
#define USB_PID 0x003e
#define USBCON
#define __cplusplus
#define __ARM__
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __volatile__
#define __SIZE_TYPE__ long

#define __ICCARM__
#define __ASM
#define __INLINE
#define __builtin_va_list void
//#define _GNU_SOURCE 
//#define __GNUC__ 0
//#undef  __ICCARM__
//#define __GNU__

typedef long Pio;
typedef long Efc;
typedef long Adc;
typedef long Pwm;
typedef long Rtc;
typedef long Rtt;
typedef long pRtc;
typedef long Spi;
typedef long spi;
typedef long Ssc;
//typedef long p_scc;
typedef long Tc;
//typedef long pTc;
typedef long Twi;
typedef long Wdt;
//typedef long pTwi;
typedef long Usart;
typedef long Pdc;
typedef long Rstc;

extern const int ADC_MR_TRGEN_DIS = 0;
extern const int ADC_MR_TRGSEL_ADC_TRIG0 = 0;
extern const int ADC_MR_TRGSEL_Pos = 0;

extern const int ADC_MR_TRGSEL_Msk = 0;
extern const int ADC_MR_TRGEN = 0;
extern const int ADC_MR_TRGSEL_ADC_TRIG1 = 0;
extern const int ADC_MR_TRGSEL_ADC_TRIG2 = 0;
extern const int ADC_MR_TRGSEL_ADC_TRIG3 = 0;



#define __ARMCC_VERSION 400678
#define __attribute__(noinline)

#define prog_void
#define PGM_VOID_P int


typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {;}



#include <arduino.h>
#include <pins_arduino.h> 
#include <variant.h> 
#undef F
#define F(string_literal) ((const PROGMEM char *)(string_literal))
#undef PSTR
#define PSTR(string_literal) ((const PROGMEM char *)(string_literal))
#undef cli
#define cli()
#define pgm_read_byte(address_short)
#define pgm_read_word(address_short)
#define pgm_read_word2(address_short)
#define digitalPinToPort(P)
#define digitalPinToBitMask(P) 
#define digitalPinToTimer(P)
#define analogInPinToBit(P)
#define portOutputRegister(P)
#define portInputRegister(P)
#define portModeRegister(P)
#include <..\MarlinKimbra4due\MarlinKimbra4due.ino>
#include <..\MarlinKimbra4due\Boards.h>
#include <..\MarlinKimbra4due\Configuration_Basic.h>
#include <..\MarlinKimbra4due\Configuration_Cartesian.h>
#include <..\MarlinKimbra4due\Configuration_Core.h>
#include <..\MarlinKimbra4due\Configuration_Delta.h>
#include <..\MarlinKimbra4due\Configuration_Feature.h>
#include <..\MarlinKimbra4due\Configuration_Overall.h>
#include <..\MarlinKimbra4due\Configuration_Pins.h>
#include <..\MarlinKimbra4due\Configuration_Scara.h>
#include <..\MarlinKimbra4due\Configuration_Version.h>
#include <..\MarlinKimbra4due\HAL.cpp>
#include <..\MarlinKimbra4due\HAL.h>
#include <..\MarlinKimbra4due\Marlin_main.cpp>
#include <..\MarlinKimbra4due\Marlin_main.h>
#include <..\MarlinKimbra4due\Sd2Card.cpp>
#include <..\MarlinKimbra4due\Sd2Card.h>
#include <..\MarlinKimbra4due\SdBaseFile.cpp>
#include <..\MarlinKimbra4due\SdBaseFile.h>
#include <..\MarlinKimbra4due\SdFatConfig.h>
#include <..\MarlinKimbra4due\SdFatStructs.h>
#include <..\MarlinKimbra4due\SdFile.cpp>
#include <..\MarlinKimbra4due\SdFile.h>
#include <..\MarlinKimbra4due\SdInfo.h>
#include <..\MarlinKimbra4due\SdVolume.cpp>
#include <..\MarlinKimbra4due\SdVolume.h>
#include <..\MarlinKimbra4due\ServoTimers.h>
#include <..\MarlinKimbra4due\base.h>
#include <..\MarlinKimbra4due\blinkm.cpp>
#include <..\MarlinKimbra4due\blinkm.h>
#include <..\MarlinKimbra4due\buzzer.cpp>
#include <..\MarlinKimbra4due\buzzer.h>
#include <..\MarlinKimbra4due\cardreader.cpp>
#include <..\MarlinKimbra4due\cardreader.h>
#include <..\MarlinKimbra4due\comunication.h>
#include <..\MarlinKimbra4due\conditionals.h>
#include <..\MarlinKimbra4due\configuration_store.cpp>
#include <..\MarlinKimbra4due\configuration_store.h>
#include <..\MarlinKimbra4due\dogm_bitmaps.h>
#include <..\MarlinKimbra4due\dogm_font_data_6x9_marlin.h>
#include <..\MarlinKimbra4due\dogm_font_data_HD44780_C.h>
#include <..\MarlinKimbra4due\dogm_font_data_HD44780_J.h>
#include <..\MarlinKimbra4due\dogm_font_data_HD44780_W.h>
#include <..\MarlinKimbra4due\dogm_font_data_ISO10646_1.h>
#include <..\MarlinKimbra4due\dogm_font_data_ISO10646_5_Cyrillic.h>
#include <..\MarlinKimbra4due\dogm_font_data_ISO10646_Kana.h>
#include <..\MarlinKimbra4due\dogm_font_data_Marlin_symbols.h>
#include <..\MarlinKimbra4due\dogm_lcd_implementation.h>
#include <..\MarlinKimbra4due\external_dac.cpp>
#include <..\MarlinKimbra4due\external_dac.h>
#include <..\MarlinKimbra4due\fastio.h>
#include <..\MarlinKimbra4due\firmware_test.cpp>
#include <..\MarlinKimbra4due\firmware_test.h>
#include <..\MarlinKimbra4due\language.h>
#include <..\MarlinKimbra4due\language_an.h>
#include <..\MarlinKimbra4due\language_bg.h>
#include <..\MarlinKimbra4due\language_ca.h>
#include <..\MarlinKimbra4due\language_cn.h>
#include <..\MarlinKimbra4due\language_da.h>
#include <..\MarlinKimbra4due\language_de.h>
#include <..\MarlinKimbra4due\language_en.h>
#include <..\MarlinKimbra4due\language_es.h>
#include <..\MarlinKimbra4due\language_eu.h>
#include <..\MarlinKimbra4due\language_fi.h>
#include <..\MarlinKimbra4due\language_fr.h>
#include <..\MarlinKimbra4due\language_it.h>
#include <..\MarlinKimbra4due\language_kana.h>
#include <..\MarlinKimbra4due\language_kana_utf8.h>
#include <..\MarlinKimbra4due\language_nl.h>
#include <..\MarlinKimbra4due\language_pl.h>
#include <..\MarlinKimbra4due\language_pt-br.h>
#include <..\MarlinKimbra4due\language_pt.h>
#include <..\MarlinKimbra4due\language_ru.h>
#include <..\MarlinKimbra4due\macros.h>
#include <..\MarlinKimbra4due\mechanics.h>
#include <..\MarlinKimbra4due\nextion_lcd.cpp>
#include <..\MarlinKimbra4due\nextion_lcd.h>
#include <..\MarlinKimbra4due\pins.h>
#include <..\MarlinKimbra4due\planner.cpp>
#include <..\MarlinKimbra4due\planner.h>
#include <..\MarlinKimbra4due\qr_solve.cpp>
#include <..\MarlinKimbra4due\qr_solve.h>
#include <..\MarlinKimbra4due\sanitycheck.h>
#include <..\MarlinKimbra4due\servo.cpp>
#include <..\MarlinKimbra4due\servo.h>
#include <..\MarlinKimbra4due\stepper.cpp>
#include <..\MarlinKimbra4due\stepper.h>
#include <..\MarlinKimbra4due\stepper_indirection.cpp>
#include <..\MarlinKimbra4due\stepper_indirection.h>
#include <..\MarlinKimbra4due\temperature.cpp>
#include <..\MarlinKimbra4due\temperature.h>
#include <..\MarlinKimbra4due\thermistortables.h>
#include <..\MarlinKimbra4due\ultralcd.cpp>
#include <..\MarlinKimbra4due\ultralcd.h>
#include <..\MarlinKimbra4due\ultralcd_implementation_hitachi_HD44780.h>
#include <..\MarlinKimbra4due\ultralcd_st7920_u8glib_rrd.h>
#include <..\MarlinKimbra4due\utf_mapper.h>
#include <..\MarlinKimbra4due\vector_3.cpp>
#include <..\MarlinKimbra4due\vector_3.h>
#include <..\MarlinKimbra4due\watchdog.cpp>
#include <..\MarlinKimbra4due\watchdog.h>
#endif
