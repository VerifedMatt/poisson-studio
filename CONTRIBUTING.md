# Contributing

Use the [setup guide](docs/SETUP.md), work on a branch, and keep provider-specific parsing in the relevant adapter. Preserve NULL for missing observations and source distinctions for derived minutes/ratings. New sources must have documented coverage and identity-matching rules.

Run `verify.ps1` for model/setup changes. Run `verify.ps1 -Integration -Graphics` in a fresh installation for schema, import, startup or UI changes. Add meaningful regression tests for changed parsing/model behavior; keep network responses and credentials out of unit tests. Update the user/provider/model docs when behavior changes.

Pull requests should explain the concrete problem, resulting behavior, relevant limitations and checks performed. Include a screenshot for visible changes, using synthetic data and excluding personal paths/secrets. Large dependency updates should include their upstream version, SHA-256, license review and clean-install results.

Contributions are provided under the project's GPL-2.0-only license. Do not contribute data or assets you do not have permission to redistribute.
