#ifndef __BBCP_SHA256_openssl_H__
#define __BBCP_SHA256_openssl_H__
/******************************************************************************/
/*                                                                            */
/*                         b b c p _ S H A 2 5 6 . h                          */
/*                                                                            */
/******************************************************************************/

#include <string.h>
#include "bbcp_Headers.h"
#include "bbcp_ChkSum.h"
#include <openssl/evp.h>

class bbcp_SHA256_openssl : public bbcp_ChkSum
{
public:

char *csCurr(char **Text=0)
           {EVP_MD_CTX *currCTX = EVP_MD_CTX_new();
            unsigned int mdLen = sizeof(MyDigest);
            if (!currCTX) throw "EVP_MD_CTX_new() failed";
            if (!EVP_MD_CTX_copy_ex(currCTX, MyContext)
            ||  !EVP_DigestFinal_ex(currCTX, MyDigest, &mdLen))
               {EVP_MD_CTX_free(currCTX);
                throw "EVP sha256 final failed";
               }
            EVP_MD_CTX_free(currCTX);
            if (Text) *Text = x2a((char *)MyDigest);
            return (char *)MyDigest;
           }

int   csSize() {return sizeof(MyDigest);}

char *Final(char **Text=0)
           {unsigned int mdLen = sizeof(MyDigest);
            if (!EVP_DigestFinal_ex(MyContext, MyDigest, &mdLen))
               throw "EVP sha256 final failed";
            if (Text) *Text = x2a((char *)MyDigest);
            return (char *)MyDigest;
           }

void  Init() {if (!EVP_DigestInit_ex(MyContext, EVP_sha256(), 0))
                 throw "EVP sha256 init failed";
             }

const char *Type() {return "sha256";}

void  Update(const char *Buff, int BLen)
          {if (!EVP_DigestUpdate(MyContext, Buff, (size_t)BLen))
              throw "EVP sha256 update failed";
          }

      bbcp_SHA256_openssl() {if (!(MyContext = EVP_MD_CTX_new()))
                                throw "EVP_MD_CTX_new() failed";
                             Init();
                            }
     ~bbcp_SHA256_openssl() {if (MyContext) EVP_MD_CTX_free(MyContext);}

private:

enum {csLen = 32};

EVP_MD_CTX   *MyContext;
unsigned char MyDigest[csLen];
};
#endif
