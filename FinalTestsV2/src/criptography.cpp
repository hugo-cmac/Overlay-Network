#include "criptography.h"

namespace std{

    EVP_PKEY* Criptography::generateKey(){
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

    EVP_PKEY* Criptography::extractPublicKey(EVP_PKEY *privateKey){
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
    
    Criptography::Criptography(){
        privateKey = generateKey();
        publicKey = extractPublicKey(privateKey);
    };

    unsigned char* Criptography::getPublicKey(){
        unsigned char* pk = NULL;
        publicKeySize = i2d_PUBKEY(publicKey, &pk);
        return pk;
    }

    int Criptography::deriveShared(unsigned char *publicKey){
        EVP_PKEY_CTX *derivationCtx = NULL;
    
        EVP_PKEY *pub = d2i_PUBKEY( NULL, (const unsigned char **) &publicKey, publicKeySize);
        derivationCtx = EVP_PKEY_CTX_new(privateKey, NULL);
        EVP_PKEY_derive_init(derivationCtx);
        EVP_PKEY_derive_set_peer(derivationCtx, pub);
        if(EVP_PKEY_derive(derivationCtx, NULL, &sharedKeySize)){
            if((skey = (unsigned char*) OPENSSL_malloc(sharedKeySize)) != NULL){
                if((EVP_PKEY_derive(derivationCtx, skey, &sharedKeySize))){
                    AES_set_encrypt_key(skey, sharedKeySize*8, &ekey);
                    AES_set_decrypt_key(skey, sharedKeySize*8, &dkey);
                    EVP_PKEY_CTX_free(derivationCtx);
                    return 1;
                }
            }
        }
        EVP_PKEY_CTX_free(derivationCtx);
        return 0;
    }

    unsigned char* Criptography::getSharedKey(){
        return skey;
    }
  
    void Criptography::encript(unsigned char* pt, int len){
        unsigned char ct[len];
        unsigned char iv[AES_BLOCK_SIZE];
        memset(iv, 0, AES_BLOCK_SIZE);
        AES_cbc_encrypt(pt, ct, len, &ekey, iv, AES_ENCRYPT);
        memcpy(pt, ct, len);
    }

    void Criptography::decript(unsigned char* ct, int len){
        unsigned char pt[len];
        unsigned char iv[AES_BLOCK_SIZE];
        memset(iv, 0, AES_BLOCK_SIZE);
        AES_cbc_encrypt(ct, pt, len, &dkey, iv, AES_DECRYPT);
        memcpy(ct, pt, len);
    }
}