#include <memory.h>
#include <commonutil/Sha1.h>

using namespace commonutil;
/*
  Define the SHA1 circular left shift macro
*/

#define SHA1CircularShift(bits,word) \
		(((word) << (bits)) | ((word) >> (32-(bits))))

/* Local Function Prototyptes */
static void SHA1PadMessage(SHA1_CONTEXT*);
static void SHA1ProcessMessageBlock(SHA1_CONTEXT*);


/*
  Initialize SHA1Context

  SYNOPSIS
    sha1_reset()
    context [in/out]		The context to reset.

 DESCRIPTION
   This function will initialize the SHA1Context in preparation
   for computing a new SHA1 message digest.

 RETURN
   SHA_SUCCESS		ok
   != SHA_SUCCESS	sha Error Code.
*/


const unsigned int sha_const_key[5] = {
	0x67452301,
	0xEFCDAB89,
	0x98BADCFE,
	0x10325476,
	0xC3D2E1F0
};


int commonutil::sha1_reset(SHA1_CONTEXT *context) {
	if (!context)
		return SHA_NULL;

	context->Length = 0;
	context->Message_Block_Index = 0;

	context->Intermediate_Hash[0] = sha_const_key[0];
	context->Intermediate_Hash[1] = sha_const_key[1];
	context->Intermediate_Hash[2] = sha_const_key[2];
	context->Intermediate_Hash[3] = sha_const_key[3];
	context->Intermediate_Hash[4] = sha_const_key[4];

	context->Computed = 0;
	context->Corrupted = 0;

	return SHA_SUCCESS;
}


/*
   Return the 160-bit message digest into the array provided by the caller

  SYNOPSIS
    sha1_result()
    context [in/out]		The context to use to calculate the SHA-1 hash.
    Message_Digest: [out]	Where the digest is returned.

  DESCRIPTION
    NOTE: The first octet of hash is stored in the 0th element,
	  the last octet of hash in the 19th element.

 RETURN
   SHA_SUCCESS		ok
   != SHA_SUCCESS	sha Error Code.
*/

int commonutil::sha1_result(SHA1_CONTEXT *context, unsigned char Message_Digest[SHA1_HASH_SIZE]) {
	int i;

	if (!context || !Message_Digest)
		return SHA_NULL;

	if (context->Corrupted)
		return context->Corrupted;

	if (!context->Computed)	{
		SHA1PadMessage(context);
		/* message may be sensitive, clear it out */
		memset((char*) context->Message_Block,0,64);
		context->Length = 0;    /* and clear length  */
		context->Computed = 1;
	}

	for (i = 0; i < SHA1_HASH_SIZE; i++)
		Message_Digest[i] = (unsigned char)((context->Intermediate_Hash[i>>2] >> 8 * (3 - (i & 0x03))));
	
	return SHA_SUCCESS;
}


/*
  Accepts an array of octets as the next portion of the message.

  SYNOPSIS
   sha1_input()
   context [in/out]	The SHA context to update
   message_array	An array of characters representing the next portion
			of the message.
  length		The length of the message in message_array

 RETURN
   SHA_SUCCESS		ok
   != SHA_SUCCESS	sha Error Code.
*/

int commonutil::sha1_input(SHA1_CONTEXT *context, const unsigned char *message_array, unsigned length) {
	if (!length)
		return SHA_SUCCESS;

	/* We assume client konows what it is doing in non-debug mode */
	if (!context || !message_array)
		return SHA_NULL;
	if (context->Computed)
		return (context->Corrupted= SHA_STATE_ERROR);
	if (context->Corrupted)
		return context->Corrupted;

	while (length--) {
		context->Message_Block[context->Message_Block_Index++]=	(*message_array & 0xFF);
		context->Length  += 8;  /* Length is in bits */

		/*
		Then we're not debugging we assume we never will get message longer
		2^64 bits.
		*/
		if (context->Length == 0)
			return (context->Corrupted= 1);	   /* Message is too long */

		if (context->Message_Block_Index == 64)
			SHA1ProcessMessageBlock(context);
		
		message_array++;
	}
	return SHA_SUCCESS;
}


/*
  Process the next 512 bits of the message stored in the Message_Block array.

  SYNOPSIS
    SHA1ProcessMessageBlock()

   DESCRIPTION
     Many of the variable names in this code, especially the single
     character names, were used because those were the names used in
     the publication.
*/

/* Constants defined in SHA-1	*/
static const unsigned int K[] = {
	0x5A827999,
	0x6ED9EBA1,
	0x8F1BBCDC,
	0xCA62C1D6
};


static void SHA1ProcessMessageBlock(SHA1_CONTEXT *context) {
	int t;		   /* Loop counter		  */
	unsigned int temp;		   /* Temporary word value	  */
	unsigned int W[80];		   /* Word sequence		  */
	unsigned int A, B, C, D, E;	   /* Word buffers		  */
	int index;

	/*
	Initialize the first 16 words in the array W
	*/
	for (t = 0; t < 16; t++) {
		index=t*4;
		W[t] = context->Message_Block[index] << 24;
		W[t] |= context->Message_Block[index + 1] << 16;
		W[t] |= context->Message_Block[index + 2] << 8;
		W[t] |= context->Message_Block[index + 3];
	}

	for (t = 16; t < 80; t++) {
		W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
	}

	A = context->Intermediate_Hash[0];
	B = context->Intermediate_Hash[1];
	C = context->Intermediate_Hash[2];
	D = context->Intermediate_Hash[3];
	E = context->Intermediate_Hash[4];

	for (t = 0; t < 20; t++) {
		temp= SHA1CircularShift(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	for (t = 20; t < 40; t++) {
		temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	for (t = 40; t < 60; t++) {
		temp= (SHA1CircularShift(5,A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2]);
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	for (t = 60; t < 80; t++) {
		temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	context->Intermediate_Hash[0] += A;
	context->Intermediate_Hash[1] += B;
	context->Intermediate_Hash[2] += C;
	context->Intermediate_Hash[3] += D;
	context->Intermediate_Hash[4] += E;

	context->Message_Block_Index = 0;
}


/*
  Pad message

  SYNOPSIS
    SHA1PadMessage()
    context: [in/out]		The context to pad

  DESCRIPTION
    According to the standard, the message must be padded to an even
    512 bits.  The first padding bit must be a '1'. The last 64 bits
    represent the length of the original message.  All bits in between
    should be 0.  This function will pad the message according to
    those rules by filling the Message_Block array accordingly.  It
    will also call the ProcessMessageBlock function provided
    appropriately. When it returns, it can be assumed that the message
    digest has been computed.

*/

void SHA1PadMessage(SHA1_CONTEXT *context) {
	/*
	Check to see if the current message block is too small to hold
	the initial padding bits and length.  If so, we will pad the
	block, process it, and then continue padding into a second
	block.
	*/

	int i=context->Message_Block_Index;

	if (i > 55) {
		context->Message_Block[i++] = 0x80;
		memset((char*) &context->Message_Block[i],0,
		sizeof(context->Message_Block[0])*(64-i));
		context->Message_Block_Index=64;

		/* This function sets context->Message_Block_Index to zero	*/
		SHA1ProcessMessageBlock(context);

		memset((char*) &context->Message_Block[0],0,
		sizeof(context->Message_Block[0])*56);
		context->Message_Block_Index=56;
	} else {
		context->Message_Block[i++] = 0x80;
		memset((char*) &context->Message_Block[i],0,
		sizeof(context->Message_Block[0])*(56-i));
		context->Message_Block_Index=56;
	}

	/*
	Store the message length as the last 8 octets
	*/

	context->Message_Block[56] = (unsigned char) (context->Length >> 56);
	context->Message_Block[57] = (unsigned char) (context->Length >> 48);
	context->Message_Block[58] = (unsigned char) (context->Length >> 40);
	context->Message_Block[59] = (unsigned char) (context->Length >> 32);
	context->Message_Block[60] = (unsigned char) (context->Length >> 24);
	context->Message_Block[61] = (unsigned char) (context->Length >> 16);
	context->Message_Block[62] = (unsigned char) (context->Length >> 8);
	context->Message_Block[63] = (unsigned char) (context->Length);

	SHA1ProcessMessageBlock(context);
}
