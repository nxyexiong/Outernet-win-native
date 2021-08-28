#pragma once
#include <stdint.h>

class Buffer;

class Crypto {
	uint8_t key[32];
public:
	static Crypto& get_instance()
	{
		static Crypto instance;
		return instance;
	}
	int init(const char* secret);
	void encrypt(Buffer* output, Buffer* input);
	void decrypt(Buffer* output, Buffer* input);
	void sha256(uint8_t* output, uint8_t * input, int inlen);
};