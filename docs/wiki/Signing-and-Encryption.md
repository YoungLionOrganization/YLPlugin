# Signing and Encryption

Three independent mechanisms answer different questions:

| Mechanism | Question |
|---|---|
| SHA256 plaintext hash | Are these resource bytes intact? |
| Ed25519 signature and trust callback | Are package bytes authenticated by a host-trusted signer? |
| AES-256-GCM and content-key callback | Can this host authenticate/decrypt this protected record? |

None grants execution permission. Hosts separately review publishers, native
code/RCC content, API compatibility and resource budgets.

## Signing coverage

The optional signature authenticates the header, YLSpec frame, TOC, padding,
stored payload and signature descriptor/key ID. The scheme signs a domain
string plus SHA512 of the defined prefix using ordinary Ed25519; it is not
Ed25519ph. Use the SDK/CLI implementation rather than recreating that scheme.

A trust callback supplies a 32-byte Ed25519 public key for an accepted key ID
and rejects unknown/revoked/disallowed IDs. Public publisher text alone is not
trusted. Signature verification can authenticate encrypted stored bytes without
obtaining the content key.

## Encryption coverage

AES-256-GCM encrypts a compressed record when compression is selected. The TOC
record, including resource path and key ID, is authenticated as AAD with its tag
field zeroed. Nonces are cryptographically random on each encryption. The
authentication tag must verify before decompression or release of plaintext;
the decoded SHA256 is checked too. Metadata remains readable without keys.

Content keys are 32 raw bytes supplied synchronously by the host. Key IDs are
lookup identifiers, not key material. Account, payment, entitlement and key
rotation policy belong to the host; the container is not a license server.

## Local CLI demonstration

In a fresh private POSIX working directory with OpenSSL and a built CLI:

```sh
umask 077
openssl rand -out content.key 32
openssl genpkey -algorithm ED25519 -out publisher-private.pem
openssl pkey -in publisher-private.pem -pubout -out publisher-public.pem

ylplugin encrypt data.ylplugin --output encrypted.ylplugin \
  --key-id content-v1 --key-file content.key
ylplugin sign encrypted.ylplugin --output signed.ylplugin \
  --key-id publisher-v1 --private-key publisher-private.pem
ylplugin verify signed.ylplugin --key-id publisher-v1 --public-key publisher-public.pem
ylplugin validate signed.ylplugin --key-id content-v1 --key-file content.key
```

Each output must be new. `content.key` is binary, not a 64-character hex file.
The CLI's private key input is unencrypted Ed25519 PEM; an encrypted PEM fails
without prompting. Keep these generated files private and outside repository/
release artifacts. The provided `examples/security.sh` automates a similar
fresh-key workflow against the data fixture.

The CLI supports one content key and one signer identity per invocation. If
their IDs differ, use separate verify/validate commands as above or SDK providers.
Encrypting signed input requires explicit signature removal, followed by signing
the changed output again. Rekeying an already encrypted package uses the SDK
builder/providers; no pretend CLI rekey command is supplied.

See [the exact crypto contract](../specification.md#authentication-and-confidentiality)
and [CLI Cookbook](CLI-Cookbook.md).
