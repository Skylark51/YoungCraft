# Refactor notes

This package is a structural refactor only. No requested user-facing feature was intentionally added, removed, or redesigned. Existing member-function bodies were relocated into smaller translation units. Generated build trees, packaged binaries, portable archive duplicates, and UI reference screenshots were removed from the source package.

The split is intentionally conservative: public class headers and signal/slot contracts remain unchanged. Gaussian parsing keeps the same single-pass `GaussianParser::parse` body; only its private helper block was moved to an included implementation fragment.
