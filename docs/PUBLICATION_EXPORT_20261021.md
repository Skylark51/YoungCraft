# Publication export — 2026-10-21

The prior save path captured widget-resolution pixels and enlarged the bitmap in `ImageExporter::supersample`. Animation similarly scaled a captured widget image. Neither operation could generate new geometric detail.

`MoleculeView::imageAtSize` now renders into a dedicated offscreen framebuffer at the requested pixel size. It reuses the live molecule, camera, style, bonds and surfaces without resizing the widget. The framebuffer uses up to 4x MSAA. The existing 2x-per-axis SSAA stage targets that framebuffer when within GPU size/memory limits (32 million internal pixels during export). If SSAA cannot be allocated, the output framebuffer retains MSAA. Invalid or unsupported output sizes return a null image so callers can report a failure instead of silently enlarging a screenshot. The output pixel budget is 32 million.

The camera aspect, shadow-pass restoration, projection of annotations, QOpenGLPaintDevice target, and sphere mesh LOD use the export dimensions. The public capture restores flags and the widget target before returning, releases its large SSAA buffer with a current context, and returns straight-alpha or opaque sRGB pixels with 600 dpi metadata. QOpenGLFramebufferObject::toImage resolves MSAA before readback: https://doc.qt.io/qt-6/qopenglframebufferobject.html#toImage.

Save and copy use this API at a default long edge of 3000px. Save allows a different pixel size; available GPU memory can limit very large requests. Transparent PNG encoding preserves resolution and color-space metadata. Animation now renders at the selected dimensions instead of stretching the preview.

Verification:

- Youngkly's render control test exercises the shared implementation: exact output dimensions, rerendered detail differing from an enlarged low-resolution image, transparent/opaque/partial coverage, saturated transparent edges, high-resolution labels, DPI, invalid dimensions, and unchanged live view.
- Youngcraft's `supersamplingSmoothsDiagonalEdges` additionally checks saturated blue partial-alpha edges, saved transparent PNG DPI, camera/viewport preservation and GL error state.
- Additional existing rendering tests cover shadows, outward faces, depth, continuous bonds and sphere self-shadowing.
- Youngkly generated a three-structure path and decoded all 32 TS GIF frames. The first cropped PNG is 1815 × 1691 pixels, with a measured DPI of 599.9988 due to the PNG integer pixels-per-metre field.

The format remains raster PNG: detail is tied to the chosen output pixel count, rather than unlimited vector magnification. GIF's palette limitation remains; use the corresponding high-resolution PNG for static paper figures.

Build environment: MinGW Qt moc failed to write output under the Unicode workspace path. The successful build uses a temporary `subst R:` mapping of this source folder and `D:/yc-tools/youngcraft-publication-build`. Build configuration selects a separate `app/` output folder before deployment to the project executable. No unrelated chemistry or optimization algorithms changed.
