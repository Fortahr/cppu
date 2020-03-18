#pragma once

#include <string_view>

namespace bear
{
	using namespace bear;
	#include <bearssl.h>
}

namespace cppu
{
	namespace net
	{
		class TLSListener;

		struct TLSKey
		{
			friend class TLSListener;

		private:
			uint keyType;
			bear::br_rsa_private_key rsaKey;
			bear::br_ec_private_key ecKey;

			bool set_cert_private_key(const std::string_view& certificate)
			{
				bear::br_skey_decoder_context dc;
				bear::br_skey_decoder_init(&dc);
				bear::br_skey_decoder_push(&dc, certificate.data(), certificate.size());
				int err = bear::br_skey_decoder_last_error(&dc);
				if (err != 0)
					return false;

				const bear::br_rsa_private_key* rk = nullptr;
				const bear::br_ec_private_key* ek = nullptr;
				keyType = bear::br_skey_decoder_key_type(&dc);
				switch (keyType)
				{
				case BR_KEYTYPE_RSA:
					rk = bear::br_skey_decoder_get_rsa(&dc);
					rsaKey.p = (uint8*)malloc(rk->plen);
					rsaKey.q = (uint8*)malloc(rk->qlen);
					rsaKey.dp = (uint8*)malloc(rk->dplen);
					rsaKey.dq = (uint8*)malloc(rk->dqlen);
					rsaKey.iq = (uint8*)malloc(rk->iqlen);

					if (!rsaKey.p || !rsaKey.q || !rsaKey.dp || !rsaKey.dq || !rsaKey.iq)
					{
						free(rsaKey.p);
						free(rsaKey.q);
						free(rsaKey.dp);
						free(rsaKey.dq);
						free(rsaKey.iq);
						memset(&rsaKey, 0, sizeof(rsaKey));

						return false;
					}

					rsaKey.n_bitlen = rk->n_bitlen;
					memcpy(rsaKey.p, rk->p, rk->plen);
					rsaKey.plen = rk->plen;
					memcpy(rsaKey.q, rk->q, rk->qlen);
					rsaKey.qlen = rk->qlen;
					memcpy(rsaKey.dp, rk->dp, rk->dplen);
					rsaKey.dplen = rk->dplen;
					memcpy(rsaKey.dq, rk->dq, rk->dqlen);
					rsaKey.dqlen = rk->dqlen;
					memcpy(rsaKey.iq, rk->iq, rk->iqlen);
					rsaKey.iqlen = rk->iqlen;
					break;

				case BR_KEYTYPE_EC:
					ek = bear::br_skey_decoder_get_ec(&dc);
					ecKey.curve = ek->curve;
					ecKey.x = (uint8_t*)malloc(ek->xlen);

					if (!ecKey.x)
					{
						free(ecKey.x);
						memset(&ecKey, 0, sizeof(ecKey));

						return false;
					}

					memcpy(ecKey.x, ek->x, ek->xlen);
					ecKey.xlen = ek->xlen;
					break;

				default:
					return false;
					break;
				}

				return true;
			}

			static std::string parse_single_base64_pem_certificate(std::string_view certificates)
			{
				const char* beginOfCertificate = "-----BEGIN PRIVATE KEY-----";
				const char* endOfCertificate = "-----END PRIVATE KEY-----";

				certificates.remove_prefix(certificates.find(beginOfCertificate) + strlen(beginOfCertificate));
				certificates.remove_prefix((certificates[0] < 32) + (certificates[1] < 32));
				std::size_t endCertificate = certificates.find(endOfCertificate);
				endCertificate -= (certificates[endCertificate - 1] < 32) + (certificates[endCertificate - 2] < 32);

				if (endCertificate != std::string::npos)
				{
					std::string decoded;
					decoded.resize(crypt::base64_dec_len(endCertificate));
					std::size_t decodedLength = crypt::base64_dec_raw((byte*)decoded.data(), nullptr, certificates.substr(0, endCertificate), nullptr);

					decoded.resize(decodedLength);
					decoded.shrink_to_fit();

					return decoded;
				}
				else
					return "";
			}

		public:
			TLSKey()
			{
				memset(&rsaKey, 0, sizeof(rsaKey));
				memset(&ecKey, 0, sizeof(ecKey));
			}

			TLSKey(std::string_view data)
			{
				memset(&rsaKey, 0, sizeof(rsaKey));
				memset(&ecKey, 0, sizeof(ecKey));

				std::string decoded = std::move(parse_single_base64_pem_certificate(data));
				if (!decoded.empty())
					set_cert_private_key(decoded);
			}

			~TLSKey()
			{
				//free(data);
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
	using namespace cppu;
#endif