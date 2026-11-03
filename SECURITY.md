# Security policy

This experimental project reads untrusted binary input and is not production
security reviewed. Report a suspected issue privately through the repository
owner's designated security contact or GitHub private vulnerability reporting
when enabled. Do not publish live keys, licensed payloads or unrelated user data.

Include the version, platform, minimal reproducer, expected result and observed
failure. There is no claimed response-time SLA or external audit certification.
The threat model, current limitations and release conditions are in
docs/security.md and RELEASES.md.

## SDK and publishing boundaries

Native Node/JNI/ctypes runtime loading is separate from untrusted plugin code
activation. Callback close/reentry, integer narrowing and Unicode conversion
are tested at the language boundary. Organization secret checks print only names
and availability. Registry uploads require matching successful CI/release/tag
and verified artifacts; do not expose tokens through debugging or Actions outputs.
