#ifndef PREVIEW_AUTH_IDENTITY_H
#define PREVIEW_AUTH_IDENTITY_H
#include <stddef.h>
#include <stdint.h>
/* Fixed-tenant RS256 verification: 1 authorized, 0 invalid identity,
 * -1 valid identity without the required preview role. No network calls. */
int auth_verify_identity_token(const char *jwt, const char *nonce, const char *jwks, int64_t now);
int auth_sha256(const void *data, size_t length, unsigned char hash[32]);
int auth_random_value(char value[44]);
#endif
