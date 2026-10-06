#pragma once
#include <openssl/hmac.h>
#define MBEDTLS_MD_SHA256 1
inline const EVP_MD *mbedtls_md_info_from_type(int){return EVP_sha256();}
inline int mbedtls_md_hmac(const EVP_MD *md,const unsigned char *key,size_t n,const unsigned char *data,size_t length,unsigned char *out){
 unsigned int written=0;return HMAC(md,key,(int)n,data,length,out,&written)&&written==32?0:-1;
}
