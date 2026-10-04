# Validation record

Date: 2026-10-04. Cleanup baseline commit: `fe62d2107854046a7849c3859a385ce12074210a`.

## Preservation and structure

- 25 retained blobs are unchanged at their current paths.
- 1019 generated build/cache/executable entries are omitted from the current tree; the baseline history remains available.
- New documents and required configuration/path adaptations are recorded in the cleanup pull request. No existing source history is rewritten.
- Current filenames have no case-insensitive collisions. Markdown file links and generated-output ignore rules are checked before publication.

## Checks and limits

- All retained firmware C++/header bytes and PlatformIO profiles match the original Git blobs.
- Generated `.pio`, `.vs`, and machine-specific compilation database files are excluded from the current tree.
- PlatformIO toolchains are not installed on the setup Mac, so this cleanup does not claim a new firmware build.
- ESP32 wiring, sensors, relay outputs, Aliyun credentials, MQTT connectivity, and notification workflows require separate hardware/cloud verification.

