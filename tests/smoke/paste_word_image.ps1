# Optional integration probe for copiesMoleculeWithTransparentBackground.
# Uses a temporary Word document and exports its embedded image without saving it.
$ErrorActionPreference = 'Stop'
$word = [Runtime.InteropServices.Marshal]::GetActiveObject('Word.Application')
$doc = $null
try {
    $doc = $word.Documents.Add()
    $doc.Range(0,0).Paste()
    if ($doc.InlineShapes.Count -ne 1) { throw 'Word did not paste an image.' }
    [xml]$package = $doc.WordOpenXML
    $media = @($package.SelectNodes('//*[local-name()="part"]') | Where-Object { $_.GetAttribute('name', 'http://schemas.microsoft.com/office/2006/xmlPackage') -like '/word/media/*' })
    if ($media.Count -ne 1) { throw 'Expected one Word image.' }
    $binary = $media[0].SelectSingleNode('*[local-name()="binaryData"]')
    [IO.File]::WriteAllBytes($env:YOUNGCRAFT_WORD_PASTE_IMAGE, [Convert]::FromBase64String($binary.InnerText))
    Write-Output $media[0].GetAttribute('name', 'http://schemas.microsoft.com/office/2006/xmlPackage')
} finally {
    if ($null -ne $doc) { $doc.Close(0) }
    [Runtime.InteropServices.Marshal]::ReleaseComObject($word) | Out-Null
}
