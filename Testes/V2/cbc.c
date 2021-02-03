#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/aes.h>
#include <time.h>

unsigned int timeR;

unsigned int randomN(unsigned int n){
    timeR++;
    srand(timeR);
    return rand() % n;
}
 
/* AES key for Encryption and Decryption */
const static unsigned char aes_key[] = {0xfd, 0x43, 0x0a, 0xfc, 0x65, 0xde, 0xc6, 0x81,
										0x5e, 0xa9, 0x8c, 0xb6, 0x3a, 0x23, 0xa8, 0x4b,
										0x0f, 0x72, 0x23, 0xa0, 0x06, 0xc7, 0x91, 0x21, 
										0xe7, 0x0a, 0x10, 0xbc, 0x7f, 0x3f, 0xbe, 0x1b};

/* Print Encrypted and Decrypted data packets */
void print_data(int b, const char *tittle, const void* data, int len);
 
int main( )
{
	timeR = time(NULL);
	/* Input data to encrypt */
	unsigned char aes_input[] = "vamos arrebentar com esta merda toda";
	
	/* Init vector */
	unsigned char iv[AES_BLOCK_SIZE];
	// unsigned int n[4];
	
	// for (size_t i = 0; i < 4; i++ ){
	// 	n[i] = randomN(4294967295);
	// 	memcpy(&iv[i*4], &n[i], 4);
	// }
	// print_data(1,"\n IV       ", iv, 16);
	memset(iv, 0x00, AES_BLOCK_SIZE);
	
	/* Buffers for Encryption and Decryption */
	int s = sizeof(aes_input) + 1;
	unsigned char enc_out[s];
	unsigned char dec_out[sizeof(aes_input)];
	
	/* AES-128 bit CBC Encryption */
	AES_KEY enc_key, dec_key;
	AES_set_encrypt_key(aes_key, sizeof(aes_key)*8, &enc_key);
	AES_cbc_encrypt(aes_input, enc_out, s, &enc_key, iv, AES_ENCRYPT);
	/* AES-128 bit CBC Decryption */
	// for (size_t i = 0; i < 4; i++ ){
	// 	memcpy(&iv[i*4], &n[i], 4);
	// }
	// print_data(1,"\n IV       ", iv, 16);
	memset(iv, 0x00, AES_BLOCK_SIZE); // don't forget to set iv vector again, else you can't decrypt data properly
	AES_set_decrypt_key(aes_key, sizeof(aes_key)*8, &dec_key); // Size of key is in bits
	AES_cbc_encrypt(enc_out, dec_out, sizeof(aes_input), &dec_key, iv, AES_DECRYPT);
	
	/* Printing and Verifying */
	print_data(0,"\n Original ",aes_input, sizeof(aes_input)); // you can not print data as a string, because after Encryption its not ASCII
	
	print_data(1,"\n Encrypted",enc_out, sizeof(enc_out));
	
	print_data(0,"\n Decrypted",dec_out, sizeof(dec_out));
	
	return 0;
}
 
void print_data(int b, const char *tittle, const void* data, int len){
	printf("%s : ",tittle);
	const unsigned char * p = (const unsigned char*)data;
	int i = 0;
	if (b){
		for (; i<len; ++i)
			printf("%02X ", *p++);
	}else{
		for (; i<len; ++i)
			printf("%c  ", *p++);
	}
	
	printf("\n");
}

//gcc cbc.c -lssl -lcrypto