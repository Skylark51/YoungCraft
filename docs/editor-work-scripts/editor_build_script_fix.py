from pathlib import Path
p=Path('tools/build_editor_release.ps1');s=p.read_text(encoding='utf-8').replace("'^Qt6_DIR:PATH='","'^Qt6_DIR:[^=]+='").replace("$entry.Substring('Qt6_DIR:PATH='.Length)","($entry -split '=', 2)[1]").replace('-DQt6_DIR=','-DQt6_DIR:PATH=');p.write_text(s,encoding='utf-8')
