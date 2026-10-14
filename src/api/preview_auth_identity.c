/* Validate cryptographic identity independently of browser and HTTP state.
 * Evidence: the existing preview protocol checks, not recovered Xbox code. */
#include <windows.h>
#include <bcrypt.h>
#include "preview_auth_config.h"
#include "preview_auth_identity.h"
#include "preview_auth_json.h"

int auth_sha256(const void *data, size_t length, unsigned char hash[32])
{
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_HASH_HANDLE handle = NULL;
    int ok = 0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0)
    {
        goto done;
    }
    if (BCryptCreateHash(alg, &handle, NULL, 0, NULL, 0, 0) < 0)
    {
        goto done;
    }
    if (BCryptHashData(handle, (PUCHAR)data, (ULONG)length, 0) < 0)
    {
        goto done;
    }
    ok = BCryptFinishHash(handle, hash, 32, 0) >= 0;
done:
    if (handle)
    {
        BCryptDestroyHash(handle);
    }
    if (alg)
    {
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    return ok;
}

int auth_random_value(char value[44])
{
    unsigned char bytes[32];
    if (BCryptGenRandom(NULL, bytes, sizeof bytes, BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
    {
        return 0;
    }
    return base64url_encode(bytes, sizeof bytes, value, 44) == 43;
}

static int auth_decode_json(const char *src, size_t length, char *dst, size_t cap, JsonParser *json)
{
    size_t n = base64url_decode(src, length, (unsigned char *)dst, cap - 1);
    if (!n || memchr(dst, 0, n))
    {
        return 0;
    }
    dst[n] = 0;
    return json_parse(json, dst) && json->tokens[0].type == JSON_OBJECT;
}

/* JWKS comes only from the fixed tenant's HTTPS endpoint. Never follow jku
 * or x5u supplied in a token. Support Auth0's asymmetric RS256 key rotation. */
static int auth_verify_signature(const char *jwt, size_t signed_length, const unsigned char *signature,
                                 size_t siglen, const JsonParser *keys, const char *kid)
{
    int array = json_find_member(keys, 0, "keys"), key = -1;
    if (array < 0 || keys->tokens[array].type != JSON_ARRAY)
    {
        return 0;
    }
    for (int i = array + 1; i < keys->tokens[array].next; i = keys->tokens[i].next)
    {
        if (json_string_equals(keys, json_find_member(keys, i, "kid"), kid))
        {
            if (key >= 0)
            {
                return 0;
            }
            key = i;
        }
    }
    if (key < 0 || !json_string_equals(keys, json_find_member(keys, key, "kty"), "RSA"))
    {
        return 0;
    }
    int alg_claim = json_find_member(keys, key, "alg"), use = json_find_member(keys, key, "use");
    if ((alg_claim >= 0 && !json_string_equals(keys, alg_claim, "RS256")) ||
        (use >= 0 && !json_string_equals(keys, use, "sig")))
    {
        return 0;
    }
    char modulus[2048], exponent[64];
    unsigned char n[1024], e[32], hash[32];
    if (!json_read_string(keys, json_find_member(keys, key, "n"), modulus, sizeof modulus) ||
        !json_read_string(keys, json_find_member(keys, key, "e"), exponent, sizeof exponent))
    {
        return 0;
    }
    size_t nl = base64url_decode(modulus, strlen(modulus), n, sizeof n),
           el = base64url_decode(exponent, strlen(exponent), e, sizeof e);
    if (nl < 256 || !el || siglen != nl || !auth_sha256(jwt, signed_length, hash))
    {
        return 0;
    }
    unsigned char blob[sizeof(BCRYPT_RSAKEY_BLOB) + sizeof n + sizeof e];
    BCRYPT_RSAKEY_BLOB header = {BCRYPT_RSAPUBLIC_MAGIC, (ULONG)nl * 8, (ULONG)el, (ULONG)nl, 0, 0};
    memcpy(blob, &header, sizeof header);
    memcpy(blob + sizeof header, e, el);
    memcpy(blob + sizeof header + el, n, nl);
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_KEY_HANDLE imported = NULL;
    BCRYPT_PKCS1_PADDING_INFO pad = {BCRYPT_SHA256_ALGORITHM};
    int ok = 0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_RSA_ALGORITHM, NULL, 0) < 0)
    {
        goto done;
    }
    if (BCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &imported, blob,
                            (ULONG)(sizeof header + el + nl), 0) < 0)
    {
        goto done;
    }
    ok = BCryptVerifySignature(imported, &pad, hash, sizeof hash, (PUCHAR)signature, (ULONG)siglen,
                               BCRYPT_PAD_PKCS1) >= 0;
done:
    if (imported)
    {
        BCryptDestroyKey(imported);
    }
    if (alg)
    {
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    return ok;
}

/* 1 approved, 0 invalid authentication, -1 valid identity without whitelist. */
int auth_verify_identity_token(const char *jwt, const char *nonce, const char *jwks, int64_t now)
{
    const char *first = strchr(jwt, '.'), *second = first ? strchr(first + 1, '.') : NULL;
    if (!first || !second || strchr(second + 1, '.'))
    {
        return 0;
    }
    JsonParser header, claims, keys;
    char h[4096], payload[32768], kid[256];
    unsigned char sig[1024];
    if (!auth_decode_json(jwt, (size_t)(first - jwt), h, sizeof h, &header) ||
        !auth_decode_json(first + 1, (size_t)(second - first - 1), payload, sizeof payload, &claims) ||
        !json_parse(&keys, jwks) || keys.tokens[0].type != JSON_OBJECT)
    {
        return 0;
    }
    if (!json_string_equals(&header, json_find_member(&header, 0, "alg"), "RS256") ||
        !json_read_string(&header, json_find_member(&header, 0, "kid"), kid, sizeof kid))
    {
        return 0;
    }
    size_t siglen = base64url_decode(second + 1, strlen(second + 1), sig, sizeof sig);
    if (!siglen || !auth_verify_signature(jwt, (size_t)(second - jwt), sig, siglen, &keys, kid))
    {
        return 0;
    }
    int audience = json_find_member(&claims, 0, "aud");
    if (!json_string_equals(&claims, json_find_member(&claims, 0, "iss"), PA_ISSUER) ||
        !(json_string_equals(&claims, audience, PA_CLIENT) ||
          json_array_contains(&claims, audience, PA_CLIENT)) ||
        !json_string_equals(&claims, json_find_member(&claims, 0, "nonce"), nonce))
    {
        return 0;
    }
    if (audience >= 0 && claims.tokens[audience].type == JSON_ARRAY)
    {
        int azp = json_find_member(&claims, 0, "azp");
        if (!json_string_equals(&claims, azp, PA_CLIENT))
        {
            return 0;
        }
    }
    int64_t expiry, issued, not_before;
    if (!json_read_integer(&claims, json_find_member(&claims, 0, "exp"), &expiry) || expiry <= now ||
        !json_read_integer(&claims, json_find_member(&claims, 0, "iat"), &issued) || issued > now + 60 ||
        issued >= expiry)
    {
        return 0;
    }
    int nbf = json_find_member(&claims, 0, "nbf");
    if (nbf >= 0 && (!json_read_integer(&claims, nbf, &not_before) || not_before > now + 60))
    {
        return 0;
    }
    char subject[512];
    if (!json_read_string(&claims, json_find_member(&claims, 0, "sub"), subject, sizeof subject) ||
        !subject[0])
    {
        return 0;
    }
    return json_array_contains(&claims, json_find_member(&claims, 0, PA_ROLE_CLAIM), PA_ROLE) ? 1 : -1;
}
