#ifndef PREVIEW_AUTH_CONFIG_H
#define PREVIEW_AUTH_CONFIG_H
/* Fixed preview tenant, client, redirect and role. Token-provided endpoints
 * must never replace these trust settings. */
#define PA_DOMAIN "hdbsd.uk.auth0.com"
#define PA_CLIENT "REqhoUiXBRbPPPGsCPeyQgYh2ZhzUJjM"
#define PA_ISSUER "https://" PA_DOMAIN "/"
#define PA_REDIRECT "http://127.0.0.1:8765/callback"
#define PA_ROLE_CLAIM "https://early-access.hdbsd.co.uk/discord_roles"
#define PA_ROLE "1559332156709404683"
#define PA_LIMIT 65536
#endif
