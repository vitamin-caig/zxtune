#include "ByteArray.h"
#include "../include/debug.h"
#include "../include/endianness.h"
#include <stdlib.h>

void ByteArray_defaults(struct ByteArray* self) {
	memset(self, 0, sizeof(*self));
}

void ByteArray_ctor(struct ByteArray* self) {
	ByteArray_defaults(self);
	// original constructor code goes here
	self->readMultiByte = ByteArray_readMultiByte;
	self->readByte = ByteArray_readByte;
	self->readUnsignedByte = ByteArray_readUnsignedByte;
	self->readShort = ByteArray_readShort;
	self->readUnsignedShort = ByteArray_readUnsignedShort;
	self->readInt = ByteArray_readInt;
	self->readUnsignedInt = ByteArray_readUnsignedInt;
	self->readBytes = ByteArray_readBytes;
	
	self->writeInt = ByteArray_writeInt;
	self->writeUnsignedInt = ByteArray_writeUnsignedInt;
	self->writeShort = ByteArray_writeShort;
	self->writeUnsignedShort = ByteArray_writeUnsignedShort;
	self->writeByte = ByteArray_writeByte;
	self->writeUnsignedByte = ByteArray_writeUnsignedByte;;
	self->writeMem = ByteArray_writeMem;
	self->writeUTFBytes = ByteArray_writeUTFBytes;
	self->writeBytes = ByteArray_writeBytes;
	self->writeFloat = ByteArray_writeFloat;
	
	self->set_position = ByteArray_set_position;
	self->set_position_rel = ByteArray_set_position_rel;
	self->get_position = ByteArray_get_position;
	
	self->bytesAvailable = ByteArray_bytesAvailable;
	
#ifdef IS_LITTLE_ENDIAN
	self->sys_endian = BAE_LITTLE;
#else
	self->sys_endian = BAE_BIG;
#endif
}

struct ByteArray* ByteArray_new(void) {
	struct ByteArray* self = (struct ByteArray*) malloc(sizeof(*self));
	if(self) ByteArray_ctor(self);
	return self;
}

void ByteArray_set_endian(struct ByteArray* self, enum ByteArray_Endianess endian) {
	self->endian = endian;
}

enum ByteArray_Endianess ByteArray_get_endian(struct ByteArray* self) {
	return self->endian;
}

// a real byte array clears the mem and resets
// "len" and pos to 0
// where len is equivalent to the bytes written into it
void ByteArray_clear(struct ByteArray* self) {
	fprintf(stderr, "clear called\n");
	assert_op(self->type, ==, BAT_MEMSTREAM);
	assert_op(self->start_addr, !=, 0);
	memset(self->start_addr, 0, self->size);
}

off_t ByteArray_get_position(struct ByteArray* self) {
	return self->pos;
}

static void seek_error() {
	perror("seek error!\n");
	assert_dbg(0);
}

static void neg_off() {
	fprintf(stderr, "negative seek attempted\n");
	assert_dbg(0);
}

static void oob() {
	fprintf(stderr, "oob access attempted\n");
	assert_dbg(0);
}

void ByteArray_set_length(struct ByteArray* self, off_t len) {
	if(len > self->size) {
		oob();
		return;
	}
	self->size = len;
}

off_t ByteArray_get_length(struct ByteArray* self) {
	return self->size;
}

int ByteArray_set_position_rel(struct ByteArray* self, int rel) {
	if((int) self->pos + rel < 0) {
		neg_off();
		rel = -self->pos;
	}
	return ByteArray_set_position(self, self->pos + rel);
}

int ByteArray_set_position(struct ByteArray* self, off_t pos) {
	if(pos == self->pos) return 1;
	if(pos > self->size) {
		oob();
		return 0;
	}
		
	if(self->type == BAT_FILESTREAM) {
		off_t ret = lseek(self->fd, pos, SEEK_SET);
		if(ret == (off_t) -1) {
			seek_error();
			return 0;
		}
	}
	self->pos = pos;
	return 1;
}

static void read_error() {
	perror("read error!\n");
	assert_dbg(0);
}

static void read_error_short() {
	perror("read error (short)!\n");
	assert_dbg(0);
}

int ByteArray_open_file(struct ByteArray* self, const char* filename) {
	struct stat st;
	self->type = BAT_FILESTREAM;
	self->pos = 0;
	self->size = 0;
	if(stat(filename, &st) == -1) return 0;
	self->size = st.st_size;
	self->fd = open(filename, O_RDONLY);
	return (self->fd != -1);
}

void ByteArray_close_file(struct ByteArray *self) {
	close(self->fd);
	self->fd = -1;
}

int ByteArray_open_mem(struct ByteArray* self, char* data, size_t size) {
	self->pos = 0;
	self->size = size;
	self->type = BAT_MEMSTREAM;
	self->start_addr = data;
	return 1;
}

void ByteArray_readMultiByte(struct ByteArray* self, char* buffer, size_t len) {
	if(self->type == BAT_MEMSTREAM) {
		assert_op(self->start_addr, !=, 0);
		assert_op((off_t) (self->pos + len), <=, self->size);
		memcpy(buffer, &self->start_addr[self->pos], len);
	} else {
		ssize_t ret = read(self->fd, buffer, len);
		if(ret == -1) {
			read_error();
			return;
		} else if(ret != (ssize_t) len) {
			read_error_short();
			self->pos += len;
			return;
		}
	}
	self->pos += len;
}

// write contents of self into what
// if len == 0 all available bytes are used.
// self->pos is considered the start offset to use for self.
// the position in dest will not be advanced.
// the position in source will be advanced len bytes.
// returns the number of bytes written
off_t ByteArray_readBytes(struct ByteArray* self, struct ByteArray *dest, off_t start, off_t len) {
	off_t left = self->size - self->pos;
	if(len == 0) len = left;
	else if(len > left) {
		oob();
		len = left;
	}
	if(len == 0) return 0;
	else if (len > start + dest->size) {
		oob();
		len = start + dest->size;
		if(len == 0) return 0;
	}
	if(dest->type != BAT_MEMSTREAM) {
		assert_dbg(0);
	}
	self->readMultiByte(self, &dest->start_addr[start], len);
	return len;
}

off_t ByteArray_bytesAvailable(struct ByteArray* self) {
	if(self->pos < self->size) return self->size - self->pos;
	return 0;
}

unsigned int ByteArray_readUnsignedInt(struct ByteArray* self) {
	union {
		unsigned int intval;
		unsigned char charval[sizeof(unsigned int)];
	} buf;
	self->readMultiByte(self, (char*) buf.charval, 4);
	if(self->endian != self->sys_endian) {
		buf.intval = byteswap32(buf.intval);
	}
	return buf.intval;
}

int ByteArray_readInt(struct ByteArray* self) {
	union {
		unsigned int intval;
		unsigned char charval[sizeof(unsigned int)];
	} buf;
	self->readMultiByte(self, (char*) buf.charval, 4);
	if(self->endian != self->sys_endian) {
		buf.intval = byteswap32(buf.intval);
	}
	return buf.intval;
}

unsigned short ByteArray_readUnsignedShort(struct ByteArray* self) {
	union {
		unsigned short intval;
		unsigned char charval[sizeof(unsigned short)];
	} buf;
	self->readMultiByte(self, (char*) buf.charval, 2);
	if(self->endian != self->sys_endian) {
		buf.intval = byteswap16(buf.intval);
	}
	return buf.intval;
}

short ByteArray_readShort(struct ByteArray* self) {
	union {
		unsigned short intval;
		unsigned char charval[sizeof(unsigned short)];
	} buf;
	self->readMultiByte(self, (char*) buf.charval, 2);
	if(self->endian != self->sys_endian) {
		buf.intval = byteswap16(buf.intval);
	}
	return buf.intval;
}

unsigned char ByteArray_readUnsignedByte(struct ByteArray* self) {
	union {
		unsigned char intval;
	} buf;
	self->readMultiByte(self, (char*) &buf.intval, 1);
	return buf.intval;
}

signed char ByteArray_readByte(struct ByteArray* self) {
	union {
		signed char intval;
	} buf;
	self->readMultiByte(self, (char*) &buf.intval, 1);
	return buf.intval;
}

/* equivalent to foo = self[x]; (pos stays unchanged) */
unsigned char ByteArray_getUnsignedByte(struct ByteArray* self, off_t index) {
	//assert_op(self->type, ==, BAT_MEMSTREAM);
	assert_op(index, <, self->size);
	if(self->type == BAT_MEMSTREAM) {
		assert_op(self->start_addr, !=, 0);
		return (self->start_addr[index]);
	} else {
		off_t save = self->pos;
		unsigned char res;
		ByteArray_set_position(self, index);
		res = ByteArray_readUnsignedByte(self);
		ByteArray_set_position(self, save);
		return res;
	}
}

/* equivalent to self[x] = what (pos stays unchanged) */
void ByteArray_setUnsignedByte(struct ByteArray* self, off_t index, unsigned char what) {
	off_t save = self->pos;
	if(ByteArray_set_position(self, index)) {
		ByteArray_writeUnsignedByte(self, what);
		self->pos = save;
	}
}

off_t ByteArray_writeByte(struct ByteArray* self, signed char what) {
	return ByteArray_writeMem(self, (unsigned char*) &what, 1);
}

off_t ByteArray_writeUnsignedByte(struct ByteArray* self, unsigned char what) {
	return ByteArray_writeMem(self, (unsigned char*) &what, 1);
}

off_t ByteArray_writeShort(struct ByteArray* self, signed short what) {
	union {
		short intval;
		unsigned char charval[sizeof(what)];
	} u;
	u.intval = what;
	if(self->sys_endian != self->endian) {
		u.intval = byteswap16(u.intval);
	}
	return ByteArray_writeMem(self, u.charval, sizeof(what));
}

off_t ByteArray_writeUnsignedShort(struct ByteArray* self, unsigned short what) {
	union {
		unsigned short intval;
		unsigned char charval[sizeof(what)];
	} u;
	u.intval = what;
	if(self->sys_endian != self->endian) {
		u.intval = byteswap16(u.intval);
	}
	return ByteArray_writeMem(self, u.charval, sizeof(what));
}

off_t ByteArray_writeInt(struct ByteArray* self, signed int what) {
	union {
		int intval;
		unsigned char charval[sizeof(what)];
	} u;
	u.intval = what;
	if(self->sys_endian != self->endian) {
		u.intval = byteswap32(u.intval);
	}
	return ByteArray_writeMem(self, u.charval, sizeof(what));
}

off_t ByteArray_writeUnsignedInt(struct ByteArray* self, unsigned int what) {
	union {
		unsigned int intval;
		unsigned char charval[sizeof(what)];
	} u;
	u.intval = what;
	if(self->sys_endian != self->endian) {
		u.intval = byteswap32(u.intval);
	}
	return ByteArray_writeMem(self, u.charval, sizeof(what));
}

off_t ByteArray_writeMem(struct ByteArray* self, unsigned char* what, size_t len) {
	if(self->type == BAT_FILESTREAM) {
		fprintf(stderr, "tried to write to file!\n");
		assert_dbg(0);
		return 0;
	}
	if((off_t)(self->pos + len) > self->size) {
		fprintf(stderr, "oob write attempted");
		assert_dbg(0);
		return 0;
	}
	assert_op(self->start_addr, !=, 0);

	memcpy(&self->start_addr[self->pos], what, len);
	self->pos += len;
	return len;
}

off_t ByteArray_writeUTFBytes(struct ByteArray* self, char* what) {
	return ByteArray_writeMem(self, (unsigned char*) what, strlen(what));
}

// write contents of what into self
off_t ByteArray_writeBytes(struct ByteArray* self, struct ByteArray* what) {
	if(what->type == BAT_FILESTREAM) {
		fprintf(stderr, "tried to write from non-memory stream\n");
		assert_dbg(0);
		return 0;
	} else {
		return ByteArray_writeMem(self, (unsigned char*) &what->start_addr[what->pos], what->size - what->pos);
	}
}

off_t ByteArray_writeFloat(struct ByteArray* self, float what) {
	union {
		float floatval;
		unsigned int intval;
		unsigned char charval[sizeof(what)];
	} u;
	u.floatval = what;
	if(self->sys_endian != self->endian) {
		u.intval = byteswap32(u.intval);
	}
	return ByteArray_writeMem(self, u.charval, sizeof(what));
}

void ByteArray_dump_to_file(struct ByteArray* self, char* filename) {
	assert_op(self->type, ==, BAT_MEMSTREAM);
	int fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	write(fd, self->start_addr, self->size);
	close(fd);
}


