//
//  exSID.c
//	A simple I/O library for exSID/exSID+ USB
//
//  (C) 2015-2018,2021 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html

/**
 * @file
 * exSID/exSID+ USB I/O library
 * @author Thibaut VARENE
 * @date 2015-2018,2021
 * @version 2.1
 *
 * This driver will control the first exSID device available.
 * All public API functions are only valid after a successful call to exSID_init().
 * To release the device and resources, exSID_exit() and exSID_free() must be called.
 *
 * The return value for public routines returning an integer is 0 for successful execution
 * and !0 for error, unless otherwise noted.
 *
 * @warning Although it can internally make use of two separate threads, the driver
 * implementation is NOT thread safe (since it is not expected that a SID device may
 * be accessed concurrently), and some optimizations in the code are based on the
 * assumption that the code is run within a single-threaded environment.
 */

#include "exSID.h"
#include "exSID_defs.h"
#include "exSID_ftdiwrap.h"
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#ifdef	EXSID_THREADED
 #if defined(HAVE_PTHREAD_H)
  #include <pthread.h>
 #else
  #error "No thread model available"
 #endif
#endif	// EXSID_TRHREADED

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof(x[0]))
#define XS_ERRORBUF		256
#define xserror(xs, format, ...)      snprintf(xs->xSerrstr, XS_ERRORBUF, "(%s) ERROR " format, __func__, ## __VA_ARGS__)

/**
 * cycles is uint_fast32_t. Technically, clkdrift should be int_fast64_t though
 * overflow should not happen under normal conditions.
 */
typedef int_fast32_t clkdrift_t;

/**
 * This private structure holds hardware-dependent constants.
 */
struct xSconsts_s {
	unsigned int	model;			///< exSID device model in use
	clkdrift_t	write_cycles;		///< number of SID clocks spent in write ops
	clkdrift_t	read_pre_cycles;	///< number of SID clocks spent in read op before data is actually read
	clkdrift_t	read_post_cycles;	///< number of SID clocks spent in read op after data is actually read
	clkdrift_t	read_offset_cycles;	///< read offset adjustment to align with writes (see function documentation)
	clkdrift_t	csioctl_cycles;		///< number of SID clocks spent in chip select ioctl
	clkdrift_t	mindel_cycles;		///< lowest number of SID clocks that can be accounted for in delay
	clkdrift_t	max_adj;		///< maximum number of SID clocks that can be encoded in final delay for read()/write()
	size_t		buff_size;		///< output buffer size
};

/** Array of supported devices */
static const struct {
	const char *		desc;	///< USB device description string
	const int		pid;	///< USB PID
	const int		vid;	///< USB VID
	const struct xSconsts_s	xsc;	///< constants specific to the device
} xSsupported[] = {
	{
		/* exSID USB */
		.desc = XS_USBDSC,
		.pid = XS_USBPID,
		.vid = XS_USBVID,
		.xsc = (struct xSconsts_s){
			.model = XS_MODEL_STD,
			.write_cycles = XS_CYCIO,
			.read_pre_cycles = XS_CYCCHR,
			.read_post_cycles = XS_CYCCHR,
			.read_offset_cycles = -2,	// see exSID_clkdread() documentation
			.csioctl_cycles = XS_CYCCHR,
			.mindel_cycles = XS_MINDEL,
			.max_adj = XS_MAXADJ,
			.buff_size = XS_BUFFSZ,
		},
	}, {
		/* exSID+ USB */
		.desc = XSP_USBDSC,
		.pid = XSP_USBPID,
		.vid = XSP_USBVID,
		.xsc = (struct xSconsts_s){
			.model = XS_MODEL_PLUS,
			.write_cycles = XSP_CYCIO,
			.read_pre_cycles = XSP_PRE_RD,
			.read_post_cycles = XSP_POSTRD,
			.read_offset_cycles = 0,
			.csioctl_cycles = XSP_CYCCS,
			.mindel_cycles = XSP_MINDEL,
			.max_adj = XSP_MAXADJ,
			.buff_size = XSP_BUFFSZ,
		},
	}
};

/** exSID private handle */
struct _exsid {
	/** negative values mean we're lagging, positive mean we're ahead. See it as a number of SID clocks queued to be spent. */
	clkdrift_t clkdrift;

	const struct xSconsts_s * restrict cst;	///< Pointer to constants used by all the hardware access routines.
	void * ftdi;				///< FTDI device handle.

#ifdef	DEBUG
	long accdrift;
	unsigned long accioops;
	unsigned long accdelay;
	unsigned long acccycle;	// ensures no overflow with exSID+ up to ~1h of continuous playback
#endif

	int backbuf_idx;			///< current index for the back buffer
	unsigned char * restrict backbuf;	///< back buffer

#ifdef	EXSID_THREADED
	// Variables for flip buffering
	pthread_mutex_t flip_mtx;
	pthread_cond_t backbuf_full_cnd, backbuf_avail_cnd;
	unsigned char * restrict frontbuf;	///< front buffer
	_Bool backbuf_ready;			///< signals back buffer is ready to be flipped
	int postdelay;				///< post delay applied after writing frontbuf to device, before the next write is attempted
	pthread_t thread_output;
#endif	// EXSID_THREADED

	uint16_t hwvers;			///< hardware version
	char xSerrstr[XS_ERRORBUF+1];		///< XS_ERRORBUF-byte max string for last error message
};

/**
 * Signal-safe nanosleep() wrapper, can be used to wait for exSID.
 * Some operations stall the device and because we don't use flow control on exSID, we may need to wait.
 * exSID+ use full bidirectionnal flow control and does not require the wait.
 * @param usecs requested sleep time in microseconds.
 * @return return value of nanosleep(), with EINTR handled internally.
 */
static int _xSusleep(int usecs)
{
	struct timespec tv;
	int ret;

	xsdbg("sleep: %d\n", usecs);

	tv.tv_sec = usecs / 1000000;
	tv.tv_nsec = (usecs % 1000000) * 1000;

	while (1) {
		ret = nanosleep(&tv, &tv);
		if (ret) {
			if (EINTR == errno)
				continue;
		}
		return ret;
	}
}

/**
 * Returns a string describing the last recorded error.
 * @param exsid exsid handle
 * @return error message (max 256 bytes long).
 */
const char * exSID_error_str(void * const exsid)
{
	const struct _exsid * const xs = exsid;
	if (!exsid)
		return NULL;
	return (xs->xSerrstr);
}


/**
 * Write routine to send data to the device.
 * Calls USB wrapper.
 * @note BLOCKING.
 * @param xs exsid private pointer
 * @param buff pointer to a byte array of data to send
 * @param size number of bytes to send
 */
static inline void xSwrite(struct _exsid * const xs, const unsigned char * buff, int size)
{
	int ret = xSfw_write_data(xs->ftdi, buff, size);
#ifdef	DEBUG
	if (unlikely(ret < 0)) {
		xsdbg("Error ftdi_write_data(%d): %s\n",
			ret, xSfw_get_error_string(xs->ftdi));
	}
	if (unlikely(ret != size)) {
		xsdbg("ftdi_write_data only wrote %d (of %d) bytes\n",
			ret, size);
	}
#endif
}

/**
 * Read routine to get data from the device.
 * Calls USB wrapper.
 * @warning if EXSID_THREADED is enabled, must not be called once the
 * output thread is writing to the device (no synchronization with output thread).
 * @note BLOCKING.
 * @param xs exsid private pointer
 * @param buff pointer to a byte array that will be filled with read data
 * @param size number of bytes to read
 */
static void xSread(struct _exsid * const xs, unsigned char * buff, int size)
{
	int ret = 0;

	// libftdi implements a non-blocking read() call that returns 0 if no data was available
	do {
		ret += xSfw_read_data(xs->ftdi, buff+ret, size-ret);
	} while (ret != size);

#ifdef	DEBUG
	if (unlikely(ret < 0)) {
		xsdbg("Error ftdi_read_data(%d): %s\n",
			ret, xSfw_get_error_string(xs->ftdi));
	}
	if (unlikely(ret != size)) {
		xsdbg("ftdi_read_data only read %d (of %d) bytes\n",
			ret, size);
	}
#endif
}


#ifdef	EXSID_THREADED
/**
 * Writer thread. ** consumer **
 * This thread consumes the buffer prepared in xSoutb().
 * Since writes to the FTDI subsystem are blocking, this thread blocks when it's
 * writing to the chip, and also while it's waiting for the back buffer to be ready.
 * When the back buffer is ready, it is flipped with the front one and immediately
 * released for the next fill, while the front buffer is written to the device.
 * @note BLOCKING.
 * @param arg exsid handle
 * @return DOES NOT RETURN, exits when xs->postdelay is negative.
 */
static void * _exSID_thread_output(void * arg)
{
	struct _exsid * const xs = arg;
	unsigned char * bufptr;
	int frontbuf_idx, delay;

#ifdef _GNU_SOURCE
	pthread_setname_np(pthread_self(), "output");
#endif

	xsdbg("thread started\n");
	while (1) {
		pthread_mutex_lock(&xs->flip_mtx);

		// wait for backbuf full
		while (!xs->backbuf_ready)
			pthread_cond_wait(&xs->backbuf_full_cnd, &xs->flip_mtx);

		// Keep the critical section fast and short:
		delay = xs->postdelay;			// record postdelay
		bufptr = xs->frontbuf;
		frontbuf_idx = xs->backbuf_idx;
		xs->frontbuf = xs->backbuf;		// flip back to front
		xs->backbuf = bufptr;
		xs->backbuf_idx = 0;
		xs->backbuf_ready = 0;

		// signal we're done and free mutex
		pthread_cond_signal(&xs->backbuf_avail_cnd);

		pthread_mutex_unlock(&xs->flip_mtx);

		// this blocks outside of mutex held
		xSwrite(xs, xs->frontbuf, frontbuf_idx);

		if (unlikely(delay)) {
			if (unlikely(delay < 0)) {	// exit condition
				xsdbg("thread exiting!\n");
				pthread_exit(NULL);
			}
			else if (XS_MODEL_STD == xs->cst->model)
				_xSusleep(delay);
		}
	}
}
#endif	// EXSID_THREADED

/**
 * Single byte output routine. ** producer **
 * Fills a buffer with bytes to send to the device until the buffer is full or a forced write is triggered.
 * @note No drift compensation is performed on read operations.
 * @param xs exsid private pointer
 * @param byte byte to send
 * @param flush force write flush if positive (add usleep(flush) if >1), trigger thread exit if negative
 */
static void xSoutb(struct _exsid * const xs, uint8_t byte, int flush)
{
	xs->backbuf[xs->backbuf_idx++] = (unsigned char)byte;

	if (likely((xs->backbuf_idx < xs->cst->buff_size) && !flush))
		return;

#ifdef	EXSID_THREADED
	// buffer dance
	pthread_mutex_lock(&xs->flip_mtx);

	xs->postdelay = flush;

	// signal backbuff full
	xs->backbuf_ready = 1;
	pthread_cond_signal(&xs->backbuf_full_cnd);

	// wait for buffer flipped
	while (xs->backbuf_idx)
		pthread_cond_wait(&xs->backbuf_avail_cnd, &xs->flip_mtx);

	pthread_mutex_unlock(&xs->flip_mtx);
#else	// unthreaded
	xSwrite(xs, xs->backbuf, xs->backbuf_idx);
	xs->backbuf_idx = 0;
	if (unlikely((flush > 1) && (XS_MODEL_STD == xs->cst->model)))
		_xSusleep(flush);
#endif
}

/**
 * Allocate an exSID handle.
 * @return allocated opaque handle, NULL if error.
 */
void * exSID_new(void)
{
	struct _exsid * xs;
	xs = calloc(1, sizeof(*xs));
	return xs;
}

/**
 * Deallocate an exSID handle.
 * Frees up all memory used by the exSID handle.
 * @param exsid exsid handle
 */
void exSID_free(void * exsid)
{
	struct _exsid * xs = exsid;

	if (!xs)
		return;

	if (xSfw_free && xs->ftdi)
		xSfw_free(xs->ftdi);

#ifdef	EXSID_THREADED
	if (xs->frontbuf)
		free(xs->frontbuf);
#endif

	if (xs->backbuf)
		free(xs->backbuf);

	free(xs);
}

/**
 * Device init routine.
 * Must be called once before any operation is attempted on the device.
 * Opens first available device, and sets various parameters: baudrate, parity, flow
 * control and USB latency.
 * @note If this function fails, exSID_free() must still be called.
 * @param exsid allocated exsid handle generated with exSID_new()
 * @return 0 on success, !0 otherwise.
 */
int exSID_init(void * const exsid)
{
	struct _exsid * const xs = exsid;
	unsigned char buf[2];
	int i, ret;

	xsdbg("libexsid v" EXSID_VERSION "\n");

	if (!exsid) {
		fprintf(stderr, "ERROR: exSID_init: invalid handle\n");
		return -1;
	}

	if (xSfw_dlopen()) {
		xserror(xs, "Failed to open dynamic loader");
		return -1;
	}

	/* Attempt to open all supported devices until first success.
	 * Cleanup ftdi after each try to avoid passing garbage around as we don't know what
	 * the FTDI open routines do with the pointer. */
	for (i = 0; i < ARRAY_SIZE(xSsupported); i++) {
		if (xSfw_new) {
			xs->ftdi = xSfw_new();
			if (!xs->ftdi) {
				xserror(xs, "ftdi_new failed");
				return -1;
			}
		}

		xsdbg("Trying %s...\n", xSsupported[i].desc);
		xs->cst = &xSsupported[i].xsc;	// setting unconditionnally avoids segfaults if user code calls exit() after failure.
		ret = xSfw_usb_open_desc(&xs->ftdi, xSsupported[i].vid, xSsupported[i].pid, xSsupported[i].desc, NULL);
		if (ret >= 0) {
			xsdbg("Opened!\n");
			break;
		}
		else {
			xsdbg("Failed: %d (%s)\n", ret, xSfw_get_error_string(xs->ftdi));
			if (xSfw_free)
				xSfw_free(xs->ftdi);
			xs->ftdi = NULL;
		}
	}

	if (i >= ARRAY_SIZE(xSsupported)) {
		xserror(xs, "No device could be opened");
		return -1;
	}

	ret = xSfw_usb_setup(xs->ftdi, XS_BDRATE, XSC_USBLAT);
	if (ret < 0) {
		xserror(xs, "Failed to setup device");
		return -1;
	}

	// success - device is setup
	xsdbg("Device setup\n");

	// Wait for device ready by trying to read hw version and wait for the answer
	buf[0] = XS_AD_IOCTHV;
	buf[1] = XS_AD_IOCTFV;	// ok as we have a 2-byte RX buffer on PIC
	xSwrite(xs, buf, 2);
	xSread(xs, buf, 2);
	xs->hwvers = buf[0] << 8 | buf[1];	// ensure proper order regardless of endianness
	xsdbg("HV: %c, FV: %hhu\n", buf[0], buf[1]);

	xs->backbuf = malloc(xs->cst->buff_size);
	if (!xs->backbuf) {
		xserror(xs, "Out of memory!");
		return -1;
	}

#ifdef	EXSID_THREADED
	xs->frontbuf = malloc(xs->cst->buff_size);
	if (!xs->frontbuf) {
		xserror(xs, "Out of memory!");
		return -1;
	}

	ret = pthread_mutex_init(&xs->flip_mtx, NULL);
	ret |= pthread_cond_init(&xs->backbuf_avail_cnd, NULL);
	ret |= pthread_cond_init(&xs->backbuf_full_cnd, NULL);
	ret |= pthread_create(&xs->thread_output, NULL, _exSID_thread_output, xs);
	if (ret) {
		xserror(xs, "Thread setup failed");
		return -1;
	}

	xsdbg("Thread setup\n");
#endif

	xsdbg("buffer size: %zu bytes\n", xs->cst->buff_size);
	xsdbg("Rock'n'roll!\n");

	return 0;
}

/**
 * Device exit routine.
 * Must be called to release the device.
 * Resets the SIDs, frees buffers and closes FTDI device.
 * @param exsid exsid handle
 * @return execution status
 */
int exSID_exit(void * const exsid)
{
	struct _exsid * const xs = exsid;
	int ret = 0;

	if (!xs)
		return -1;

	if (xs->ftdi) {
		exSID_reset(xs);

#ifdef	EXSID_THREADED
		xSoutb(xs, XS_AD_IOCTD1, -1);	// signal end of thread
		pthread_join(xs->thread_output, NULL);
		pthread_cond_destroy(&xs->backbuf_full_cnd);
		pthread_cond_destroy(&xs->backbuf_avail_cnd);
		pthread_mutex_destroy(&xs->flip_mtx);
		if (xs->frontbuf)
			free(xs->frontbuf);
		xs->frontbuf = NULL;
#endif

		if (xs->backbuf)
			free(xs->backbuf);
		xs->backbuf = NULL;

		ret = xSfw_usb_close(xs->ftdi);
		if (ret < 0)
			xserror(xs, "Unable to close ftdi device: %d (%s)",
				ret, xSfw_get_error_string(xs->ftdi));

		if (xSfw_free)
			xSfw_free(xs->ftdi);
		xs->ftdi = NULL;

#ifdef	DEBUG
		xsdbg("mean jitter: %.2f cycle(s) over %lu I/O ops\n",
			((float)xs->accdrift/xs->accioops), xs->accioops);
		xsdbg("bandwidth used for I/O ops: %lu%% (approx)\n",
			100-(xs->accdelay*100/xs->acccycle));
		xs->accdrift = xs->accioops = xs->accdelay = xs->acccycle = 0;
#endif
	}

	xs->clkdrift = 0;
	xSfw_dlclose();

	return ret;
}


/**
 * SID reset routine.
 * Performs a hardware reset on the SIDs.
 * This also resets the internal drift adjustment counter.
 * @note since the reset procedure in firmware will stall the device,
 * reset can wait before resuming execution.
 * @param exsid exsid handle
 * @return execution status
 */
int exSID_reset(void * const exsid)
{
	struct _exsid * const xs = exsid;

	if (!xs)
		return -1;

	xsdbg("reset\n");

	xSoutb(xs, XS_AD_IOCTRS, 100);	// this will stall

	xs->clkdrift = 0;

	return 0;
}


/**
 * exSID+ clock selection routine.
 * Selects between PAL, NTSC and 1MHz clocks. Only implemented in exSID+ devices.
 * @note upon clock change the hardware resync itself and resets the SIDs, which
 * takes approximately 50us: this call introduces a stall in the stream
 * and resets the internal drift adjustment counter. Output should be muted before execution.
 * @param exsid exsid handle
 * @param clock clock selector value, see exSID.h.
 * @return execution status
 */
int exSID_clockselect(void * const exsid, int clock)
{
	struct _exsid * const xs = exsid;

	if (!xs)
		return -1;

	xsdbg("clk: %d\n", clock);

	if (XS_MODEL_PLUS != xs->cst->model)
		return -1;

	switch (clock) {
		case XS_CL_PAL:
			xSoutb(xs, XSP_AD_IOCTCP, 100);
			break;
		case XS_CL_NTSC:
			xSoutb(xs, XSP_AD_IOCTCN, 100);
			break;
		case XS_CL_1MHZ:
			xSoutb(xs, XSP_AD_IOCTC1, 100);
			break;
		default:
			return -1;
	}

	xs->clkdrift = 0;	// reset drift

	return 0;
}

/**
 * exSID+ audio operations routine.
 * Selects the audio mixing / muting option. Only implemented in exSID+ devices.
 * @warning all these operations (excepting unmuting obviously) will mute the
 * output by default.
 * @note no accounting for SID cycles consumed (not expected to be used during playback).
 * @param exsid exsid handle
 * @param operation audio operation value, see exSID.h.
 * @return execution status
 */
int exSID_audio_op(void * const exsid, int operation)
{
	struct _exsid * const xs = exsid;

	if (!xs)
		return -1;

	xsdbg("auop: %d\n", operation);

	if (XS_MODEL_PLUS != xs->cst->model)
		return -1;

	switch (operation) {
		case XS_AU_6581_8580:
			xSoutb(xs, XSP_AD_IOCTA0, 0);
			break;
		case XS_AU_8580_6581:
			xSoutb(xs, XSP_AD_IOCTA1, 0);
			break;
		case XS_AU_8580_8580:
			xSoutb(xs, XSP_AD_IOCTA2, 0);
			break;
		case XS_AU_6581_6581:
			xSoutb(xs, XSP_AD_IOCTA3, 0);
			break;
		case XS_AU_MUTE:
			xSoutb(xs, XSP_AD_IOCTAM, 0);
			break;
		case XS_AU_UNMUTE:
			xSoutb(xs, XSP_AD_IOCTAU, 0);
			break;
		default:
			return -1;
	}

	return 0;
}

/**
 * SID chipselect routine.
 * Selects which SID will play the tunes.
 * @note Accounts for elapsed cycles.
 * @param exsid exsid handle
 * @param chip SID selector value, see exSID.h.
 * @return execution status
 */
int exSID_chipselect(void * const exsid, int chip)
{
	struct _exsid * const xs = exsid;

	if (!xs)
		return -1;

	xs->clkdrift -= xs->cst->csioctl_cycles;

	xsdbg("cs: %d\n", chip);

	switch (chip) {
		case XS_CS_CHIP0:
			xSoutb(xs, XS_AD_IOCTS0, 0);
			break;
		case XS_CS_CHIP1:
			xSoutb(xs, XS_AD_IOCTS1, 0);
			break;
		case XS_CS_BOTH:
			xSoutb(xs, XS_AD_IOCTSB, 0);
			break;
		default:
			return -1;
	}

	return 0;
}

/**
 * Device hardware model.
 * Queries the driver for the hardware model currently identified.
 * @param exsid exsid handle
 * @return hardware model as enumerated in exSID.h, negative value on error.
 */
int exSID_hwmodel(void * const exsid)
{
	struct _exsid * const xs = exsid;
	int model;

	if (!xs)
		return -1;

	switch (xs->cst->model) {
		case XS_MODEL_STD:
			model = XS_MD_STD;
			break;
		case XS_MODEL_PLUS:
			model = XS_MD_PLUS;
			break;
		default:
			model = -1;
			break;
	}

	xsdbg("HW model: %d\n", model);

	return model;
}

/**
 * Hardware and firmware version of the device.
 * Returns both in the form of a 16bit integer: MSB is an ASCII
 * character representing the hardware revision (e.g. 0x42 = "B"), and LSB
 * is a number representing the firmware version in decimal integer.
 * Does not reach the hardware (information is fetched once in exSID_init()).
 * @param exsid exsid handle
 * @return version information as described above, or 0xFFFF if error.
 */
uint16_t exSID_hwversion(void * const exsid)
{
	struct _exsid * const xs = exsid;

	if (!xs)
		return 0xFFFF;

	return xs->hwvers;
}

/**
 * Private delay loop.
 * Introduces NOP requests to the device for the sole purpose of elapsing a number of SID clock cycles.
 * @param xs exsid private pointer
 * @param cycles how many SID clocks to loop for.
 */
static inline void xSdelay(struct _exsid * const xs, uint_fast32_t cycles)
{
#ifdef	DEBUG
	xs->accdelay += cycles;
#endif
	while (likely(cycles >= xs->cst->mindel_cycles)) {
		xSoutb(xs, XS_AD_IOCTD1, 0);
		cycles -= xs->cst->mindel_cycles;
		xs->clkdrift -= xs->cst->mindel_cycles;
	}
#ifdef	DEBUG
	xs->accdelay -= cycles;
#endif
}

/**
 * Cycle accurate delay routine.
 * Delay for cycles SID clocks while leaving enough lead time for an I/O operation.
 * @param exsid exsid handle
 * @param cycles how many SID clocks to loop for.
 * @return 0 on success, -1 on error and 1 if the requested number of cycles is smaller than the feasible delay
 */
int exSID_delay(void * const exsid, uint_fast32_t cycles)
{
	struct _exsid * const xs = exsid;
	uint_fast32_t delay;

	if (unlikely(!xs))
		return -1;

	xs->clkdrift += cycles;
#ifdef	DEBUG
	xs->acccycle += cycles;
#endif

	if (unlikely(xs->clkdrift <= xs->cst->write_cycles))	// never delay for less than a full write would need
		return 1;	// too short

	delay = xs->clkdrift - xs->cst->write_cycles;
	xSdelay(xs, delay);

	return 0;
}

/**
 * Private write routine for a tuple address + data.
 * @param xs exsid private pointer
 * @param addr target address to write to.
 * @param data data to write at that address.
 * @param flush if non-zero, force immediate flush to device.
 */
static inline void _exSID_write(struct _exsid * const xs, uint_least8_t addr, uint8_t data, int flush)
{
	xSoutb(xs, (unsigned char)addr, 0);
	xSoutb(xs, (unsigned char)data, flush);
}

/**
 * Timed write routine, attempts cycle-accurate writes.
 * This function will be cycle-accurate provided that no two consecutive writes
 * are less than write_cycles apart and the leftover delay is <= max_adj SID clock cycles.
 * @note this function accepts writes to invalid addresses (> 0x18). If DEBUG is enabled
 * it will not pass them to the hardware and return 1. Elapsed SID cycles will still be accounted for.
 * @param exsid exsid handle
 * @param cycles how many SID clocks to wait before the actual data write.
 * @param addr target address.
 * @param data data to write at that address.
 * @return -1 if error, 0 on success
 */
int exSID_clkdwrite(void * const exsid, uint_fast32_t cycles, uint_least8_t addr, uint8_t data)
{
	struct _exsid * const xs = exsid;
	int adj;

	if (unlikely(!xs))
		return -1;

#ifdef	DEBUG
	if (unlikely(addr > 0x18)) {
		xsdbg("Invalid write: %.2" PRIxLEAST8 "\n", addr);
		exSID_delay(xs, cycles);
		return 1;
	}
#endif

	// actual write will cost write_cycles. Delay for cycles - write_cycles then account for the write
	xs->clkdrift += cycles;
	if (xs->clkdrift > xs->cst->write_cycles)
		xSdelay(xs, xs->clkdrift - xs->cst->write_cycles);

	xs->clkdrift -= xs->cst->write_cycles;	// write is going to consume write_cycles clock ticks

#ifdef	DEBUG
	if (xs->clkdrift >= xs->cst->mindel_cycles)
		xsdbg("Impossible drift adjustment! %" PRIdFAST32 " cycles\n", xs->clkdrift);
	else if (xs->clkdrift < 0)
		xs->accdrift += xs->clkdrift;
#endif

	/* if we are still going to be early, delay actual write by up to max_adj ticks
	At this point it is guaranted that clkdrift will be < mindel_cycles. */
	if (likely(xs->clkdrift >= 0)) {
		adj = xs->clkdrift % (xs->cst->max_adj+1);
		/* if max_adj is >= clkdrift, modulo will give the same results
		   as the correct test:
		   adj = (clkdrift < max_adj ? clkdrift : max_adj)
		   but without an extra conditional branch. If is is < max_adj, then it
		   seems to provide better results by evening jitter accross writes. So
		   it's the preferred solution for all cases. */
		addr = (unsigned char)(addr | (adj << 5));	// final delay encoded in top 3 bits of address
#ifdef	DEBUG
		xs->accdrift += (xs->clkdrift - adj);
#endif
		//xsdbg("drft: %d, adj: %d, addr: %.2hhx, data: %.2hhx\n", clkdrift, adj, (char)(addr | (adj << 5)), data);
	}

#ifdef	DEBUG
	xs->acccycle += cycles;
	xs->accioops++;
#endif

	//xsdbg("delay: %d, clkdrift: %d\n", cycles, xs->clkdrift);
	_exSID_write(xs, addr, data, 0);

	return 0;
}

/**
 * BLOCKING Timed read routine, attempts cycle-accurate reads.
 * The following description is based on exSID (standard).
 * This function will be cycle-accurate provided that no two consecutive reads or writes
 * are less than XS_CYCIO apart and leftover delay is <= max_adj SID clock cycles.
 * Read result will only be available after a full XS_CYCIO, giving clkdread() the same
 * run time as clkdwrite(). There's a 2-cycle negative adjustment in the code because
 * that's the actual offset from the write calls ('/' denotes falling clock edge latch),
 * which the following ASCII tries to illustrate: <br />
 * Write looks like this in firmware:
 * > ...|_/_|...
 * ...end of data byte read | cycle during which write is enacted / next cycle | etc... <br />
 * Read looks like this in firmware:
 * > ...|_|_|_/_|_|...
 * ...end of address byte read | 2 cycles for address processing | cycle during which SID is read /
 *	then half a cycle later the CYCCHR-long data TX starts, cycle completes | another cycle | etc... <br />
 * This explains why reads happen a relative 2-cycle later than then should with
 * respect to writes.
 * @warning this function is only valid if EXSID_THREADED is not defined. If it
 * is called when EXSID_THREADED is defined, no read will be performed, however
 * the requested cycles will still be clocked.
 * @note this function accepts reads from invalid addresses (addr < 0x19 or addr > 0x1c).
 * If DEBUG is enabled it will not pass them to the hardware and return 1. Elapsed SID
 * cycles will still be accounted for.
 * @note The actual time the read will take to complete depends
 * on the USB bus activity and settings. It *should* complete in XSC_USBLAT ms, but
 * not less, meaning that read operations are bound to introduce timing inaccuracy.
 * As such, this function is only really provided as a proof of concept but SHOULD
 * BETTER BE AVOIDED.
 * @param exsid exsid handle
 * @param cycles how many SID clocks to wait before the actual data read.
 * @param addr target address.
 * @param data pointer to store data read from address.
 * @return -1 if error, 0 on success
 */
int exSID_clkdread(void * const exsid, uint_fast32_t cycles, uint_least8_t addr, uint8_t * data)
{
	struct _exsid * const xs = exsid;
#ifndef	EXSID_THREADED
	int adj;

	if (unlikely(!xs || !data))
		return -1;

#ifdef	DEBUG
	if (unlikely((addr < 0x19) || (addr > 0x1C))) {
		xsdbg("Invalid read: %.2" PRIxLEAST8 "\n", addr);
		exSID_delay(xs, cycles);
		return 1;
	}
#endif

	// actual read will happen after read_pre_cycles. Delay for cycles - read_pre_cycles then account for the read
	xs->clkdrift += xs->cst->read_offset_cycles;		// 2-cycle offset adjustement, see function documentation.
	xs->clkdrift += cycles;
	if (xs->clkdrift > xs->cst->read_pre_cycles)
		xSdelay(xs, xs->clkdrift - xs->cst->read_pre_cycles);

	xs->clkdrift -= xs->cst->read_pre_cycles;	// read request is going to consume read_pre_cycles clock ticks

#ifdef	DEBUG
	if (xs->clkdrift > xs->cst->mindel_cycles)
		xsdbg("Impossible drift adjustment! %" PRIdFAST32 " cycles", xs->clkdrift);
	else if (xs->clkdrift < 0) {
		xs->accdrift += xs->clkdrift;
		xsdbg("Late read request! %" PRIdFAST32 " cycles\n", xs->clkdrift);
	}
#endif

	// if we are still going to be early, delay actual read by up to max_adj ticks
	if (likely(xs->clkdrift >= 0)) {
		adj = xs->clkdrift % (xs->cst->max_adj+1);	// see clkdwrite()
		addr = (unsigned char)(addr | (adj << 5));	// final delay encoded in top 3 bits of address
#ifdef	DEBUG
		xs->accdrift += (xs->clkdrift - adj);
#endif
		//xsdbg("drft: %d, adj: %d, addr: %.2hhx, data: %.2hhx\n", clkdrift, adj, (char)(addr | (adj << 5)), data);
	}

#ifdef	DEBUG
	xs->acccycle += cycles;
	xs->accioops++;
#endif

	// after read has completed, at least another read_post_cycles will have been spent
	xs->clkdrift -= xs->cst->read_post_cycles;

	//xsdbg("delay: %d, clkdrift: %d\n", cycles, clkdrift);
	xSoutb(xs, addr, 1);
	xSread(xs, data, 1);		// blocking
	return 0;
#else	// !EXSID_THREADED
	exSID_delay(xs, cycles);
	return -1;
#endif
}
