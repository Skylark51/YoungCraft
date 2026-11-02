# Publication plots · 2026-10-23

Scan, IRC, optimization, spectra, and 2D scan maps now use a common white-background, Arial-based design. The default curve uses blue (#005C8A), open circular sample markers, black inward ticks, and no grid. Scan candidates use a red (#BE262D) diamond and a leader label. Curves connect measured samples without invented smoothing.

## Use

- Right-click any graph for **축·표기 설정** (axis titles, ranges, independent X/Y tick intervals, grid, points, candidate visibility).
- Right-click for **논문용 그림 내보내기**, or use the matching button in Scan Results.
- Choose single-column (84.6 mm), double-column (177.8 mm), or custom width and height. PDF retains vector paths and text. PNG supports 300/600/1200 dpi with physical resolution metadata and optional transparency.
- Figure copying uses the publication renderer at 600 dpi. Interactive selection rings are excluded from publication output; the data, ranges and fonts in the active graph are preserved.
- Mechanism stage labels occupy a separate lane below the ticks. Names wrap at word boundaries instead of being truncated. The existing stage editor still controls names and coordinate ranges.

## Scientific interpretation

A scan's highest interior local maximum is a **TS candidate**, not a verified saddle point. Candidate detection does not establish a Hessian signature or IRC connectivity. The annotation can be hidden or renamed in the Scan Results style editor. Mechanism stage names and ranges remain user annotations, not inferred chemical identities.

2D scans retain measured cells and missing cells; the sequential color scale avoids rainbow hue ordering. Switching between 1D and 2D clears the old manual range because the axes represent different quantities.

## Physical output

Reference: [JACS Author Guidelines, Appendix 2](https://researcher-resources.acs.org/publish/author_guidelines?coden=jacsat).

The guidelines specify single-column artwork up to 240 pt and double-column artwork up to 504 pt, legible lettering at least 4.5 pt, and strokes at least 0.5 pt. Default labels use 9–10 pt Arial; publication rendering guards against strokes below 0.5 pt. Raster resolution guidance is 300 dpi for color, 600 dpi for grayscale, and 1200 dpi for black-and-white line art. Choose the export format and resolution appropriate to the intended submission.

## Validation

GUI regression: 59 passed, 0 failed; one real-file Scan test requires an external fixture directory. Additional checks cover independent numeric ticks, interaction after layout changes, candidate detection, mechanism ranges, PNG dimensions/alpha/DPI, export state preservation, invalid output rejection, spectra zero baselines, 1D-to-2D range reset, and the export preview. PDF samples were rendered with Poppler and inspected; the single/double-column media boxes are 240/504 pt wide and contain vector artwork with embedded font resources and no raster images.
