# Shared Youngcraft Studio preset — 2026-10-21

The calibrated Youngkly appearance now lives in `StyleManager::preset("Youngcraft Studio")`. It includes the complete element palette, radius scales, bond radius, white background, orthographic camera, lighting directions/intensities, shadow strength and antialiasing defaults.

Dark Studio derives directly from this preset, replacing only its name and background (pure black). Its legacy saved settings migrate once to `darkStudioRevision` 1; subsequent edits persist normally.

The wide neutral atom reflection is part of the shared shader and controlled by `RenderStyle::atomSheenStrength`. Studio enables it at 0.08; other presets keep the default value of zero. The field round-trips through style settings. Previously saved non-Studio styles without the field default to zero.

Saved Studio styles at revision 0–2 are replaced with the new preset once and saved at revision 3. Later user edits round-trip normally. This implements the explicit request to replace Studio, including its old saved appearance. Other named presets are not migrated.

Youngkly's render dialog now reads this preset and only applies its brightness and shadow controls. The local GLSL injection adapter has been removed to prevent duplicated sheen or divergent material settings.

Checks: preset/migration and style round-trip model tests, Youngkly render preset parity, OpenGL controls/export regression, and a three-structure PNG/32-frame GIF generation using the shared preset. High-resolution export remains enabled.
