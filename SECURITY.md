# 🔐 Vix.cpp Security Policy

Security is a top priority for the Vix.cpp project.  
We appreciate your help in keeping the framework and its ecosystem safe.

---

## 🧭 Supported Versions

We actively maintain and apply security patches to the following versions:

| Version                  | Supported  | Notes                                          |
| ------------------------ | ---------- | ---------------------------------------------- |
| `main` (development)     | ✅ Yes     | Actively developed and reviewed                |
| `v0.x` (stable releases) | ✅ Yes     | Security fixes and maintenance updates         |
| Pre-release / alpha      | ⚠️ Partial | Use at your own risk, no guarantee of patching |

If you are using an older version, please upgrade to the latest stable release to receive security updates.

---

## 🚨 Reporting a Vulnerability

If you discover a security vulnerability in **Vix.cpp** or any of its modules, please report it **privately and responsibly**.

### 🔒 How to report

Send an email to:

📧 **vixcpp.security@gmail.com**

Include the following details:

1. A detailed description of the issue.
2. Steps to reproduce or proof-of-concept code.
3. The affected version(s) or commit(s).
4. Any suggestions or potential fixes (optional).

> ⚠️ **Do not open a public GitHub issue** for security vulnerabilities.

We take every report seriously and will:

- Confirm receipt within **48 hours**.
- Investigate and reproduce the issue.
- Provide a fix or mitigation as soon as possible.
- Acknowledge you in the release notes (if you wish).

---

## 🧪 Responsible Disclosure Policy

We kindly ask researchers and users to:

- Report vulnerabilities **confidentially** via email.
- Avoid publicly disclosing details until a fix is released.
- Avoid exploiting or abusing vulnerabilities in production systems.

We commit to working **transparently and collaboratively** with the reporter to resolve issues safely and promptly.

---

## 🧰 Security Best Practices for Users

When using Vix.cpp in production:

- Always build with the **latest stable release**.
- Use **compiler hardening flags**:
  ```bash
  -fstack-protector-strong -D_FORTIFY_SOURCE=2 -O2
  ```

. Prefer ASan/UBSan when debugging:

```bash
cmake -DVIX_ENABLE_SANITIZERS=ON
```

. Keep dependencies up to date (Boost, nlohmann/json, etc.).

. Use HTTPS and secure headers in your deployments.

# 🧱 Security Features in Vix.cpp

. 🔒 Safe memory handling with RAII and smart pointers.

. 🧠 Type-safe routing and JSON handling.

. ⚙️ Built-in input validation utilities (Vix::utils::Validation).

. 🧩 Modular isolation — each module can be audited independently.

. 🧰 Optional runtime sanitizers (ASan + UBSan).

# 📜 Acknowledgements

We thank all security researchers and contributors who help make Vix.cpp safer for everyone.
Your responsible disclosure is essential to maintaining a secure ecosystem.

# 💡 Remember:

Security is not a one-time event — it’s an ongoing commitment.
Together, we keep Vix.cpp fast, modular, and secure.
