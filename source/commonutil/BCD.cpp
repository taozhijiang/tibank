#include <string.h>
#include <commonutil/BCD.h>

int commonutil::BCD_Encode(const unsigned char *source, int sourceLength, 
		char *dest, int maxDestLength, int *resultLength) {
	int i, pos, g, l;
	int result = 0;
	const static char *ctable = "0123456789ABCDEF";

	if (resultLength == NULL || maxDestLength <= 0
		|| sourceLength <= 0 || source == NULL || dest == NULL) {
		return -1;
	}
	dest[0] = 0;
	*resultLength = 0;
	pos = 0;
	i = 0;

	while (i<sourceLength) {
		g = source[i] / 16;
		l = source[i] % 16;
		if (pos<maxDestLength-2) {
			dest[pos] = ctable[g];
			dest[pos+1] = ctable[l];
			pos += 2;
		} else {
			result = -2;
			break;
		}
		i++;
	}
	dest[pos] = 0;
	*resultLength = pos;
	return result;
}

int commonutil::BCD_Encode_LowChar(const unsigned char *source, int sourceLength, 
		char *dest, int maxDestLength, int *resultLength) {
	int i, pos, g, l;
	int result = 0;
	const static char *ctable = "0123456789abcdef";

	if (resultLength == NULL || maxDestLength <= 0
		|| sourceLength <= 0 || source == NULL || dest == NULL) {
		return -1;
	}
	dest[0] = 0;
	*resultLength = 0;
	pos = 0;
	i = 0;

	while (i<sourceLength) {
		g = source[i] / 16;
		l = source[i] % 16;
		if (pos<maxDestLength-2) {
			dest[pos] = ctable[g];
			dest[pos+1] = ctable[l];
			pos += 2;
		} else {
			result = -2;
			break;
		}
		i++;
	}
	dest[pos] = 0;
	*resultLength = pos;
	return result;
}

int commonutil::BCD_Decode(const char *source, int sourceLength, 
		unsigned char *dest, int maxDestLength, int *resultLength) {
	int i, pos;
	int g, l;
	int result = 0;

	if (resultLength == NULL || maxDestLength <= 0
		|| sourceLength <= 0 || source == NULL || dest == NULL) {
		return -1;
	}
	*resultLength = 0;
	pos = 0;
	i = 0;
	
	while (i < sourceLength && source[i]) {
		char c1 = source[i];
		char c2 = source[i+1];

		g = l = 0;
		if (c1 >= 'A' && c1 <= 'F') {
			g = 10 + (c1 - 'A');
		} else if (c1 >= 'a' && c1 <= 'f') {
			g = 10 + (c1 - 'a');
		} else if (c1 >= '0' && c1 <= '9') {
			g = c1 - '0';
		} else{
			result = -3;
			break;
		}

		if (c2 >= 'A' && c2 <= 'F') {
			l = 10 + (c2 - 'A');
		} else if (c2 >= 'a' && c2 <= 'f') {
			l = 10 + (c2 - 'a');
		} else if (c2 >= '0' && c2 <= '9') {
			l = c2 - '0';
		} else {
			result = -4;
			break;
		}

		if (pos<maxDestLength) {
			dest[pos] = (unsigned char)(g*16+l);
			pos ++;
		} else {
			result = -5;
			break;
		}
		i += 2;
	}
	*resultLength = pos;
	return result;
}
