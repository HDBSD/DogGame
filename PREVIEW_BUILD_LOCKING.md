# Preview build access

`PREVIEW_BUILD_LOCKING` is a CMake option, OFF by default. When enabled, `DogGame.exe` authenticates before initializing the game. Public builds exclude the authentication source, startup call, and authentication-specific libraries.

The C implementation ports the Auth0 flow in the workspace's `example.ps1`:

- Tenant: `hdbsd.uk.auth0.com`
- Public client ID: `REqhoUiXBRbPPPGsCPeyQgYh2ZhzUJjM`
- Connection: `discord`
- Callback: `http://127.0.0.1:8765/callback`
- Namespaced ID-token claim: `https://early-access.hdbsd.co.uk/discord_roles`
- Required role: **`1559332156709404683`**

The Auth0 Post-Login Action must check membership in Discord guild `1559324244180074526` and populate the claim with role IDs as JSON strings. The game checks the exact role in that signed claim. No Discord bot token or Auth0 client secret belongs in the executable.

## Build

From the workspace root, using CMake on PATH:

```powershell
# Preview: login and whitelist required
cmake -S SD100F -B SD100F/build -DPREVIEW_BUILD_LOCKING=ON
cmake --build SD100F/build --config Release --target DogGame

# Public release: no login or access controls
cmake -S SD100F -B SD100F/build -DPREVIEW_BUILD_LOCKING=OFF
cmake --build SD100F/build --config Release --target DogGame
```

Both configurations produce `SD100F/build/Release/DogGame.exe`. CMake remembers the selected flag in its build cache: explicitly set it OFF before preparing a public release. There is no command-line or environment-variable bypass for locked builds.

## Auth0 configuration

Use a Native/public application with Authorization Code + PKCE enabled, token endpoint authentication set to None, and RS256 ID-token signing. Register the exact callback URI above in Allowed Callback URLs. The existing Discord connection and Post-Login Action from the PowerShell example must be enabled for this client.

The role claim must look like:

```json
{
  "https://early-access.hdbsd.co.uk/discord_roles": ["1559332156709404683"]
}
```

## Player flow

Launch the preview normally. The default browser opens Auth0's Discord sign-in. Complete the login, then return to the game. If the role is present, startup continues. If the role is missing, empty, or incorrectly typed, an error says **“You need to be whitelisted to run this preview build”** and the process exits with code 3.

Login denial, network errors, invalid tokens, and expiry also block startup with a separate authentication error. Closing the browser leaves the process waiting until its five-minute login timeout. Only one login attempt can own local port 8765; close a previous attempt before launching another.

Every preview launch performs a new authentication flow and role-claim check; tokens are not stored on disk and there is no offline authorization cache. The game checks access at startup; revoking a role does not terminate an already-running play session. The Auth0 Action must retrieve current membership/roles on each login for revocation to take effect on the next launch.

## Validation

`analysis/Check-PreviewAuth.py` exercises the production C implementation offline using genuine locally generated RS256 signatures. It covers exact-role approval, missing/wrong/malformed roles, invalid signatures, issuer/audience/nonce/expiry, duplicate claims, and callback parsing/state validation. The implementation uses Windows CNG for PKCE and signature verification, WinHTTP with normal HTTPS certificate validation for the fixed tenant, and an exclusive loopback-only Winsock listener.

References: [Auth0 PKCE flow](https://auth0.com/docs/get-started/authentication-and-authorization-flow/authorization-code-flow-with-pkce/add-login-using-the-authorization-code-flow-with-pkce), [ID-token validation](https://auth0.com/docs/secure/tokens/id-tokens/validate-id-tokens).
