# ==============================================================================
# Script: video-to-frames.ps1
# Convert video MP4 -> video.avi (320x180 @ 20fps, All-in-One MJPEG + WAV PCM Mono)
# Usage: powershell -ExecutionPolicy Bypass -File .\tools\video-to-frames.ps1 -Fps 18 -Quality 10
# ==============================================================================

param (
    [string]$InputVideo = "$PSScriptRoot\video.mp4",
    [string]$OutputDir = "$PSScriptRoot\out",
    [int]$Fps = 20,     # 20 fps muot ma tren ESP32-S3
    [int]$Width = 320,
    [int]$Height = 180,    # 320x180 ti le chuan 16:9, can giua man hinh 320x240
    [int]$Quality = 10,     # 2-31: 10 cho toc do decode nhanh va chat luong tot
    [int]$AudioRate = 22050   # 22050 Hz Mono WAV PCM toi uu cho I2S DMA
)

Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host "   ESP32-S3 2.8 VIDEO CONVERTER (ALL-IN-ONE AVI: MJPEG + PCM)     " -ForegroundColor Cyan
Write-Host "==================================================================" -ForegroundColor Cyan

# 1. Tim kiem cong cu FFmpeg
$ffmpeg = Join-Path $PSScriptRoot "ffmpeg.exe"
if (!(Test-Path $ffmpeg)) {
    $ffmpegCmd = Get-Command ffmpeg -ErrorAction SilentlyContinue
    if ($ffmpegCmd) {
        $ffmpeg = $ffmpegCmd.Source
    }
    else {
        Write-Host "[ERROR] Khong tim thay ffmpeg.exe tai: $ffmpeg" -ForegroundColor Red
        Write-Host "Vui long tai FFmpeg tu https://www.ffmpeg.org/ va dat vao folder tools/" -ForegroundColor Yellow
        exit 1
    }
}
Write-Host "[1/3] Tim thay FFmpeg: $ffmpeg" -ForegroundColor Green

# 2. Kiem tra file video dau vao
if (!(Test-Path $InputVideo)) {
    Write-Host "[ERROR] Khong tim thay video nguon tai: $InputVideo" -ForegroundColor Red
    exit 1
}

$videoItem = Get-Item $InputVideo
$videoSizeMB = [math]::Round($videoItem.Length / 1MB, 2)
Write-Host "[2/3] Video nguon: $($videoItem.Name) ($videoSizeMB MB)" -ForegroundColor Green

# 3. Tao thu muc output
if (!(Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

$outputAvi = Join-Path $OutputDir "video.avi"

Write-Host "------------------------------------------------------------------" -ForegroundColor DarkGray
Write-Host "Cau hinh convert:" -ForegroundColor White
Write-Host "  - Do phan giai video : $($Width)x$($Height) (Ti le 16:9)" -ForegroundColor White
Write-Host "  - Frame rate         : $Fps fps" -ForegroundColor White
Write-Host "  - Chat luong JPEG    : q:v = $Quality" -ForegroundColor White
Write-Host "  - Dinh dang Audio    : PCM 16-bit Mono @ $AudioRate Hz (Dong goi truc tiep trong AVI)" -ForegroundColor White
Write-Host "  - Output Video AVI   : $outputAvi (All-in-One: Video + Audio)" -ForegroundColor White
Write-Host "------------------------------------------------------------------" -ForegroundColor DarkGray

# 4. Convert Video sang All-in-One AVI (MJPEG + PCM Audio)
Write-Host "[3/3] Dang convert sang All-in-One AVI ($($Width)x$($Height) @ $($Fps)fps, Audio $AudioRate Hz)..." -ForegroundColor Yellow
$swVideo = [System.Diagnostics.Stopwatch]::StartNew()

$scaleFilter = "scale=$($Width):$($Height):force_original_aspect_ratio=decrease,pad=$($Width):$($Height):(ow-iw)/2:(oh-ih)/2"

# Xuat file video.avi chua CA Video Motion JPEG va Audio PCM WAV:
# -> VLC tren may tinh phat ca tieng lan hinh muot ma
# -> ESP32 chi can doc 1 file duy nhat, tu dong bo A/V qua chunk
& $ffmpeg -y -i $InputVideo -vf $scaleFilter -r $Fps -c:v mjpeg -q:v $Quality -c:a pcm_s16le -ar $AudioRate -ac 1 $outputAvi

$swVideo.Stop()

if ($LASTEXITCODE -ne 0 -or !(Test-Path $outputAvi)) {
    Write-Host "[ERROR] Convert video that bai! Exit code: $LASTEXITCODE" -ForegroundColor Red
    exit 1
}

$aviItem = Get-Item $outputAvi
$aviSizeMB = [math]::Round($aviItem.Length / 1MB, 2)
Write-Host "  -> Convert hoan tat trong $($swVideo.Elapsed.ToString('mm\:ss')) ($aviSizeMB MB)" -ForegroundColor Green

# 5. Tong ket & Huong dan
Write-Host "==================================================================" -ForegroundColor Green
Write-Host " CONVERT HOAN TAT THANH CONG!" -ForegroundColor Green
Write-Host "==================================================================" -ForegroundColor Green
Write-Host "Ket qua tai: $OutputDir" -ForegroundColor White
Write-Host "  File: video.avi ($aviSizeMB MB) [All-in-One chua ca Video va Audio]" -ForegroundColor Cyan

Write-Host ""
Write-Host "HUONG DAN COPY VAO THE NHO:" -ForegroundColor White
Write-Host "  Copy DUY NHAT 1 file video.avi vao the nho MicroSD (FAT32):" -ForegroundColor White
Write-Host "  [SD_CARD]:\esp32-video\video.avi" -ForegroundColor Magenta
Write-Host "==================================================================" -ForegroundColor Green
