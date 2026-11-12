param([string]$Directory,[int]$Clips,[int]$Width,[int]$Height,[string]$Mode='pptx')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression,System.IO.Compression.FileSystem
$cancel=Join-Path $Directory 'cancel'
$zip=[IO.Compression.ZipFile]::Open((Join-Path $Directory "presentation.$Mode"),[IO.Compression.ZipArchiveMode]::Create)
function Part([string]$Name,[string]$Xml) {
    $writer=[IO.StreamWriter]::new($zip.CreateEntry($Name).Open(),[Text.UTF8Encoding]::new($false))
    try { $writer.Write($Xml) } finally { $writer.Dispose() }
}
function Asset([string]$Name,[string]$Source) {
    [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip,(Join-Path $Directory $Source),$Name,[IO.Compression.CompressionLevel]::NoCompression)
}
$rel='http://schemas.openxmlformats.org/officeDocument/2006/relationships'
$pkg='http://schemas.openxmlformats.org/package/2006/relationships'
$p='http://schemas.openxmlformats.org/presentationml/2006/main'
$a='http://schemas.openxmlformats.org/drawingml/2006/main'
$ns="xmlns:p='$p' xmlns:a='$a' xmlns:r='$rel'"
$tree='<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr/>'
$mapping='<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>'
$timing=@'
<p:timing><p:tnLst><p:par><p:cTn id="1" dur="indefinite" restart="never" nodeType="tmRoot"><p:childTnLst>
<p:seq concurrent="1" nextAc="seek"><p:cTn id="2" dur="indefinite" nodeType="mainSeq"><p:childTnLst>
<p:par><p:cTn id="3" fill="hold"><p:stCondLst><p:cond delay="indefinite"/><p:cond evt="onBegin" delay="0"><p:tn val="2"/></p:cond></p:stCondLst><p:childTnLst>
<p:par><p:cTn id="4" fill="hold"><p:stCondLst><p:cond delay="0"/></p:stCondLst><p:childTnLst>
<p:par><p:cTn id="5" presetID="1" presetClass="mediacall" presetSubtype="0" fill="hold" nodeType="withEffect"><p:stCondLst><p:cond delay="0"/></p:stCondLst><p:childTnLst>
<p:cmd type="call" cmd="playFrom(0.0)"><p:cBhvr><p:cTn id="6" dur="1" fill="hold"/><p:tgtEl><p:spTgt spid="2"/></p:tgtEl></p:cBhvr></p:cmd>
</p:childTnLst></p:cTn></p:par></p:childTnLst></p:cTn></p:par></p:childTnLst></p:cTn></p:par>
</p:childTnLst></p:cTn><p:prevCondLst><p:cond evt="onPrev" delay="0"><p:tgtEl><p:sldTgt/></p:tgtEl></p:cond></p:prevCondLst><p:nextCondLst><p:cond evt="onNext" delay="0"><p:tgtEl><p:sldTgt/></p:tgtEl></p:cond></p:nextCondLst></p:seq>
<p:video><p:cMediaNode><p:cTn id="7" fill="hold" display="0"><p:stCondLst><p:cond delay="indefinite"/></p:stCondLst></p:cTn><p:tgtEl><p:spTgt spid="2"/></p:tgtEl></p:cMediaNode></p:video>
</p:childTnLst></p:cTn></p:par></p:tnLst></p:timing>
'@
try {
    if ($Mode -eq 'zip') {
        Asset '00-start.png' 'start.png'
        for ($i=1;$i -le $Clips;++$i) {
            if (Test-Path -LiteralPath $cancel) { return }
            Asset ('{0:D3}-scene.mp4' -f $i) "scene-$i.mp4"
            Asset ('{0:D3}-end.png' -f $i) "end-$i.png"
        }
        return
    }
    $cx=12192000
    $cy=[int][Math]::Round($cx*$Height/$Width)
    $content='<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="png" ContentType="image/png"/><Default Extension="mp4" ContentType="video/mp4"/>'
    foreach ($pair in @(@('presentation.xml','presentation.main'),@('presProps.xml','presProps'),@('slideMasters/slideMaster1.xml','slideMaster'),@('slideLayouts/slideLayout1.xml','slideLayout'))) {
        $content+="<Override PartName='/ppt/$($pair[0])' ContentType='application/vnd.openxmlformats-officedocument.presentationml.$($pair[1])+xml'/>"
    }
    $content+='<Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/>'
    $links="<Relationship Id='master' Type='$rel/slideMaster' Target='slideMasters/slideMaster1.xml'/><Relationship Id='props' Type='$rel/presProps' Target='presProps.xml'/>"
    $ids=''
    for ($i=0;$i -le $Clips;++$i) {
        if (Test-Path -LiteralPath $cancel) { return }
        $slide=$i+1
        $poster="poster-$i.png"
        $ids+="<p:sldId id='$($slide+255)' r:id='slide$slide'/>"
        $links+="<Relationship Id='slide$slide' Type='$rel/slide' Target='slides/slide$slide.xml'/>"
        $content+="<Override PartName='/ppt/slides/slide$slide.xml' ContentType='application/vnd.openxmlformats-officedocument.presentationml.slide+xml'/>"
        $slideLinks="<Relationship Id='layout' Type='$rel/slideLayout' Target='../slideLayouts/slideLayout1.xml'/><Relationship Id='poster' Type='$rel/image' Target='../media/$poster'/>"
        $media=''; $action=''; $effects=''
        if ($i -eq 0) { Asset "ppt/media/$poster" 'start.png' }
        else {
            $movie="scene-$i.mp4"
            Asset "ppt/media/$poster" "end-$i.png"
            Asset "ppt/media/$movie" $movie
            $slideLinks+="<Relationship Id='video' Type='$rel/video' Target='../media/$movie'/><Relationship Id='media' Type='http://schemas.microsoft.com/office/2007/relationships/media' Target='../media/$movie'/>"
            $media='<a:videoFile r:link="video"/><p:extLst><p:ext uri="{DAA4B4D4-6D71-4841-9C94-3DE7FCFB9230}"><p14:media xmlns:p14="http://schemas.microsoft.com/office/powerpoint/2010/main" r:embed="media"/></p:ext></p:extLst>'
            $action='<a:hlinkClick r:id="" action="ppaction://media"/>'
            $effects=$timing
        }
        $picture="<p:pic><p:nvPicPr><p:cNvPr id='2' name='Reaction scene $i'>$action</p:cNvPr><p:cNvPicPr><a:picLocks noChangeAspect='1'/></p:cNvPicPr><p:nvPr>$media</p:nvPr></p:nvPicPr><p:blipFill><a:blip r:embed='poster'/><a:stretch><a:fillRect/></a:stretch></p:blipFill><p:spPr><a:xfrm><a:off x='0' y='0'/><a:ext cx='$cx' cy='$cy'/></a:xfrm><a:prstGeom prst='rect'><a:avLst/></a:prstGeom></p:spPr></p:pic>"
        Part "ppt/slides/slide$slide.xml" "<p:sld $ns><p:cSld><p:spTree>$tree$picture</p:spTree></p:cSld>$mapping<p:transition advClick='1'/>$effects</p:sld>"
        Part "ppt/slides/_rels/slide$slide.xml.rels" "<Relationships xmlns='$pkg'>$slideLinks</Relationships>"
    }
    Part '[Content_Types].xml' "<Types xmlns='http://schemas.openxmlformats.org/package/2006/content-types'>$content</Types>"
    Part '_rels/.rels' "<Relationships xmlns='$pkg'><Relationship Id='office' Type='$rel/officeDocument' Target='ppt/presentation.xml'/></Relationships>"
    Part 'ppt/_rels/presentation.xml.rels' "<Relationships xmlns='$pkg'>$links</Relationships>"
    Part 'ppt/presentation.xml' "<p:presentation $ns><p:sldMasterIdLst><p:sldMasterId id='2147483648' r:id='master'/></p:sldMasterIdLst><p:sldIdLst>$ids</p:sldIdLst><p:sldSz cx='$cx' cy='$cy'/><p:notesSz cx='6858000' cy='9144000'/></p:presentation>"
    Part 'ppt/presProps.xml' "<p:presentationPr $ns><p:showPr useTimings='0'><p:present/><p:sldAll/><p:extLst><p:ext uri='{2FDB2607-1784-4EEB-B798-7EB5836EED8A}'><p14:showMediaCtrls xmlns:p14='http://schemas.microsoft.com/office/powerpoint/2010/main' val='0'/></p:ext></p:extLst></p:showPr></p:presentationPr>"
    $colors='<p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/>'
    Part 'ppt/slideMasters/slideMaster1.xml' "<p:sldMaster $ns><p:cSld><p:spTree>$tree</p:spTree></p:cSld>$colors<p:sldLayoutIdLst><p:sldLayoutId id='2147483649' r:id='layout'/></p:sldLayoutIdLst><p:txStyles><p:titleStyle/><p:bodyStyle/><p:otherStyle/></p:txStyles></p:sldMaster>"
    Part 'ppt/slideMasters/_rels/slideMaster1.xml.rels' "<Relationships xmlns='$pkg'><Relationship Id='layout' Type='$rel/slideLayout' Target='../slideLayouts/slideLayout1.xml'/><Relationship Id='theme' Type='$rel/theme' Target='../theme/theme1.xml'/></Relationships>"
    Part 'ppt/slideLayouts/slideLayout1.xml' "<p:sldLayout $ns type='blank' preserve='1'><p:cSld name='Blank'><p:spTree>$tree</p:spTree></p:cSld>$mapping</p:sldLayout>"
    Part 'ppt/slideLayouts/_rels/slideLayout1.xml.rels' "<Relationships xmlns='$pkg'><Relationship Id='master' Type='$rel/slideMaster' Target='../slideMasters/slideMaster1.xml'/></Relationships>"
    $swatches=[ordered]@{dk1='000000';lt1='FFFFFF';dk2='172338';lt2='F1F4F9';accent1='456DD1';accent2='CF6252';accent3='39A185';accent4='775CC4';accent5='37A0C9';accent6='E89C47';hlink='2355B8';folHlink='604A85'}
    $scheme=''
    foreach ($color in $swatches.Keys) { $scheme+="<a:$color><a:srgbClr val='$($swatches[$color])'/></a:$color>" }
    $font='<a:latin typeface="Arial"/><a:ea typeface=""/><a:cs typeface=""/>'
    $fill='<a:solidFill><a:schemeClr val="phClr"/></a:solidFill>'
    $line="<a:ln w='12700'>$fill</a:ln>"
    $effect='<a:effectStyle><a:effectLst/></a:effectStyle>'
    Part 'ppt/theme/theme1.xml' "<a:theme xmlns:a='$a' name='Youngcraft'><a:themeElements><a:clrScheme name='Youngcraft'>$scheme</a:clrScheme><a:fontScheme name='Youngcraft'><a:majorFont>$font</a:majorFont><a:minorFont>$font</a:minorFont></a:fontScheme><a:fmtScheme name='Youngcraft'><a:fillStyleLst>$($fill*3)</a:fillStyleLst><a:lnStyleLst>$($line*3)</a:lnStyleLst><a:effectStyleLst>$($effect*3)</a:effectStyleLst><a:bgFillStyleLst>$($fill*3)</a:bgFillStyleLst></a:fmtScheme></a:themeElements></a:theme>"
} finally { $zip.Dispose() }
