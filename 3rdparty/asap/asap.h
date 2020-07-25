/* Generated automatically with "cito". Do not edit. */
#ifndef _ASAP_H_
#define _ASAP_H_
typedef int cibool;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct ASAP ASAP;
typedef struct ASAPInfo ASAPInfo;

/**
 * Format of output samples.
 */
typedef enum {
	/**
	 * Unsigned 8-bit.
	 */
	ASAPSampleFormat_U8,
	/**
	 * Signed 16-bit little-endian.
	 */
	ASAPSampleFormat_S16_L_E,
	/**
	 * Signed 16-bit big-endian.
	 */
	ASAPSampleFormat_S16_B_E
}
ASAPSampleFormat;

typedef struct 
{
	void *obj;
	void (*func)(void *obj, int data);
}
ByteWriter;

typedef struct 
{
	void *obj;
	void (*func)(void *obj, const char *s);
}
StringConsumer;

ASAP *ASAP_New(void);
void ASAP_Delete(ASAP *self);

/**
 * Enables silence detection.
 * Causes playback to stop after the specified period of silence.
 * @param seconds Length of silence which ends playback. Zero disables silence detection.
 */
void ASAP_DetectSilence(ASAP *self, int seconds);

/**
 * Fills the specified buffer with generated samples.
 * @param buffer The destination buffer.
 * @param bufferLen Number of bytes to fill.
 * @param format Format of samples.
 */
int ASAP_Generate(ASAP *self, unsigned char *buffer, int bufferLen, ASAPSampleFormat format);

/**
 * Returns current playback position in blocks.
 * A block is one sample or a pair of samples for stereo.
 */
int ASAP_GetBlocksPlayed(ASAP const *self);

/**
 * Returns information about the loaded module.
 */
ASAPInfo const *ASAP_GetInfo(ASAP const *self);

/**
 * Returns POKEY channel volume - an integer between 0 and 15.
 * @param channel POKEY channel number (from 0 to 7).
 */
int ASAP_GetPokeyChannelVolume(ASAP const *self, int channel);

/**
 * Returns current playback position in milliseconds.
 */
int ASAP_GetPosition(ASAP const *self);

/**
 * Fills leading bytes of the specified buffer with WAV file header.
 * Returns the number of changed bytes.
 * @param buffer The destination buffer.
 * @param format Format of samples.
 * @param metadata Include metadata (title, author, date).
 */
int ASAP_GetWavHeader(ASAP const *self, unsigned char *buffer, ASAPSampleFormat format, cibool metadata);

/**
 * Loads music data ("module").
 * @param filename Filename, used to determine the format.
 * @param module Contents of the file.
 * @param moduleLen Length of the file.
 */
cibool ASAP_Load(ASAP *self, const char *filename, unsigned char const *module, int moduleLen);

/**
 * Mutes the selected POKEY channels.
 * @param mask An 8-bit mask which selects POKEY channels to be muted.
 */
void ASAP_MutePokeyChannels(ASAP *self, int mask);

/**
 * Prepares playback of the specified song of the loaded module.
 * @param song Zero-based song index.
 * @param duration Playback time in milliseconds, -1 means infinity.
 */
cibool ASAP_PlaySong(ASAP *self, int song, int duration);

/**
 * Output sample rate.
 */
#define ASAP_SAMPLE_RATE  44100

/**
 * Changes the playback position.
 * @param position The requested absolute position in milliseconds.
 */
cibool ASAP_Seek(ASAP *self, int position);

/**
 * Changes the playback position.
 * @param block The requested absolute position in samples (always 44100 per second, even in stereo).
 */
cibool ASAP_SeekSample(ASAP *self, int block);

ASAPInfo *ASAPInfo_New(void);
void ASAPInfo_Delete(ASAPInfo *self);

/**
 * Short license notice.
 * Display after the credits.
 */
#define ASAPInfo_COPYRIGHT  "This program is free software; you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published\nby the Free Software Foundation; either version 2 of the License,\nor (at your option) any later version."

/**
 * Short credits for ASAP.
 */
#define ASAPInfo_CREDITS  "Another Slight Atari Player (C) 2005-2014 Piotr Fusik\nCMC, MPT, TMC, TM2 players (C) 1994-2005 Marcin Lewandowski\nRMT player (C) 2002-2005 Radek Sterba\nDLT player (C) 2009 Marek Konopka\nCMS player (C) 1999 David Spilka\nFC player (C) 2011 Jerzy Kut\n"

/**
 * Returns author's name.
 * A nickname may be included in parentheses after the real name.
 * Multiple authors are separated with <code>" &amp; "</code>.
 * An empty string means the author is unknown.
 */
const char *ASAPInfo_GetAuthor(ASAPInfo const *self);

/**
 * Returns 1 for mono or 2 for stereo.
 */
int ASAPInfo_GetChannels(ASAPInfo const *self);

int ASAPInfo_GetCovoxAddress(ASAPInfo const *self);

/**
 * Returns music creation date.
 * Some of the possible formats are:
 * <ul>
 * <li>YYYY</li>
 * <li>MM/YYYY</li>
 * <li>DD/MM/YYYY</li>
 * <li>YYYY-YYYY</li>
 * </ul>
 * An empty string means the date is unknown.
 */
const char *ASAPInfo_GetDate(ASAPInfo const *self);

/**
 * Returns day of month of the music creation date.
 * -1 means the day is unknown.
 */
int ASAPInfo_GetDayOfMonth(ASAPInfo const *self);

/**
 * Returns 0-based index of the "main" song.
 * The specified song should be played by default.
 */
int ASAPInfo_GetDefaultSong(ASAPInfo const *self);

/**
 * Returns length of the specified song.
 * The length is specified in milliseconds. -1 means the length is indeterminate.
 */
int ASAPInfo_GetDuration(ASAPInfo const *self, int song);

/**
 * Returns human-readable description of the filename extension.
 * @param ext Filename extension without the leading dot.
 */
const char *ASAPInfo_GetExtDescription(const char *ext);

int ASAPInfo_GetInitAddress(ASAPInfo const *self);

const char *ASAPInfo_GetInstrumentName(ASAPInfo const *self, unsigned char const *module, int moduleLen, int i);

/**
 * Returns information whether the specified song loops.
 * Returns:
 * <ul>
 * <li><code>true</code> if the song loops</li>
 * <li><code>false</code> if the song stops</li>
 * </ul>
 * 
 */
cibool ASAPInfo_GetLoop(ASAPInfo const *self, int song);

/**
 * Returns music creation month (1-12).
 * -1 means the month is unknown.
 */
int ASAPInfo_GetMonth(ASAPInfo const *self);

int ASAPInfo_GetMusicAddress(ASAPInfo const *self);

/**
 * Returns the extension of the original module format.
 * For native modules it simply returns their extension.
 * For the SAP format it attempts to detect the original module format.
 * @param module Contents of the file.
 * @param moduleLen Length of the file.
 */
const char *ASAPInfo_GetOriginalModuleExt(ASAPInfo const *self, unsigned char const *module, int moduleLen);

int ASAPInfo_GetPlayerAddress(ASAPInfo const *self);

int ASAPInfo_GetPlayerRateHz(ASAPInfo const *self);

int ASAPInfo_GetPlayerRateScanlines(ASAPInfo const *self);

int ASAPInfo_GetSapHeaderLength(ASAPInfo const *self);

/**
 * Returns number of songs in the file.
 */
int ASAPInfo_GetSongs(ASAPInfo const *self);

/**
 * Returns music title.
 * An empty string means the title is unknown.
 */
const char *ASAPInfo_GetTitle(ASAPInfo const *self);

/**
 * Returns music title or filename.
 * If title is unknown returns filename without the path or extension.
 */
const char *ASAPInfo_GetTitleOrFilename(ASAPInfo const *self);

int ASAPInfo_GetTypeLetter(ASAPInfo const *self);

/**
 * Returns music creation year.
 * -1 means the year is unknown.
 */
int ASAPInfo_GetYear(ASAPInfo const *self);

/**
 * Returns <code>true</code> for NTSC song and <code>false</code> for PAL song.
 */
cibool ASAPInfo_IsNtsc(ASAPInfo const *self);

/**
 * Checks whether the filename extension represents a module type supported by ASAP.
 * Returns <code>true</code> if the filename extension is supported by ASAP.
 * @param ext Filename extension without the leading dot.
 */
cibool ASAPInfo_IsOurExt(const char *ext);

/**
 * Checks whether the filename represents a module type supported by ASAP.
 * Returns <code>true</code> if the filename is supported by ASAP.
 * @param filename Filename to check the extension of.
 */
cibool ASAPInfo_IsOurFile(const char *filename);

/**
 * Loads file information.
 * @param filename Filename, used to determine the format.
 * @param module Contents of the file.
 * @param moduleLen Length of the file.
 */
cibool ASAPInfo_Load(ASAPInfo *self, const char *filename, unsigned char const *module, int moduleLen);

/**
 * Maximum length of a supported input file.
 * You may assume that files longer than this are not supported by ASAP.
 */
#define ASAPInfo_MAX_MODULE_LENGTH  65000

/**
 * Maximum number of songs in a file.
 */
#define ASAPInfo_MAX_SONGS  32

/**
 * Maximum length of text metadata.
 */
#define ASAPInfo_MAX_TEXT_LENGTH  127

/**
 * Returns the number of milliseconds represented by the given string.
 * @param s Time in the <code>"mm:ss.xxx"</code> format.
 */
int ASAPInfo_ParseDuration(const char *s);

/**
 * Sets author's name.
 * A nickname may be included in parentheses after the real name.
 * Multiple authors are separated with <code>" &amp; "</code>.
 * An empty string means the author is unknown.
 */
cibool ASAPInfo_SetAuthor(ASAPInfo *self, const char *value);

/**
 * Sets music creation date.
 * Some of the possible formats are:
 * <ul>
 * <li>YYYY</li>
 * <li>MM/YYYY</li>
 * <li>DD/MM/YYYY</li>
 * <li>YYYY-YYYY</li>
 * </ul>
 * An empty string means the date is unknown.
 */
cibool ASAPInfo_SetDate(ASAPInfo *self, const char *value);

/**
 * Sets the 0-based index of the "main" song.
 */
cibool ASAPInfo_SetDefaultSong(ASAPInfo *self, int song);

/**
 * Sets length of the specified song.
 * The length is specified in milliseconds. -1 means the length is indeterminate.
 */
cibool ASAPInfo_SetDuration(ASAPInfo *self, int song, int duration);

/**
 * Sets information whether the specified song loops.
 * Use:
 * <ul>
 * <li><code>true</code> if the song loops</li>
 * <li><code>false</code> if the song stops</li>
 * </ul>
 * 
 */
cibool ASAPInfo_SetLoop(ASAPInfo *self, int song, cibool loop);

/**
 * Causes music to be relocated.
 * Use only with <code>ASAPWriter.Write</code>.
 */
cibool ASAPInfo_SetMusicAddress(ASAPInfo *self, int address);

/**
 * Sets music title.
 * An empty string means the title is unknown.
 */
cibool ASAPInfo_SetTitle(ASAPInfo *self, const char *value);

/**
 * ASAP version as a string.
 */
#define ASAPInfo_VERSION  "3.2.0"

/**
 * ASAP version - major part.
 */
#define ASAPInfo_VERSION_MAJOR  3

/**
 * ASAP version - micro part.
 */
#define ASAPInfo_VERSION_MICRO  0

/**
 * ASAP version - minor part.
 */
#define ASAPInfo_VERSION_MINOR  2

/**
 * Years ASAP was created in.
 */
#define ASAPInfo_YEARS  "2005-2014"

/**
 * Writes text representation of the given duration.
 * Returns the number of bytes written to <code>result</code>.
 * @param result The output buffer.
 * @param value Number of milliseconds.
 */
int ASAPWriter_DurationToString(unsigned char *result, int value);

/**
 * Enumerates possible file types the given module can be written as.
 * @param output Receives filename extensions without the leading dot.
 * @param info File information.
 * @param module Contents of the file.
 * @param moduleLen Length of the file.
 */
void ASAPWriter_EnumSaveExts(StringConsumer output, ASAPInfo const *info, unsigned char const *module, int moduleLen);

/**
 * Maximum length of text representation of a duration.
 * Corresponds to the longest format which is <code>"mm:ss.xxx"</code>.
 */
#define ASAPWriter_MAX_DURATION_LENGTH  9

/**
 * Writes the given module in a possibly different file format.
 * @param targetFilename Output filename, used to determine the format.
 * @param w Receives output file contents.
 * @param info File information got from the source file with data updated for the output file.
 * @param module Contents of the source file.
 * @param moduleLen Length of the source file.
 * @param tag Display information (xex output only).
 */
cibool ASAPWriter_Write(const char *targetFilename, ByteWriter w, ASAPInfo const *info, unsigned char const *module, int moduleLen, cibool tag);

#ifdef __cplusplus
}
#endif
#endif
