#include <stdio.h>
#include <stdlib.h>
#include <openssl/ecdh.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

struct derivedKey {
    char* secret;
    int length;
};

typedef struct derivedKey derivedKey;

// function prototypes
EVP_PKEY* generateKey();
EVP_PKEY* extractPublicKey(EVP_PKEY *privateKey);
derivedKey* deriveShared(EVP_PKEY *publicKey, EVP_PKEY *privateKey);

int main()
{
    // Generate Key pairs for Alice And Bob, Using the NIST named curve primve256v1 for generation
    EVP_PKEY *alicePrivateKey = generateKey();
    EVP_PKEY *alicePubKey = extractPublicKey(alicePrivateKey);
   

    unsigned char *pk = NULL;
    
    int size = i2d_PUBKEY(alicePubKey, &pk);
    printf("\n Size: %d bytes\n\n DER Public Key: ", size);
    for (size_t i = 0; i < size; i++){
        printf("%02x ", pk[i]);
    }
    printf("\n");
    
    EVP_PKEY *alicePub = d2i_PUBKEY( NULL, (const unsigned char **) &pk, size);
    
    //d2i_PUBKEY(&alicePub, (const unsigned char **) &pk, size);

    if ( alicePub == NULL){
        printf("Erro\n");
    }
    
    
    
    
    //exit(0);
    EVP_PKEY *bobPrivateKey = generateKey();
    EVP_PKEY *bobPubKey = extractPublicKey(bobPrivateKey);

    // Extract the public key from the private key of Alice and Bob,
    // So that Alice can be given Bob's public key and Bob can be given Alice's.
    // Using ECDH, Alice and Bob will then compute a shared secret, which will be same

    // Here we give to Alice, Bob's public key and Alice computes the shared secret using her private key.
    derivedKey* secretAlice = deriveShared(bobPubKey, alicePrivateKey);
    
    // Here we give to Bob, Alice's public key and Bob computes the shared secret using his private key.
    derivedKey* secretBob = deriveShared(alicePub, bobPrivateKey);
    
   

    printf("\n Secret computed by Alice :\n");
    
    for (size_t i = 0; i < secretAlice->length; i++){
        printf(" %02x", (unsigned char)secretAlice->secret[i]);
    }
    printf("\n");


    printf("\n Secret computed by Bob : \n");

    for (size_t i = 0; i < secretBob->length; i++){
        printf(" %02x", (unsigned char)secretBob->secret[i]);
    }
    printf("\n\n");

    return 0;
}

void handleErrors(){
    printf("\n\nFailed...");
}

void handleDerivationErrors(int x){
    printf("\n\nDerivation Failed...");
    printf("%d", x);
}

/**
    Generates a key pair and returns it
*/
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

/**
    Takes in a private key and extracts the public key from it.
*/
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

/**
    Takes in the private key and peer public key and spits out the derived shared secret.
*/
derivedKey* deriveShared(EVP_PKEY *publicKey, EVP_PKEY *privateKey){
    derivedKey *dk = (derivedKey *)malloc(sizeof(derivedKey));
    EVP_PKEY_CTX *derivationCtx = NULL;

    derivationCtx = EVP_PKEY_CTX_new(privateKey, NULL);

    EVP_PKEY_derive_init(derivationCtx);
    EVP_PKEY_derive_set_peer(derivationCtx, publicKey);

	if(EVP_PKEY_derive(derivationCtx, NULL, (size_t*) &dk->length)){
        if((dk->secret = (char *)OPENSSL_malloc(dk->length)) != NULL){
            if((EVP_PKEY_derive(derivationCtx, (unsigned char *)dk->secret, (size_t*) &dk->length))){
                EVP_PKEY_CTX_free(derivationCtx);
                return dk;
            }
        }
    }
	return NULL;
}