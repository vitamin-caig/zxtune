//
//  exSID_defs.h
//	A simple I/O library for exSID USB - private header file
//
//  (C) 2015-2017 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html
//

/** 
 * @file 
 * libexsid private definitions header file.
 * @note These defines are closely related to the exSID firmware.
 * Any modification that does not correspond to a related change in firmware
 * will cause the device to operate unpredictably or not at all.
 */

#ifndef exSID_defs_h
#define exSID_defs_h

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

// CLOCK_FREQ_NTSC = 1022727.14;
// CLOCK_FREQ_PAL  = 985248.4;

/* common definition for all hardware platforms */
/**
 * USB buffer multiple.
 * Each 64-byte USB packet contains 62 user bytes. 4k buffers (the default) thus contain 3968 data bytes.
 * For optimal throughput performance, data should be sent in chunks that are multiples of this value.
 * See http://www.ftdichip.com/Support/Documents/AppNotes/AN232B-03_D2XXDataThroughput.pdf section 1.4
 */
#define	XSC_USBMOD	(4096/64*62)
#define	XSC_USBLAT	1		///< FTDI latency: 1-255ms in 1ms increments
#define	XSC_BUFFMS	40		///< write buffer size in milliseconds of playback (high water mark).

/* exSID hardware definitions */
#define	XS_BDRATE	2000000		///< 2Mpbs
#define	XS_SIDCLK	1000000		///< 1MHz (for computation only, currently hardcoded in firmware)
#define XS_RSBCLK	(XS_BDRATE/10)	///< RS232 byte clock. Each RS232 byte is 10 bits long due to start and stop bits
#define	XS_CYCCHR	(XS_SIDCLK/XS_RSBCLK)	///< SID cycles between two consecutive chars
//#define	XS_CYCCHR	((XS_SIDCLK+XS_RSBCLK-1)/XS_RSBCLK)	// ceiling
#define	XS_BUFFSZ	((((XS_RSBCLK/1000)*XSC_BUFFMS)/XSC_USBMOD)*XSC_USBMOD)	///< Must be multiple of user data transfer size or USB won't be happy. This floors XSC_BUFFMS.
#define	XS_LDMULT	501		///< long delay SID cycle loop multiplier

#define XS_MINDEL	(XS_CYCCHR)	///< Smallest possible delay (with IOCTD1).
#define	XS_CYCIO	(2*XS_CYCCHR)	///< minimum cycles between two consecutive I/Os (addr + data)
#define	XS_MAXADJ	7		///< maximum encodable value for post write clock adjustment: must fit on 3 bits
#define XS_LDOFFS	(3*XS_CYCCHR)	///< long delay loop SID cycles offset

/* exSID+ hardware definitions */
#define XSP_RSBCLK	666667		///< maximum byte clock at 32MHz base clock is approx 666.6kHz (1 byte every 1.5us)
#define	XSP_BUFFSZ	((((XSP_RSBCLK/1000)*XSC_BUFFMS)/XSC_USBMOD)*XSC_USBMOD)	///< Must be multiple of user data transfer size or USB won't be happy. This floors XSC_BUFFMS.
#define XSP_MINDEL	2		///< Smallest possible delay (with IOCTD1).
#define	XSP_CYCIO	3		///< minimum cycles between two consecutive I/Os (addr + data)
#define XSP_PRE_RD	2
#define XSP_POSTRD	2
#define	XSP_MAXADJ	4		///< maximum encodable value for post write clock adjustment: must fit on 3 bits
#define XSP_LDOFFS	3		///< long delay loop SID cycles offset
#define XSP_CYCCS	2		///< cycles lost in chipselect()

/* IOCTLS */
/* IO controls 0x3D to 0x7F are only implemented on exSID+ */
#define XSP_AD_IOCTCP	0x3D	///< Select PAL clock
#define XSP_AD_IOCTCN	0x3E	///< Select NTSC clock
#define XSP_AD_IOCTC1	0x3F	///< Select 1MHz clock

#define XSP_AD_IOCTA0	0x5D	///< Audio Mix: 6581 L / 8580 R
#define XSP_AD_IOCTA1	0x5E	///< Audio Mix: 8580 L / 6581 R
#define XSP_AD_IOCTA2	0x5F	///< Audio Mix: 8580 L / 8580 R

#define XSP_AD_IOCTA3	0x7D	///< Audio Mix: 6581 L / 6581 R
#define XSP_AD_IOCTAM	0x7E	///< Audio Mute
#define XSP_AD_IOCTAU	0x7F	///< Audio Unmute

#define XS_AD_IOCTD1	0x9D	///< shortest delay (XS_MINDEL SID cycles)
#define	XS_AD_IOCTLD	0x9E	///< polled delay, amount of SID cycles to wait must be given in data

#define	XS_AD_IOCTS0	0xBD	///< select chip 0
#define XS_AD_IOCTS1	0xBE	///< select chip 1
#define XS_AD_IOCTSB	0xBF	///< select both chips. @warning Invalid for reads: unknown behaviour!

#define	XS_AD_IOCTFV	0xFD	///< Firmware version query
#define	XS_AD_IOCTHV	0xFE	///< Hardware version query
#define XS_AD_IOCTRS	0xFF	///< SID reset

#define XS_USBVID	0x0403	///< Default FTDI VID
#define XS_USBPID	0x6001	///< Default FTDI PID
#define XS_USBDSC	"exSID USB"

#define XSP_USBVID	0x0403	///< Default FTDI VID
#define XSP_USBPID	0x6015	///< Default FTDI PID
#define XSP_USBDSC	"exSID+ USB"

#define XS_MODEL_STD	0
#define XS_MODEL_PLUS	1

#ifdef DEBUG
 #define xsdbg(format, ...)       printf("(%s) " format, __func__, ## __VA_ARGS__)
#else
 #define xsdbg(format, ...)       /* nothing */
#endif

#ifdef HAVE_BUILTIN_EXPECT
 #define likely(x)       __builtin_expect(!!(x), 1)
 #define unlikely(x)     __builtin_expect(!!(x), 0)
#else
 #define likely(x)      (x)
 #define unlikely(x)    (x)
#endif

#endif /* exSID_defs_h */
