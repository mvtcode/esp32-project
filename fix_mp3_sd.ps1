# ==============================================================================
# Script: fix_mp3_sd.ps1
# execute: powershell -ExecutionPolicy Bypass -File .\fix_mp3_sd.ps1
# Smart Mode: Kiem tra tung file - File nao LOI / CO ANH BIA / SAI TAN SO moi fix
# ==============================================================================

$ffmpegDir = "D:\ffmpeg-2025-07-01-git-11d1b71c31-full_build\bin"
$ffmpeg  = Join-Path $ffmpegDir "ffmpeg.exe"
$ffprobe = Join-Path $ffmpegDir "ffprobe.exe"
$srcDir  = "E:\"
$outDir  = "E:\Fixed"

# Kiem tra cong cu
if (!(Test-Path $ffmpeg) -or !(Test-Path $ffprobe)) {
    Write-Host "[ERROR] Khong tim thay ffmpeg/ffprobe tai: $ffmpegDir" -ForegroundColor Red
    Exit
}

# Kiem tra o dia E:\
if (!(Test-Path $srcDir)) {
    Write-Host "[ERROR] Khong tim thay the nho SD tai: $srcDir" -ForegroundColor Red
    Exit
}

if (!(Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir | Out-Null
}

function ConvertTo-CleanFileName ([string]$text) {
    $normalized = $text.Normalize([System.Text.NormalizationForm]::FormD)
    $sb = New-Object System.Text.StringBuilder
    foreach ($c in $normalized.ToCharArray()) {
        $cat = [System.Globalization.CharUnicodeInfo]::GetUnicodeCategory($c)
        if ($cat -ne [System.Globalization.UnicodeCategory]::NonSpacingMark) {
            [void]$sb.Append($c)
        }
    }
    $result = $sb.ToString()
    $result = $result.Replace([char]0x0111, "d").Replace([char]0x0110, "D")
    $result = $result -replace "[^\w\s-]", ""
    $result = $result -replace "\s+", "_"
    if ($result.Length -gt 45) {
        $result = $result.Substring(0, 45).TrimEnd("_")
    }
    return $result
}

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host " BAT DAU KIEM TRA & XU LY THONG MINH (SMART SCAN)" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

$files = Get-ChildItem -Path $srcDir -Filter "*.mp3"
if ($files.Count -eq 0) {
    Write-Host "Khong tim thay file MP3 nao trong $srcDir!" -ForegroundColor Yellow
    Exit
}

$count = 0
$fixedCount = 0
$okCount = 0

foreach ($f in $files) {
    $count++
    $filePath = $f.FullName

    Write-Host "[$count/$($files.Count)] Kiem tra: $($f.Name)" -ForegroundColor White

    # 1. Kiem tra xem file co loi stream khong
    $errOutput = & $ffmpeg -v error -i $filePath -f null - 2>&1
    $hasStreamError = [bool]($errOutput -and $errOutput.Count -gt 0)

    # 2. Kiem tra xem co chua anh bia (Cover Art / Video Stream) khong
    $hasCoverArt = $false
    $videoStream = & $ffprobe -v error -select_streams v -show_entries stream=codec_name -of csv=p=0 $filePath 2>$null
    if ($videoStream) { $hasCoverArt = $true }

    # 3. Kiem tra sample rate
    $sampleRate = & $ffprobe -v error -select_streams a:0 -show_entries stream=sample_rate -of csv=p=0 $filePath 2>$null
    $needResample = ($sampleRate -ne "44100")

    # 4. Doc bitrate goc
    $bitrateStr = & $ffprobe -v error -select_streams a:0 -show_entries stream=bit_rate -of csv=p=0 $filePath 2>$null
    if (!$bitrateStr -or $bitrateStr -eq "N/A") {
        $bitrateStr = & $ffprobe -v error -show_entries format=bit_rate -of csv=p=0 $filePath 2>$null
    }

    $bitrateKbps = 192
    if ($bitrateStr -match "^\d+$") {
        $bitrateKbps = [math]::Round([int64]$bitrateStr / 1000)
        if ($bitrateKbps -gt 320) { $bitrateKbps = 320 }
        if ($bitrateKbps -lt 128) { $bitrateKbps = 128 }
    }

    # Chuan hoa ten file
    $cleanName = ConvertTo-CleanFileName ($f.BaseName)
    $outPath = Join-Path $outDir ($cleanName + ".mp3")

    # Danh gia file co can Fix hay khong
    $needFix = $hasStreamError -or $hasCoverArt -or $needResample

    if ($needFix) {
        $reasons = @()
        if ($hasStreamError) { $reasons += "Loi frame audio" }
        if ($hasCoverArt)    { $reasons += "Co anh bia (Cover Art nang)" }
        if ($needResample)   { $reasons += "Tan so $sampleRate Hz (can 44.1kHz)" }
        
        Write-Host "   -> [CAN FIX] Ly do: $($reasons -join ', ')" -ForegroundColor Yellow
        Write-Host "   -> Dang convert voi Bitrate: ${bitrateKbps} kbps..." -ForegroundColor Gray

        & $ffmpeg -y -i $filePath -vn -map_metadata -1 -ar 44100 -ac 2 -b:a "${bitrateKbps}k" -c:a libmp3lame $outPath 2>$null

        if ($LASTEXITCODE -eq 0) {
            Write-Host "   -> [OK] Da fix xong!" -ForegroundColor Green
            $fixedCount++
        } else {
            Write-Host "   -> [FAIL] Khong the fix file nay!" -ForegroundColor Red
        }
    } else {
        Write-Host "   -> [HOAN HAO] File chuan 44.1kHz, khong anh bia, khong loi." -ForegroundColor Green
        # Copy sang thu muc Fixed voi ten chuan hoa
        Copy-Item -Path $filePath -Destination $outPath -Force
        $okCount++
    }
    Write-Host "--------------------------------------------------"
}

Write-Host "==================================================" -ForegroundColor Green
Write-Host " TONG KET:" -ForegroundColor Green
Write-Host "   - So file da fix loi/xoa anh bia: $fixedCount file" -ForegroundColor Yellow
Write-Host "   - So file sach da dat chuan    : $okCount file" -ForegroundColor Green
Write-Host "   - Tat ca file da gom ve         : $outDir" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Green
