#include <commonutil/Base64.h>

#define XX 127
/*
 * Table for decoding hexadecimal in quoted-printable
 */
static const unsigned char index_hex[256] = {
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	 0, 1, 2, 3,  4, 5, 6, 7,  8, 9,XX,XX, XX,XX,XX,XX,
	XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX
};
#define HEXCHAR(c)  (index_hex[(unsigned char)(c)])

/*
 * Table for decoding base64
 */
static const unsigned char index_64[256] = {
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
	52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
	XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
	15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
	XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
	41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
	XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX
};
#define CHAR64(c)  (index_64[(unsigned char)(c)])

static const char base64_char[64] = {
	'A','B','C','D',  'E','F','G','H',
	'I','J','K','L',  'M','N','O','P',
	'Q','R','S','T',  'U','V','W','X',
	'Y','Z','a','b',  'c','d','e','f',
	'g','h','i','j',  'k','l','m','n',
	'o','p','q','r',  's','t','u','v',
	'w','x','y','z',  '0','1','2','3',
	'4','5','6','7',  '8','9','+','/'
};


void Base64EncodeGroup(const unsigned char *input, int grpLen, char* output) {
	if (grpLen == 3) {
		output[0] = base64_char[ input[0]>>2 ];
		output[1] = base64_char[ ((input[0]&0x03)<<4) + (input[1]>>4) ];
		output[2] = base64_char[ ((input[1]&0x0f)<<2) + (input[2]>>6) ];
		output[3] = base64_char[ input[2]&0x3f ];
	} else if (grpLen == 2) {
		output[0] = base64_char[ input[0]>>2 ];
		output[1] = base64_char[ ((input[0]&0x03)<<4) + (input[1]>>4) ];
		output[2] = base64_char[ ((input[1]&0x0f)<<2) ];
		output[3] = '=';
	} else if (grpLen == 1)	{
		output[0] = base64_char[ input[0]>>2 ];
		output[1] = base64_char[ ((input[0]&0x03)<<4) ];
		output[2] = '=';
		output[3] = '=';
	}
}


void Base64DecodeGroup(const char input[4], unsigned char output[3], int* pWrite) {	
	if (input[2] == '=' && input[3] == '=') {
		//this means there 1 byte in the last group.
		output[0] = CHAR64(input[0])<<2;
		output[0] |= CHAR64(input[1])>>4;
		*pWrite = 1;
	} else if (input[3] == '=') {
		//this means there 2 byte in the last group.
		output[0] = CHAR64(input[0])<<2;
		output[0] |= CHAR64(input[1])>>4;
		output[1] = CHAR64(input[1])<<4;
		output[1] |= CHAR64(input[2])>>2;
		*pWrite = 2;
	} else {
		//this means there 3 byte in the last group.
		output[0] = CHAR64(input[0])<<2;
		output[0] |= CHAR64(input[1])>>4;
		output[1] = CHAR64(input[1])<<4;
		output[1] |= CHAR64(input[2])>>2;
		output[2] = CHAR64(input[2])<<6;
		output[2] |= CHAR64(input[3]);
		*pWrite = 3;
	}
}

bool commonutil::Base64_Encode(const unsigned char* input, int input_len, char* output, int output_size, int* result_length)
{
	int encode_length;
	int grpCount, i;

	if (!output) //output buffer is invalid.
		return false;

	encode_length = ((input_len+2)/3)*4;
	if (output_size < encode_length+1)
		return false;
	*result_length = encode_length;
	grpCount = (input_len+2)/3;
	for (i = 0; i < grpCount; i++) {
		if (i == grpCount-1)
			Base64EncodeGroup(input+i*3, input_len-i*3, output+i*4);
		else
			Base64EncodeGroup(input+i*3, 3, output+i*4);
	}
	output[encode_length] = 0;
	return true;
}


bool commonutil::Base64_Decode(const char* input, int input_len, unsigned char* output, int output_size, int* result_length)
{
	int i,count;
	int bad_data;
	int written;

	if (!output)   //output buffer is invalid.
		return false;

	if ((input_len%4) != 0)  //the data to be decoding must be 4-bytes align.
		return false;

	*result_length = count = 0;
	while (count < input_len) {
		//Output buffer is insufficient.
		if (*result_length >= output_size)
			return false;

		//Check the input data at first.
		bad_data = 0;
		//for the last group '=' is permitted.
		if (input[count+2] == '=' && input[count+3] == '=') { //this group contain only 1 byte.
			if (CHAR64(input[count]) == XX || CHAR64(input[count+1]) == XX)
				bad_data = 1;
		} else if (input[count+3] == '=') {//this group contain 2 bytes.
			if (CHAR64(input[count]) == XX 
				|| CHAR64(input[count+1]) == XX 
				|| CHAR64(input[count+2]) == XX)
				bad_data = 1;
		} else {
			//this group contain 3 bytes.
			for (i = 0; i < 4 && !bad_data; i++) {
				if (CHAR64(input[count+i]) == XX || input[count+i] == '=')
					bad_data = 1;
			}
		}
		if (bad_data)
			return false;

		//Source data is correct, decode this group.
		Base64DecodeGroup(input+count, (unsigned char*)output + *result_length, &written);
		*result_length += written;
		count += 4;
	}
	return true;
}
