#ifndef _BASE64_H
#define _BASE64_H

namespace commonutil
{

/*-------------------------------------------------------------
功能：将一块数据编码成BASE64
参数：input ---待编码的数据
input_len --- 待编码的数据块长度。
output --- 用来存放结果的缓冲区
output_siz ---缓冲区output的长度
result_length --- 编码后的数据块长度。
注：编码后的数据长度为：((input_len+2)/3)*4  , output_size >= ((input_len+2)/3)*4+1.
---------------------------------------------------------------*/
bool Base64_Encode(const unsigned char* input, int input_len, char* output, int output_size, int* result_length);


/*-------------------------------------------------------------
功能：将一块BASE64编码的数据解码
参数：input ---待解码的数据
格式：
input_len --- 待解码的数据块长度。
output --- 用来存放解码后数据的缓冲区
output_size ---缓冲区output的长度
result_length --- 解码后的数据块长度。
返回值：0 成功
        -1失败
注：由于每个Group中可能含有1至3字节。 所以解码后的数据长度不一定，
    如果每个Group中都有3个字节数据，则解码后的数据长度为：
	(input_len*3)/4.
---------------------------------------------------------------*/
bool Base64_Decode(const char* input, int input_len, unsigned char* output, int output_size, int* result_length);

}

#endif
