#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <time.h>
#include <unistd.h>

#include <openssl/aes.h>
#include <openssl/ecdh.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

using namespace std;

class Criptography{
    private:
        AES_KEY ekey;
        AES_KEY dkey;

        unsigned char* skey = NULL;
        //size_t skeylen = 0;

        EVP_PKEY *privateKey = NULL;
        EVP_PKEY *publicKey = NULL;
        

        EVP_PKEY* generateKey(){
            EVP_PKEY_CTX *paramGenCtx = NULL, *keyGenCtx = NULL;
            EVP_PKEY *params= NULL, *keyPair= NULL;

            paramGenCtx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);

            if(EVP_PKEY_paramgen_init(paramGenCtx)){
                EVP_PKEY_CTX_set_ec_paramgen_curve_nid(paramGenCtx, NID_X9_62_prime256v1);
                EVP_PKEY_paramgen(paramGenCtx, &params);

                keyGenCtx = EVP_PKEY_CTX_new(params, NULL);

                if(EVP_PKEY_keygen_init(keyGenCtx)){
                    if(EVP_PKEY_keygen(keyGenCtx, &keyPair)){
                       
                        EVP_PKEY_CTX_free(paramGenCtx);
                        EVP_PKEY_CTX_free(keyGenCtx);
                        return keyPair;
                    }
                } 
            }
            EVP_PKEY_CTX_free(paramGenCtx);
            EVP_PKEY_CTX_free(keyGenCtx);
            return NULL;
        }

        EVP_PKEY* extractPublicKey(EVP_PKEY *privateKey){
            EC_KEY* ecKey = EVP_PKEY_get1_EC_KEY(privateKey);
            EC_POINT* ecPoint = (EC_POINT*) EC_KEY_get0_public_key(ecKey);

            EVP_PKEY *publicKey = EVP_PKEY_new();

            EC_KEY *pubEcKey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

            EC_KEY_set_public_key(pubEcKey, ecPoint);

            EVP_PKEY_set1_EC_KEY(publicKey, pubEcKey);

            EC_KEY_free(ecKey);
            EC_POINT_free(ecPoint);

            return publicKey;
        }

    public:
        size_t skeylen = 0;
        int pksize = 0;

        Criptography(){
            privateKey = generateKey();
            publicKey = extractPublicKey(privateKey);
        }
        //return pk pointer
        unsigned char* getPublicKey(){
            unsigned char* pk = NULL;
            pksize = i2d_PUBKEY(publicKey, &pk);
            return pk;
        }

        int deriveShared(unsigned char *publicKey){
            EVP_PKEY_CTX *derivationCtx = NULL;
        
            EVP_PKEY *pub = d2i_PUBKEY( NULL, (const unsigned char **) &publicKey, pksize);

            derivationCtx = EVP_PKEY_CTX_new(privateKey, NULL);

            EVP_PKEY_derive_init(derivationCtx);
            EVP_PKEY_derive_set_peer(derivationCtx, pub);

            if(EVP_PKEY_derive(derivationCtx, NULL, &skeylen)){
                if((skey = (unsigned char*) OPENSSL_malloc(skeylen)) != NULL){
                    if((EVP_PKEY_derive(derivationCtx, skey, &skeylen))){
                        AES_set_encrypt_key(skey, skeylen*8, &ekey);
                        AES_set_decrypt_key(skey, skeylen*8, &dkey);
                        free(skey);
                        EVP_PKEY_CTX_free(derivationCtx);
                        return 1;
                    }
                }
            }
            EVP_PKEY_CTX_free(derivationCtx);
            return 0;
        }

        //pt +1 e len +1
        void encript(unsigned char* pt, int len){
            unsigned char ct[len];
            unsigned char iv[AES_BLOCK_SIZE];
		    memset(iv, 0, AES_BLOCK_SIZE);

            AES_cbc_encrypt(pt, ct, len, &ekey, iv, AES_ENCRYPT);
            memcpy(pt, ct, len);
        }

        //pt +1 e len +1
        void decript(unsigned char* ct, int len){
            unsigned char pt[len];
            unsigned char iv[AES_BLOCK_SIZE];
		    memset(iv, 0, AES_BLOCK_SIZE);

            AES_cbc_encrypt(ct, pt, len, &dkey, iv, AES_DECRYPT);
            memcpy(ct, pt, len);
        }
    
    };


int main(int argc, char const *argv[]){
    
    Criptography* alice = new Criptography();
    Criptography* bob = new Criptography();
    
    unsigned char* pkBOB = bob->getPublicKey();
    unsigned char* pkALICE = alice->getPublicKey();
    

    alice->deriveShared(pkBOB);
    memset(pkBOB, 0, bob->pksize);
    printf("%d\n", bob->pksize);
    bob->deriveShared(pkALICE);

    

    // unsigned char* sakey = alice->getSharedKey();
    // unsigned char* sbkey = bob->getSharedKey();

    // for (size_t i = 0; i < alice->skeylen; i++){
    //     printf("%02x ", sakey[i]);
    // }
    // puts(" ");
    // for (size_t i = 0; i < alice->skeylen; i++){
    //     printf("|| ");
    // }
    // puts(" ");
    // for (size_t i = 0; i < bob->skeylen; i++){
    //     printf("%02x ", sbkey[i]);
    // }
    // puts(" ");
    // puts(" ");
    unsigned char pt[16]  = {'h','u','r','r','o',' ','b','u','g','o',' ','o','l','a','a','a'};
    // 16 + 16 = 32
    // 32 + 16 = 48
    // 
    for (size_t i = 0; i < sizeof(pt); i++){
        printf("%02x ", pt[i]);
    }
    puts(" ");

    alice->encript(pt, sizeof(pt));

    for (size_t i = 0; i < sizeof(pt); i++){
        printf("%02x ", pt[i]);
    }
    puts(" ");

    bob->decript(pt, sizeof(pt));

    for (size_t i = 0; i < sizeof(pt); i++){
        printf("%02x ", pt[i]);
    }
    puts(" ");
    cout << pt << endl;

    return 0;
}
