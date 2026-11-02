$ErrorActionPreference = 'Stop'
$word = $null
$doc = $null
$ppt = $null
$deck = $null
$report = [ordered]@{}
try {
    Add-Type -AssemblyName System.Windows.Forms
    $clip = [Windows.Forms.Clipboard]::GetDataObject()
    $report.formats = @($clip.GetFormats())
    $report.htmlLength = ([string]$clip.GetData('HTML Format')).Length
    if ($env:YOUNGCRAFT_REACTION_PROBE_TARGET -ne 'PowerPoint') {
    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $doc = $word.Documents.Add()
    $doc.Range(0,0).Paste()
    $report.wordCount = $doc.InlineShapes.Count
    $report.wordFloatingCount = $doc.Shapes.Count
    $report.wordText = $doc.Content.Text
    $report.wordWidthsCm = @($doc.InlineShapes | ForEach-Object { [Math]::Round($_.Width * 2.54 / 72, 3) })
    $report.wordHeightsCm = @($doc.InlineShapes | ForEach-Object { [Math]::Round($_.Height * 2.54 / 72, 3) })
    [xml]$package = $doc.WordOpenXML
    $report.wordMedia = @($package.SelectNodes('//*[local-name()="part"]') | Where-Object { $_.GetAttribute('name', 'http://schemas.microsoft.com/office/2006/xmlPackage') -like '/word/media/*' } | ForEach-Object { $_.GetAttribute('name', 'http://schemas.microsoft.com/office/2006/xmlPackage') })
    if ($env:YOUNGCRAFT_REACTION_CAPTURE_DIR) { [IO.File]::WriteAllText((Join-Path $env:YOUNGCRAFT_REACTION_CAPTURE_DIR 'word-paste.xml'), $doc.WordOpenXML) }
    if ($doc.InlineShapes.Count -ne 3) { throw 'Expected three individually editable images in Word.' }
    foreach ($shape in $doc.InlineShapes) { if ([Math]::Abs($shape.Width*2.54/72 - 5) -gt 0.05) { throw 'Word width differs from 5 cm.' } }
    if ($env:YOUNGCRAFT_REACTION_CAPTURE_DIR) {
        $report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $env:YOUNGCRAFT_REACTION_CAPTURE_DIR 'office-probe.json') -Encoding UTF8
        foreach ($part in $package.SelectNodes('//*[local-name()="part"]')) {
            $mediaName=$part.GetAttribute('name','http://schemas.microsoft.com/office/2006/xmlPackage')
            if ($mediaName -like '/word/media/*') {
                $binary=$part.SelectSingleNode('*[local-name()="binaryData"]')
                [IO.File]::WriteAllBytes((Join-Path $env:YOUNGCRAFT_REACTION_CAPTURE_DIR ([IO.Path]::GetFileName($mediaName))),[Convert]::FromBase64String($binary.InnerText))
            }
        }
    }
    $doc.Close(0); $doc=$null; $word.Quit();[Runtime.InteropServices.Marshal]::ReleaseComObject($word) | Out-Null;$word=$null
    }
    if ($env:YOUNGCRAFT_REACTION_PROBE_TARGET -eq 'PowerPoint') {
    if (Get-Process POWERPNT -ErrorAction SilentlyContinue) { throw 'Close PowerPoint before running the optional isolated paste probe.' }
    $ppt = New-Object -ComObject PowerPoint.Application
    $deck = $ppt.Presentations.Add(-1)
    $slide = $deck.Slides.Add(1,12)
    $pasted = $slide.Shapes.Paste()
    $report.pptCount = $slide.Shapes.Count
    $report.pptWidthsCm = @($slide.Shapes | ForEach-Object { [Math]::Round($_.Width*2.54/72,3) })
    $report.pptTypes = @($slide.Shapes | ForEach-Object { $_.Type })
    $report.pptHeightsCm = @($slide.Shapes | ForEach-Object { [Math]::Round($_.Height*2.54/72,3) })
    if ($slide.Shapes.Count -ne 3) { throw 'Expected three editable pictures in PowerPoint.' }
    foreach ($shape in $slide.Shapes) { if ([Math]::Abs($shape.Width*2.54/72 - 10) -gt 0.05) { throw 'PowerPoint width differs from 10 cm.' } }
    if (($report.pptHeightsCm | Measure-Object -Maximum).Maximum - ($report.pptHeightsCm | Measure-Object -Minimum).Minimum -gt 0.05) { throw 'PowerPoint picture heights differ.' }
    if ($env:YOUNGCRAFT_REACTION_CAPTURE_DIR) {
        $path=Join-Path $env:YOUNGCRAFT_REACTION_CAPTURE_DIR ('ppt-paste-'+[Guid]::NewGuid().ToString('N')+'.pptx')
        $deck.SaveCopyAs($path,24)
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zip=[IO.Compression.ZipFile]::OpenRead($path)
        try {
            $entry=@($zip.Entries | Where-Object { $_.FullName -like 'ppt/media/*.gif' })
            if ($entry.Count -ne 1) { throw 'PowerPoint did not retain the animated GIF.' }
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry[0],(Join-Path $env:YOUNGCRAFT_REACTION_CAPTURE_DIR 'ppt-embedded-TS.gif'),$true)
            $report.pptGifRetained=$true
        } finally { $zip.Dispose() }
    }
    }
    $report | ConvertTo-Json -Depth 5
} finally {
    if ($env:YOUNGCRAFT_REACTION_CAPTURE_DIR) {
        $name=if($env:YOUNGCRAFT_REACTION_PROBE_TARGET -eq 'PowerPoint'){'ppt-probe.json'}else{'word-probe.json'}
        $report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $env:YOUNGCRAFT_REACTION_CAPTURE_DIR $name) -Encoding UTF8
    }
    if ($null -ne $doc) { $doc.Close(0) }
    if ($null -ne $word) { $word.Quit(); [Runtime.InteropServices.Marshal]::ReleaseComObject($word) | Out-Null }
    if ($null -ne $deck) { $deck.Close() }
    if ($null -ne $ppt) { $ppt.Quit(); [Runtime.InteropServices.Marshal]::ReleaseComObject($ppt) | Out-Null }
}
