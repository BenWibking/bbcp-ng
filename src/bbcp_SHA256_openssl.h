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
#include <openssl/sha.h>

class bbcp_SHA256_openssl : public bbcp_ChkSum
{
public:

char *csCurr(char **Text=0)
           {SHA256_CTX currCTX = MyContext;
            SHA256_Final(MyDigest, &currCTX);
            if (Text) *Text = x2a((char *)MyDigest);
            return (char *)MyDigest;
           }

int   csSize() {return sizeof(MyDigest);}

char *Final(char **Text=0)
           {SHA256_Final(MyDigest, &MyContext);
            if (Text) *Text = x2a((char *)MyDigest);
            return (char *)MyDigest;
           }

void  Init() {SHA256_Init(&MyContext);}

const char *Type() {return "sha256";}

void  Update(const char *Buff, int BLen)
          {SHA256_Update(&MyContext, (unsigned char *)Buff, (unsigned)BLen);}

      bbcp_SHA256_openssl() {SHA256_Init(&MyContext);}
     ~bbcp_SHA256_openssl() {}

private:

SHA256_CTX    MyContext;
unsigned char MyDigest[SHA256_DIGEST_LENGTH];
};
#endif
