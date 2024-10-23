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

int SSL_CTX_use_certificate_chain_pointer(SSL_CTX *ctx, const char *file, int len);

int SSL_CTX_use_PrivateKey_pointer(SSL_CTX *ctx, const char *file, int len);

STACK_OF(X509_NAME) *SSL_load_client_CA_pointer(const char *file, int len);

int SSL_CTX_load_verify_pointer(SSL_CTX *ctx, const char *file, int len);

DH* PEM_read_DHparams_file(const char *file, DH **x, pem_password_cb *cb, void *u);

DH* PEM_read_DHparams_pointer(const char *file, int len, DH **x, pem_password_cb *cb, void *u);