#if !defined(_BCD_H_)
#define _BCD_H_

namespace commonutil
{

//dest will end by '\0', maxDestLength include the '\0'
int BCD_Encode(const unsigned char *source, int sourceLength, char *dest, int maxDestLength, int *resultLength);

//dest will end by '\0', maxDestLength include the '\0'
int BCD_Encode_LowChar(const unsigned char *source, int sourceLength, char *dest, int maxDestLength, int *resultLength);

//dest will not end by '\0', because the dest is binary
int BCD_Decode(const char *source, int sourceLength, unsigned char *dest, int maxDestLength, int *resultLength);

}

#endif
