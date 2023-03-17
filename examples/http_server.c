/* This is a barebone keep-alive HTTP server */
#include <libusockets.h>
/* Compiling with LIBUS_NO_SSL will ignore SSL */
const int SSL = 1;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct http_socket {
	/* How far we have streamed our response */
	int offset;
};

struct http_context {
	/* The shared response */
	char *response;
	int length;
};

/* We don't need any of these */
void on_wakeup(struct us_loop_t *loop) {

}

void on_pre(struct us_loop_t *loop) {

}

/* This is not HTTP POST, it is merely an event emitted post loop iteration */
void on_post(struct us_loop_t *loop) {

}

struct us_socket_t *on_http_socket_writable(struct us_socket_t *s) {
	struct http_socket *http_socket = (struct http_socket *) us_socket_ext(SSL, s);
	struct http_context *http_context = (struct http_context *) us_socket_context_ext(SSL, us_socket_context(SSL, s));

	/* Stream whatever is remaining of the response */
	http_socket->offset += us_socket_write(SSL, s, http_context->response + http_socket->offset, http_context->length - http_socket->offset, 0);

	return s;
}

struct us_socket_t *on_http_socket_close(struct us_socket_t *s, int code, void *reason) {
	// printf("Client disconnected\n");

	return s;
}

struct us_socket_t *on_http_socket_end(struct us_socket_t *s) {
	/* HTTP does not support half-closed sockets */
	us_socket_shutdown(SSL, s);
	return us_socket_close(SSL, s, 0, NULL);
}

struct us_socket_t *on_http_socket_data(struct us_socket_t *s, char *data, int length) {
	/* Get socket extension and the socket's context's extension */
	struct http_socket *http_socket = (struct http_socket *) us_socket_ext(SSL, s);
	struct http_context *http_context = (struct http_context *) us_socket_context_ext(SSL, us_socket_context(SSL, s));

	/* We treat all data events as a request */
	http_socket->offset = us_socket_write(SSL, s, http_context->response, http_context->length, 0);

	/* Reset idle timer */
	us_socket_timeout(SSL, s, 30);

	return s;
}

struct us_socket_t *on_http_socket_open(struct us_socket_t *s, int is_client, char *ip, int ip_length) {
	struct http_socket *http_socket = (struct http_socket *) us_socket_ext(SSL, s);

	/* Reset offset */
	http_socket->offset = 0;

	/* Timeout idle HTTP connections */
	us_socket_timeout(SSL, s, 30);

	// printf("Client connected\n");

	return s;
}

struct us_socket_t *on_http_socket_timeout(struct us_socket_t *s) {
	/* Close idle HTTP sockets */
	return us_socket_close(SSL, s, 0, NULL);
}

int main() {
	/* Create the event loop */
	struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

	/* Create a socket context for HTTP */
	struct us_bun_socket_context_options_t options = {};
	const char* certs[] = {
		"-----BEGIN CERTIFICATE-----\nMIIDXTCCAkWgAwIBAgIJAKLdQVPy90jjMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV\nBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBX\naWRnaXRzIFB0eSBMdGQwHhcNMTkwMjAzMTQ0OTM1WhcNMjAwMjAzMTQ0OTM1WjBF\nMQswCQYDVQQGEwJBVTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50\nZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\nCgKCAQEA7i7IIEdICTiSTVx+ma6xHxOtcbd6wGW3nkxlCkJ1UuV8NmY5ovMsGnGD\nhJJtUQ2j5ig5BcJUf3tezqCNW4tKnSOgSISfEAKvpn2BPvaFq3yx2Yjz0ruvcGKp\nDMZBXmB/AAtGyN/UFXzkrcfppmLHJTaBYGG6KnmU43gPkSDy4iw46CJFUOupc51A\nFIz7RsE7mbT1plCM8e75gfqaZSn2k+Wmy+8n1HGyYHhVISRVvPqkS7gVLSVEdTea\nUtKP1Vx/818/HDWk3oIvDVWI9CFH73elNxBkMH5zArSNIBTehdnehyAevjY4RaC/\nkK8rslO3e4EtJ9SnA4swOjCiqAIQEwIDAQABo1AwTjAdBgNVHQ4EFgQUv5rc9Smm\n9c4YnNf3hR49t4rH4yswHwYDVR0jBBgwFoAUv5rc9Smm9c4YnNf3hR49t4rH4ysw\nDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEATcL9CAAXg0u//eYUAlQa\nL+l8yKHS1rsq1sdmx7pvsmfZ2g8ONQGfSF3TkzkI2OOnCBokeqAYuyT8awfdNUtE\nEHOihv4ZzhK2YZVuy0fHX2d4cCFeQpdxno7aN6B37qtsLIRZxkD8PU60Dfu9ea5F\nDDynnD0TUabna6a0iGn77yD8GPhjaJMOz3gMYjQFqsKL252isDVHEDbpVxIzxPmN\nw1+WK8zRNdunAcHikeoKCuAPvlZ83gDQHp07dYdbuZvHwGj0nfxBLc9qt90XsBtC\n4IYR7c/bcLMmKXYf0qoQ4OzngsnPI5M+v9QEHvYWaKVwFY4CTcSNJEwfXw+BAeO5\nOA==\n-----END CERTIFICATE-----"};
	options.cert = &(certs[0]);
	options.cert_count = 1;
	const char* keys[] = { "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDuLsggR0gJOJJN\nXH6ZrrEfE61xt3rAZbeeTGUKQnVS5Xw2Zjmi8ywacYOEkm1RDaPmKDkFwlR/e17O\noI1bi0qdI6BIhJ8QAq+mfYE+9oWrfLHZiPPSu69wYqkMxkFeYH8AC0bI39QVfOSt\nx+mmYsclNoFgYboqeZTjeA+RIPLiLDjoIkVQ66lznUAUjPtGwTuZtPWmUIzx7vmB\n+pplKfaT5abL7yfUcbJgeFUhJFW8+qRLuBUtJUR1N5pS0o/VXH/zXz8cNaTegi8N\nVYj0IUfvd6U3EGQwfnMCtI0gFN6F2d6HIB6+NjhFoL+QryuyU7d7gS0n1KcDizA6\nMKKoAhATAgMBAAECggEAd5g/3o1MK20fcP7PhsVDpHIR9faGCVNJto9vcI5cMMqP\n6xS7PgnSDFkRC6EmiLtLn8Z0k2K3YOeGfEP7lorDZVG9KoyE/doLbpK4MfBAwBG1\nj6AHpbmd5tVzQrnNmuDjBBelbDmPWVbD0EqAFI6mphXPMqD/hFJWIz1mu52Kt2s6\n++MkdqLO0ORDNhKmzu6SADQEcJ9Suhcmv8nccMmwCsIQAUrfg3qOyqU4//8QB8ZM\njosO3gMUesihVeuF5XpptFjrAliPgw9uIG0aQkhVbf/17qy0XRi8dkqXj3efxEDp\n1LSqZjBFiqJlFchbz19clwavMF/FhxHpKIhhmkkRSQKBgQD9blaWSg/2AGNhRfpX\nYq+6yKUkUD4jL7pmX1BVca6dXqILWtHl2afWeUorgv2QaK1/MJDH9Gz9Gu58hJb3\nymdeAISwPyHp8euyLIfiXSAi+ibKXkxkl1KQSweBM2oucnLsNne6Iv6QmXPpXtro\nnTMoGQDS7HVRy1on5NQLMPbUBQKBgQDwmN+um8F3CW6ZV1ZljJm7BFAgNyJ7m/5Q\nYUcOO5rFbNsHexStrx/h8jYnpdpIVlxACjh1xIyJ3lOCSAWfBWCS6KpgeO1Y484k\nEYhGjoUsKNQia8UWVt+uWnwjVSDhQjy5/pSH9xyFrUfDg8JnSlhsy0oC0C/PBjxn\nhxmADSLnNwKBgQD2A51USVMTKC9Q50BsgeU6+bmt9aNMPvHAnPf76d5q78l4IlKt\nwMs33QgOExuYirUZSgjRwknmrbUi9QckRbxwOSqVeMOwOWLm1GmYaXRf39u2CTI5\nV9gTMHJ5jnKd4gYDnaA99eiOcBhgS+9PbgKSAyuUlWwR2ciL/4uDzaVeDQKBgDym\nvRSeTRn99bSQMMZuuD5N6wkD/RxeCbEnpKrw2aZVN63eGCtkj0v9LCu4gptjseOu\n7+a4Qplqw3B/SXN5/otqPbEOKv8Shl/PT6RBv06PiFKZClkEU2T3iH27sws2EGru\nw3C3GaiVMxcVewdg1YOvh5vH8ZVlxApxIzuFlDvnAoGAN5w+gukxd5QnP/7hcLDZ\nF+vesAykJX71AuqFXB4Wh/qFY92CSm7ImexWA/L9z461+NKeJwb64Nc53z59oA10\n/3o2OcIe44kddZXQVP6KTZBd7ySVhbtOiK3/pCy+BQRsrC7d71W914DxNWadwZ+a\njtwwKjDzmPwdIXDSQarCx0U=\n-----END PRIVATE KEY-----" };
	options.key = &(keys[0]);
	options.key_count = 1;
	// options.cert_file_name = "./misc/cert.pem";
	// options.key_file_name = "./misc/key.pem";
	options.passphrase = "1234";
	const char* cas[] = { "-----BEGIN CERTIFICATE-----\nMIIDAzCCAesCFHwXVn+q3YnlwmwijComXUB9sTt0MA0GCSqGSIb3DQEBCwUAMDwx\nFDASBgNVBAoMC3VOZXR3b3JraW5nMREwDwYDVQQKDAh1U29ja2V0czERMA8GA1UE\nAwwIdmFsaWRfY2EwHhcNMjMwMzEzMTQxNjM2WhcNMjQwMzEyMTQxNjM2WjBAMRQw\nEgYDVQQKDAt1TmV0d29ya2luZzERMA8GA1UECgwIdVNvY2tldHMxFTATBgNVBAMM\nDHZhbGlkX2NsaWVudDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANPH\nSp1VaI2kTmBprubniSG8j2uu88u5MZxC5Xkv4DMEu0lhWrg9zk07daK2MNeCdZKI\nT4pIgeKxNXSd3KZK9lJxUBMoTuAhvoribJOsz7Z9nF9enSSuhY2Z529tTiftFpYh\n8NcQIame6fykOy1uU7SnPUkq8cXoOmr0/bPs9Kdsb/aAWf8gOt41cKqllmBpMzQJ\nZR5y2DZenQuTG0GUSrQ3kKopOayXATm5gNRf8gGKPsRqo6DwNhbNXYKL/hG/nqDT\nBUNU0aSnFch7HMpAaJ1sY/TAMmkMKMvL40RE0jsm8bf4ah0imAHjG4gBSdZz7oHi\nwQHsEwI53BAcf8XIrxUCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAJHNXWrCgdp0v\njMPFT1Tgs5Jwbp6E+oh4GzcMXCKBT+34r9MymN5h627LbPZ6G7VMZ0TsKsSKMVkz\n44xHclyQZOxRww4Lq3csB03yuYHdQkxAh99+JsU9LwtJiAy8H2aqG/CcgqIHMZjd\nXSBkk28dTLyE9Xfc71K0bVcRNISzyGEDdbcei6MCVi4NdK0Df3R5yKpUQ+2BZ400\ne9gRFLOQrOaYYzZ7fKEPc3LPHzDiLmP7racaMpzL3a/rc5ovWWVKWeHXgDroYWlt\ngfrX1VvftqsuhEg1Bef0bcqjeEf2o9Abk6jKRwpPnG51rUEEfpceSxA+Zk//Taqz\nTfS53plZRQ==\n-----END CERTIFICATE-----"};
	options.ca = &(cas[0]);
	options.ca_count = 1;

	printf("Creating SSL context!!!\n");
	struct us_socket_context_t *http_context = us_create_bun_socket_context(SSL, loop, sizeof(struct http_context), options);

	if (!http_context) {
		printf("Could not load SSL cert/key\n");
		exit(0);
	}

	/* Generate the shared response */
	const char body[] = "<html><body><h1>Why hello there!</h1></body></html>";
	struct http_context *http_context_ext = (struct http_context *) us_socket_context_ext(SSL, http_context);
	http_context_ext->response = (char *) malloc(128 + sizeof(body) - 1);
	http_context_ext->length = snprintf(http_context_ext->response, 128 + sizeof(body) - 1, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n%s", sizeof(body) - 1, body);
	/* Set up event handlers */
	us_socket_context_on_open(SSL, http_context, on_http_socket_open);
	us_socket_context_on_data(SSL, http_context, on_http_socket_data);
	us_socket_context_on_writable(SSL, http_context, on_http_socket_writable);
	us_socket_context_on_close(SSL, http_context, on_http_socket_close);
	us_socket_context_on_timeout(SSL, http_context, on_http_socket_timeout);
	us_socket_context_on_end(SSL, http_context, on_http_socket_end);

	/* Start serving HTTP connections */
	struct us_listen_socket_t *listen_socket = us_socket_context_listen(SSL, http_context, 0, 8000, 0, sizeof(struct http_socket));
	//int ssl, struct us_socket_context_t *context, const char *hostname_pattern, struct us_bun_socket_context_options_t options, void *user
    us_bun_socket_context_add_server_name(SSL, http_context, "localhost", options, NULL);

	if (listen_socket) {
		printf("Listening on port 8000...\n");
		us_loop_run(loop);
	} else {
		printf("Failed to listen!\n");
	}
}
