# ECU MEMS Manager — versioning rule

The Windows GitHub Actions build number is the single source of truth for application versions.

Mapping:
- build #610 -> 1.6.10
- build #611 -> 1.6.11
- build #699 -> 1.6.99
- build #700 -> 1.7.0
- build #1000 -> 1.10.0

Formula: `1.(build / 100).(build % 100)` using integer division.

This rule applies to future GitHub Windows builds. The application must derive its displayed version from `GITHUB_RUN_NUMBER` so the title, splash, About/help version and build artifact remain identifiable by the same build number.
