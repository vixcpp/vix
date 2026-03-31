# Advanced Security Section --- Sessions & JWT

This guide expands on advanced security considerations for
**Session-based** and **JWT-based** authentication.

This is for developers building real production systems.

# 1️⃣ Transport Security (MANDATORY)

## Always use HTTPS

Without HTTPS:

-   Cookies can be intercepted
-   JWT tokens can be stolen
-   Session IDs can be hijacked

In production:

-   Enable TLS
-   Set Secure cookies
-   Use HSTS

Example (session strict preset):

``` cpp
app.use(middleware::app::session_strict("strong_secret"));
```

# 2️⃣ Cookie Security (Session)

When using Sessions:

## Enable:

-   HttpOnly → prevents JS access
-   Secure → HTTPS only
-   SameSite → Lax or Strict

Example:

``` cpp
middleware::app::session_dev(
    "secret",
    "sid",
    std::chrono::hours(24),
    true,       // secure
    "Strict",   // same_site
    true        // http_only
);
```

## Protect Against:

-   XSS
-   CSRF
-   Session fixation

# 3️⃣ CSRF Protection (Session)

Sessions are vulnerable to CSRF.

Use CSRF middleware:

``` cpp
app.use(middleware::app::csrf_dev());
```

Why?

Because browsers automatically send cookies.

JWT (in Authorization header) is less vulnerable to CSRF.

# 4️⃣ JWT Security Best Practices

## Use Strong Secret

Do NOT use:

"dev" "1234" "secret"

Use:

-   256-bit random secret
-   Or environment variable

## Set Expiration

Always set:

-   exp claim
-   Short TTL for access tokens

Enable expiration verification:

``` cpp
middleware::app::jwt_dev("secret", true);
```

## Avoid Storing JWT in LocalStorage

Why?

LocalStorage is accessible to JavaScript. If XSS occurs → token stolen.

Safer option:

-   Store JWT in HttpOnly Secure cookie
-   Or use short-lived tokens

# 5️⃣ Token Revocation Strategy

JWT problem:

You cannot easily revoke tokens.

Solutions:

-   Short expiration (15 min)
-   Refresh tokens
-   Blacklist storage
-   Rotate signing keys

# 6️⃣ Session Storage Hardening

If using sessions at scale:

-   Use Redis
-   Use expiration
-   Limit session size
-   Rotate session ID on login

Prevent session fixation:

Generate new session ID after authentication.

# 7️⃣ RBAC Hardening

Never trust client roles.

Always verify on server:

``` cpp
require_role("admin");
require_perm("products:write");
```

Never rely only on frontend checks.

# 8️⃣ Rate Limiting + Auth

Protect authentication endpoints:

-   Rate limit login
-   Rate limit token generation

Example:

``` cpp
app.use(middleware::app::rate_limit_dev(10));
```

# 9️⃣ Defense in Depth

Combine:

-   HTTPS
-   Security headers
-   CSRF
-   Rate limit
-   RBAC
-   Logging
-   Monitoring

Security is layered.

# 🔟 Production Checklist

For Sessions:

-   HTTPS
-   Secure + HttpOnly
-   SameSite configured
-   CSRF enabled
-   Session TTL
-   Session ID rotation

For JWT:

-   Strong secret
-   exp enabled
-   verify_exp = true
-   Short TTL
-   Secure storage
-   RBAC enforcement

# Final Advice

Authentication is not just middleware.

It is:

-   Transport security
-   Storage security
-   Token lifecycle management
-   Authorization enforcement
-   Monitoring and logging

Build systems assuming attackers exist.

Security is architecture, not configuration.
