#include <string.h>
#include "buffer.h"

void Buffer::init() {
	size = 512;
	buf = new uint8_t[size];
	len = 0;
}

void Buffer::uninit() {
	size = 0;
	delete[] buf;
	len = 0;
}

void Buffer::expand() {
	uint8_t* tmp = new uint8_t[size * 2];
	memcpy(tmp, buf, size);
	delete[] buf;
	buf = tmp;
	size *= 2;
}

void Buffer::set_len(int len) {
	this->len = len;
}

int Buffer::get_len() {
	return len;
}

uint8_t* Buffer::get_buf() {
	return buf;
}

void Buffer::insert_front(Buffer* buffer) {
	insert_front(buffer->get_buf(), buffer->get_len());
}

void Buffer::insert_front(uint8_t* buf, int len) {
	while (len + this->len > this->size) expand();
	memcpy(this->buf + len, this->buf, this->len);
	memcpy(this->buf, buf, len);
	this->len += len;
}

void Buffer::insert_back(Buffer* buffer) {
	insert_back(buffer->get_buf(), buffer->get_len());
}

void Buffer::insert_back(uint8_t* buf, int len) {
	while (len + this->len > this->size) expand();
	memcpy(this->buf + this->len, buf, len);
	this->len += len;
}

void Buffer::copy(Buffer* buffer) {
	copy(buffer->get_buf(), buffer->get_len());
}

void Buffer::copy(uint8_t* buf, int len) {
	while (len > this->size) expand();
	memcpy(this->buf, buf, len);
	this->len = len;
}

void Buffer::alloc(int len) {
	while (len > this->size) expand();
}

void Buffer::clear() {
	this->len = 0;
}