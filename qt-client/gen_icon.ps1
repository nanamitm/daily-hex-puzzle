# gen_icon.ps1 — Generates app.ico for the Windows resource file.
# Draws the same hex design as appicon.h using System.Drawing.
param([string]$OutDir = $PSScriptRoot)

Add-Type -AssemblyName System.Drawing

function Get-HexPts($cx, $cy, $r) {
    0..5 | ForEach-Object {
        $a = $_ * [Math]::PI / 3.0
        [System.Drawing.PointF]::new(
            [float]($cx + $r * [Math]::Sin($a)),
            [float]($cy - $r * [Math]::Cos($a)))
    }
}

function Draw-HexBitmap($size) {
    $bmp = [System.Drawing.Bitmap]::new($size, $size,
           [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g   = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)

    $cx = $size / 2.0; $cy = $size / 2.0; $R = $size * 0.46

    $bg = Get-HexPts $cx $cy $R
    $g.FillPolygon(
        [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(30,55,120)),
        $bg)

    $colors = @('#4682B4','#FFA032','#3CB371','#DC5050','#9467BD','#40C8C8','#F0C832')
    $ox = @(0.0, 0.5, 0.5,  0.0,-0.5,-0.5, 0.0)
    $oy = @(-0.5,-0.25,0.25,0.5, 0.25,-0.25,0.0)
    $sr = $R * 0.30

    for ($k = 0; $k -lt 7; $k++) {
        $pts = Get-HexPts ($cx + $ox[$k]*$R) ($cy + $oy[$k]*$R) $sr
        $col = [System.Drawing.ColorTranslator]::FromHtml($colors[$k])
        $g.FillPolygon([System.Drawing.SolidBrush]::new($col), $pts)
        if ($size -ge 32) {
            $pw = [float]($size * 0.018)
            $g.DrawPolygon(
                [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(20,20,20), $pw),
                $pts)
        }
    }

    $bw = [float]($size * 0.05)
    $g.DrawPolygon(
        [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(15,35,90), $bw),
        $bg)

    $g.Dispose()
    return $bmp
}

# Write a multi-size ICO file (PNG entries, supported on Vista+)
function Write-Ico($bitmaps, $path) {
    $entries = @()
    foreach ($bmp in $bitmaps) {
        $ms = [System.IO.MemoryStream]::new()
        $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        $entries += ,@{ Size = $bmp.Width; Data = $ms.ToArray() }
        $ms.Dispose()
    }

    $out = [System.IO.MemoryStream]::new()
    $bw  = [System.IO.BinaryWriter]::new($out)

    # ICONDIR header
    $bw.Write([uint16]0)              # reserved
    $bw.Write([uint16]1)              # type: ICO
    $bw.Write([uint16]$entries.Count) # count

    # Offset of first image data = 6 + 16*count
    $offset = 6 + 16 * $entries.Count
    foreach ($e in $entries) {
        $sz = $e.Size
        $szByte = if ($sz -eq 256) { [byte]0 } else { [byte]$sz }
        $bw.Write($szByte) # width  (0 means 256)
        $bw.Write($szByte) # height
        $bw.Write([byte]0)    # colour count (0 = truecolour)
        $bw.Write([byte]0)    # reserved
        $bw.Write([uint16]1)  # planes
        $bw.Write([uint16]32) # bit count
        $bw.Write([uint32]$e.Data.Length)
        $bw.Write([uint32]$offset)
        $offset += $e.Data.Length
    }
    foreach ($e in $entries) { $bw.Write($e.Data) }

    $bw.Flush()
    [System.IO.File]::WriteAllBytes($path, $out.ToArray())
    $out.Dispose()
}

$sizes   = @(16, 32, 48, 256)
$bitmaps = $sizes | ForEach-Object { Draw-HexBitmap $_ }

$icoPath = Join-Path $OutDir "app.ico"
Write-Ico $bitmaps $icoPath
$bitmaps | ForEach-Object { $_.Dispose() }
Write-Host "Generated: $icoPath"
