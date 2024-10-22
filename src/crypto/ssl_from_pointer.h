/*
 * Authored by Matthew Reiner, 2024.
 * Intellectual property of third-party.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef __INTELLISENSE__
#include <openssl/ssl.h>
#endif

int SSL_CTX_use_certificate_chain_pointer(SSL_CTX *ctx, const char *file, int len){
	BIO *bio = BIO_new_mem_buf((void*)file, len);
	if(!bio) return 0;
	X509* cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);

	// Load top cert. SSL_CTX_use_certificate() fails gracefully if cert == NULL
	int ret = SSL_CTX_use_certificate(ctx, cert);
	X509_free(cert);
	if(ret != 1) goto end;
	
	// In case there was previously a certificate
	SSL_CTX_clear_chain_certs(ctx);

	// Keep reading until there are no more
	while((cert = PEM_read_bio_X509(bio, NULL, NULL, NULL))){
		if((ret = SSL_CTX_add0_chain_cert(ctx, cert)) != 1){
			X509_free(cert);
			goto end;
		}
		// If above succeeded then ownership of X509* cert is taken (refcount is not increased)
	}
	ret = 1;
	end:
	BIO_free(bio);
	return ret;
}

int SSL_CTX_use_PrivateKey_pointer(SSL_CTX *ctx, const char *file, int len){
	BIO *bio = BIO_new_mem_buf((void*)file, len);
	if(!bio) return 0;
	EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, NULL, 0, NULL);
	BIO_free(bio);

	// Load private key. SSL_CTX_use_PrivateKey() fails gracefully if cert == NULL
	int ret = SSL_CTX_use_PrivateKey(ctx, pkey);

	// Always free, ownership of pkey is not taken
	EVP_PKEY_free(pkey);
	return ret;
}

STACK_OF(X509_NAME) *SSL_load_client_CA_pointer(const char *file, int len){
	BIO *bio = BIO_new_mem_buf((void*)file, len);
	if(!bio) return 0;
	STACK_OF(X509_NAME) *ca_list = sk_X509_NAME_new_null();
	if(!ca_list) return 0;
	X509 *x509;

	// Keep reading until there are no more
	while((x509 = PEM_read_bio_X509(bio, NULL, 0, NULL))){
		X509_NAME *name = X509_get_subject_name(x509);
		if(name && (name = X509_NAME_dup(name))) sk_X509_NAME_push(ca_list, name);
		X509_free(x509);
	}
	BIO_free(bio);

	// Free & return if no CAs were added
	if(!sk_X509_NAME_num(ca_list)){
		sk_X509_NAME_free(ca_list);
		return NULL;
	}
	return ca_list;
}

int SSL_CTX_load_verify_pointer(SSL_CTX *ctx, const char *file, int len){
	BIO *bio = BIO_new_mem_buf((void*)file, len);
	if(!bio) return 0;
	int ret;
	X509 *cert;
	// In case there was previously a certificate
	SSL_CTX_clear_chain_certs(ctx);
	// Load each certificate from the CA data and add to the trust store
	while((cert = PEM_read_bio_X509(bio, NULL, NULL, NULL))){
		if((ret = SSL_CTX_add0_chain_cert(ctx, cert)) != 1){
			X509_free(cert);
			goto end;
		}
		// If above succeeded then ownership of X509* cert is taken (refcount is not increased)
	}
	ret = 1;
	end:
	BIO_free(bio);
	return ret;
}

DH* PEM_read_DHparams_file(const char *file, DH **x, pem_password_cb *cb, void *u){
	BIO *bio = BIO_new(BIO_s_file());
	if(!bio) return 0;
	FILE* fp = fopen(file, "r");
	if(!fp) return 0;
	BIO_set_fp(bio, fp, BIO_CLOSE);
   DH* ret = PEM_read_bio_DHparams(bio, x, cb, u);
	BIO_free(bio);
	return ret;
}

DH* PEM_read_DHparams_pointer(const char *file, int len, DH **x, pem_password_cb *cb, void *u){
	BIO *bio = BIO_new_mem_buf((void*)file, len);
	if(!bio) return 0;
   DH* ret = PEM_read_bio_DHparams(bio, x, cb, u);
	BIO_free(bio);
	return ret;
}