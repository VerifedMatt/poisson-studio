# Security and private data

Never commit or attach `secrets.bin`, populated `config.json`, database files, API keys, private SQL bootstrap files or complete runtime logs without review. Git ignore rules and source-package allowlists exclude installation state. Dependency downloads require matching SHA-256 before extraction/execution; archive paths are checked before writing files.

The private MySQL server listens on loopback and uses random per-installation credentials. Windows DPAPI binds stored secrets to the current user/device. This does not protect against software already running as that Windows user. The project is a local desktop application, not a hardened multi-user database service.

For a suspected vulnerability, use the destination repository's GitHub **Security → Report a vulnerability** feature when enabled. If unavailable, contact the repository maintainer privately; do not publish credentials or exploit details in a public issue. Repository owners should enable private vulnerability reporting after creating the GitHub repository.

Only the current source release is maintained. Dependency vulnerabilities should be addressed by reviewing and updating the lock file, followed by clean setup/tests; do not disable checksum validation.
