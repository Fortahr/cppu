#include "../dtypes.h"

#include <array>
#include <vector>
#include <string_view>

namespace cppu
{
	namespace crypt
	{
		static const char encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		static const uint8 decode[128] = {
		#define __ 128 // invalid
		#define PA 129 // padding
			//x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xA  xB  xC  xD  xE  xF
			  __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 00-0F */
			  __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 10-1F */
			  __, __, __, __, __, __, __, __, __, __, __, 62, __, 62, __, 63, /* 20-2F */ // yes, there are two 62s and 63s
			  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, __, __, __, PA, __, __, /* 30-3F */
			  __,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 40-4F */
			  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, __, __, __, __, 63, /* 50-5F */
			  __, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 60-6F */
			  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, __, __, __, __, __, /* 70-7F */
		};

		inline size_t base64_dec_len(size_t len)
		{
			return (len + 3) / 4 * 3;
		}

		inline size_t base64_enc_len(size_t len)
		{
			return (len + 2) / 3 * 4;
		}

		//'out' must be 3 bytes; returns number of bytes actually written, 0 if input is invalid
		inline size_t base64_dec_core(uint8* out, const char* in)
		{
			uint8 c1 = in[0];
			uint8 c2 = in[1];
			uint8 c3 = in[2];
			uint8 c4 = in[3];

			if ((c1 | c2 | c3 | c4) >= 0x80)
				return 0;

			c1 = decode[c1];
			c2 = decode[c2];
			c3 = decode[c3];
			c4 = decode[c4];

			size_t ret = 3;
			if ((c1 | c2 | c3 | c4) >= 0x80)
			{
				if (c4 == PA)
				{
					ret = 2;
					c4 = 0;
					if (c3 == PA)
					{
						ret = 1;
						c3 = 0;
					}
				}

				if ((c1 | c2 | c3 | c4) >= 0x80)
					return 0;
			}

			//TODO: reject padded data where the bits of the partial byte aren't zero
			uint32 outraw = (c1 << 18) | (c2 << 12) | (c3 << 6) | (c4 << 0);

			out[0] = outraw >> 16;
			out[1] = outraw >> 8;
			out[2] = outraw >> 0;

			return ret;
		}

		inline bool l_isspace(char ch)
		{
			return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
		}

		inline size_t base64_dec_raw(byte* out, size_t* outend, std::string_view text, size_t* textend)
		{
			size_t outat = 0;
			const char* ptr = (char*)text.data();
			const char* ptrend = ptr + text.size();

		again:
			if (ptr < ptrend - 4)
			{
				size_t n = base64_dec_core(out + outat, ptr);
				if (n != 3)
					goto slowpath;

				ptr += 4;
				outat += 3;

				goto again;
			}
			else
			{
			slowpath:;
				char cs[4];

				int i = 0;
				while (i < 4)
				{
					if (ptr == ptrend)
						goto finish;

					char ch = *(ptr++);
					if (ch >= 0x21)
					{
						cs[i++] = ch;
						continue;
					}
					else
					{
						if (l_isspace(ch))
							continue;

						goto finish;
					}
				}

				size_t n = base64_dec_core(out + outat, cs);
				outat += n;
				if (n != 3)
					goto finish;

				goto again;
			}

		finish:
			while (ptr < ptrend && l_isspace(*ptr))
				ptr++;

			if (outend)
				*outend = outat;

			if (textend)
				*textend = ptrend - ptr;

			if (ptr == ptrend)
				return outat;

			return 0;
		}

		inline std::vector<byte> base64_dec(std::string_view text)
		{
			std::vector<byte> ret;
			ret.resize(base64_dec_len(text.size()));
			size_t actual = base64_dec_raw(ret.data(), NULL, text, NULL);
			ret.resize(actual);
			ret.shrink_to_fit();
			return ret;
		}

		inline void base64_enc_raw(byte* out, std::string_view bytes)
		{
			uint8* outp = out;
			const uint8* inp = (uint8*)bytes.data();
			const uint8* inpe = inp + bytes.size();

			while (inp + 3 <= inpe)
			{
				uint32_t three = inp[0] << 16 | inp[1] << 8 | inp[2];

				*(outp++) = encode[(three >> 18) & 63];
				*(outp++) = encode[(three >> 12) & 63];
				*(outp++) = encode[(three >> 6) & 63];
				*(outp++) = encode[(three >> 0) & 63];
				inp += 3;
			}

			if (inp + 0 == inpe) {}
			if (inp + 1 == inpe)
			{
				uint32_t three = inp[0] << 16;

				*(outp++) = encode[(three >> 18) & 63];
				*(outp++) = encode[(three >> 12) & 63];
				*(outp++) = '=';
				*(outp++) = '=';
			}
			if (inp + 2 == inpe)
			{
				uint32_t three = inp[0] << 16 | inp[1] << 8;

				*(outp++) = encode[(three >> 18) & 63];
				*(outp++) = encode[(three >> 12) & 63];
				*(outp++) = encode[(three >> 6) & 63];
				*(outp++) = '=';
			}
		}

		inline std::vector<byte> base64_enc(std::string_view bytes)
		{
			std::vector<byte> ret(base64_enc_len(bytes.size()));
			base64_enc_raw(ret.data(), bytes);
			return ret;
		}
	}
}