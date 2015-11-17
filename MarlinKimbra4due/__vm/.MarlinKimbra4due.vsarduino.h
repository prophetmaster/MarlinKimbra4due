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
#define ARDUINO 164
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
#define __asm__ 
#define __volatile__


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
extern const int ADC_TRIG_TIO_CH_0 = 0;
extern const int ADC_MR_TRGSEL_ADC_TRIG1 = 0;
extern const int ADC_TRIG_TIO_CH_1 = 0;
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
#include <MarlinKimbra4due.ino>
#include <Boards.h>
#include <Configuration_Basic.h>
#include <Configuration_Cartesian.h>
#include <Configuration_Core.h>
#include <Configuration_Delta.h>
#include <Configuration_Feature.h>
#include <Configuration_Overall.h>
#include <Configuration_Pins.h>
#include <Configuration_Scara.h>
#include <Configuration_Version.h>
#include <HAL.cpp>
#include <HAL.h>
#include <Marlin_main.cpp>
#include <Marlin_main.h>
#include <Sd2Card.cpp>
#include <Sd2Card.h>
#include <SdBaseFile.cpp>
#include <SdBaseFile.h>
#include <SdFatConfig.h>
#include <SdFatStructs.h>
#include <SdFile.cpp>
#include <SdFile.h>
#include <SdInfo.h>
#include <SdVolume.cpp>
#include <SdVolume.h>
#include <ServoTimers.h>
#include <base.h>
#include <blinkm.cpp>
#include <blinkm.h>
#include <buzzer.cpp>
#include <buzzer.h>
#include <cardreader.cpp>
#include <cardreader.h>
#include <comunication.h>
#include <conditionals.h>
#include <configuration_store.cpp>
#include <configuration_store.h>
#include <dogm_bitmaps.h>
#include <dogm_font_data_6x9_marlin.h>
#include <dogm_font_data_HD44780_C.h>
#include <dogm_font_data_HD44780_J.h>
#include <dogm_font_data_HD44780_W.h>
#include <dogm_font_data_ISO10646_1.h>
#include <dogm_font_data_ISO10646_5_Cyrillic.h>
#include <dogm_font_data_ISO10646_Kana.h>
#include <dogm_font_data_Marlin_symbols.h>
#include <dogm_lcd_implementation.h>
#include <external_dac.cpp>
#include <external_dac.h>
#include <fastio.h>
#include <firmware_test.cpp>
#include <firmware_test.h>
#include <language.h>
#include <language_an.h>
#include <language_bg.h>
#include <language_ca.h>
#include <language_cn.h>
#include <language_da.h>
#include <language_de.h>
#include <language_en.h>
#include <language_es.h>
#include <language_eu.h>
#include <language_fi.h>
#include <language_fr.h>
#include <language_it.h>
#include <language_kana.h>
#include <language_kana_utf8.h>
#include <language_nl.h>
#include <language_pl.h>
#include <language_pt-br.h>
#include <language_pt.h>
#include <language_ru.h>
#include <macros.h>
#include <mechanics.h>
#include <nextion_lcd.cpp>
#include <nextion_lcd.h>
#include <pins.h>
#include <planner.cpp>
#include <planner.h>
#include <qr_solve.cpp>
#include <qr_solve.h>
#include <sanitycheck.h>
#include <servo.cpp>
#include <servo.h>
#include <stepper.cpp>
#include <stepper.h>
#include <stepper_indirection.cpp>
#include <stepper_indirection.h>
#include <temperature.cpp>
#include <temperature.h>
#include <thermistortables.h>
#include <ultralcd.cpp>
#include <ultralcd.h>
#include <ultralcd_implementation_hitachi_HD44780.h>
#include <ultralcd_st7920_u8glib_rrd.h>
#include <utf_mapper.h>
#include <vector_3.cpp>
#include <vector_3.h>
#include <watchdog.cpp>
#include <watchdog.h>
#endif
