#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "../crypt/base64.h"

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

		struct TLSCertificate
		{
			friend class TLSListener;
		private:
			enum parsed_cert_type
			{
				FAILED = 0,
				NON_CA = 1,
				IS_CA = 2
			};

			bear::br_x509_certificate data;

			static std::vector<bear::br_x509_trust_anchor>& trustAnchor() { static std::vector<bear::br_x509_trust_anchor> v; return v; }

			static void free_ta_contents(const std::vector<bear::br_x509_trust_anchor>::const_iterator& it)
			{
				free(it->dn.data);
				if (it->pkey.key_type == BR_KEYTYPE_RSA)
				{
					free(it->pkey.key.rsa.n);
					free(it->pkey.key.rsa.e);
				}
				else if (it->pkey.key_type == BR_KEYTYPE_EC)
					free(it->pkey.key.ec.q);

				trustAnchor().erase(it);
			}

			static void byte_vector_append(void* ctx, const void* buff, size_t len)
			{
				std::vector<uint8_t>* vec = static_cast<std::vector<uint8_t>*>(ctx);
				vec->reserve(vec->size() + len); // Allocate extra space all at once
				for (size_t i = 0; i < len; i++)
					vec->push_back(((uint8_t*)buff)[i]);
			}
						
			static bool append_cert_x509(const bear::br_x509_certificate& xc)
			{
				bear::br_x509_trust_anchor& ta = trustAnchor().emplace_back();
				auto certIt = trustAnchor().begin() + (trustAnchor().size() - 1);
				memset(&ta, 0, sizeof(bear::br_x509_trust_anchor));

				std::vector<byte> vdn;
				bear::br_x509_decoder_context dc;
				bear::br_x509_decoder_init(&dc, byte_vector_append, &vdn);
				bear::br_x509_decoder_push(&dc, xc.data, xc.data_len);
				bear::br_x509_pkey* pk = bear::br_x509_decoder_get_pkey(&dc);

				if (pk == nullptr)
					return false;
				
				ta.dn.len = vdn.size();
				ta.dn.data = (unsigned char*)malloc(vdn.size());
				memcpy(ta.dn.data, vdn.data(), vdn.size());

				ta.flags = bear::br_x509_decoder_isCA(&dc) ? BR_X509_TA_CA : 0;
				ta.pkey.key_type = pk->key_type;
				switch (ta.pkey.key_type)
				{
				case BR_KEYTYPE_RSA:
					ta.pkey.key.rsa.nlen = pk->key.rsa.nlen;
					ta.pkey.key.rsa.n = (unsigned char*)malloc(pk->key.rsa.nlen);
					memcpy(ta.pkey.key.rsa.n, pk->key.rsa.n, pk->key.rsa.nlen);
					ta.pkey.key.rsa.elen = pk->key.rsa.elen;
					ta.pkey.key.rsa.e = (unsigned char*)malloc(pk->key.rsa.elen);
					memcpy(ta.pkey.key.rsa.e, pk->key.rsa.e, pk->key.rsa.elen);
					break;
				case BR_KEYTYPE_EC:
					ta.pkey.key.ec.curve = pk->key.ec.curve;
					ta.pkey.key.ec.qlen = pk->key.ec.qlen;
					ta.pkey.key.ec.q = (unsigned char*)malloc(pk->key.ec.qlen);
					memcpy(ta.pkey.key.ec.q, pk->key.ec.q, pk->key.ec.qlen);
					break;
				default:
					return false;
				}
				return true;
			}


			static std::vector<std::string> parse_base64_pem_certificate(std::string certificates)
			{
				std::vector<std::string> certificateOutput;
				while(true)
				{
					std::string decoded = parse_single_base64_pem_certificate(certificates);
					if (!decoded.empty())
						certificateOutput.emplace_back(std::move(decoded));
				}

				return certificateOutput;
			}

			static std::string parse_single_base64_pem_certificate(std::string_view certificates)
			{
				const char* beginOfCertificate = "-----BEGIN CERTIFICATE-----";
				const char* endOfCertificate = "-----END CERTIFICATE-----";

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
			TLSCertificate()
			{
				memset(&this->data, 0, sizeof(bear::br_x509_certificate));
			}

			TLSCertificate(const TLSCertificate& copy)
			{
				memset(&this->data, 0, sizeof(bear::br_x509_certificate));

				data.data_len = copy.data.data_len;
				data.data = (byte*)malloc(copy.data.data_len);
				memcpy(data.data, copy.data.data, copy.data.data_len);
			}

			TLSCertificate(std::string_view data)
			{
				memset(&this->data, 0, sizeof(bear::br_x509_certificate));

				std::string decoded = std::move(parse_single_base64_pem_certificate(data));
				if (!decoded.empty())
				{
					this->data.data_len = decoded.size();
					this->data.data = (byte*)malloc(decoded.size());
					memcpy(this->data.data, decoded.data(), decoded.size());
				}
			}

			inline bool IsValid()
			{
				return data.data != nullptr;
			}

			static const std::vector<bear::br_x509_trust_anchor>& GetCerticiatesChain()
			{
				return trustAnchor();
			}

			static void AddTrustedCertificate(const TLSCertificate& certificate)
			{
				append_cert_x509(certificate.data);
			}

			~TLSCertificate()
			{
				free(data.data);
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
	using namespace cppu;
#endif