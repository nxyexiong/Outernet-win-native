#include <sodium.h>
#include <string.h>
#include "crypto.h"
#include "buffer.h"

int Crypto::init(const char* secret) {
	if (sodium_init() == -1)
		return -1;

	int len = strlen(secret);
	sha256(key, (unsigned char *)secret, len);
	return 0;
}

void Crypto::encrypt(Buffer* output, Buffer* input) {
	output->alloc(input->get_len() + 8); // 8 bytes of nonce
	randombytes_buf(output->get_buf(), 8);
	crypto_stream_chacha20_xor(output->get_buf() + 8, input->get_buf(), input->get_len(), output->get_buf(), key);
	output->set_len(input->get_len() + 8);
}

void Crypto::decrypt(Buffer* output, Buffer* input) {
	output->alloc(input->get_len() - 8);
	crypto_stream_chacha20_xor(output->get_buf(), input->get_buf() + 8, input->get_len() - 8, input->get_buf(), key);
	output->set_len(input->get_len() - 8);
}

void Crypto::sha256(uint8_t* output, uint8_t * input, int inlen) {
	crypto_hash_sha256(output, input, inlen);
}