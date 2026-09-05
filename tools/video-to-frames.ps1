# ==============================================================================
# Script: video-to-frames.ps1
# Convert video MP4 -> video.avi / video.mjpeg (320x180 @ 20fps) & audio.wav (PCM 16-bit Mono)
# Usage: powershell -ExecutionPolicy Bypass -File .\tools\video-to-frames.ps1
# ==============================================================================

param (
    [string]$InputVideo = "$PSScriptRoot\video.mp4",
    [string]$OutputDir  = "$PSScriptRoot\out",
    [int]$Fps           = 20,     # 20 fps muot ma tren ESP32-S3 voi SIMD acceleration
    [int]$Width         = 320,
    [int]$Height        = 180,    # 320x180 ti le chuan 16:9, can giua man hinh 320x240
    [int]$Quality       = 7,      # 2-31: 7 cho frame ~8-12KB, ESP32-S3 decode trong ~20ms
    [int]$AudioRate     = 22050   # 22050 Hz Mono WAV PCM toi uu cho I2S DMA
)

Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host "   ESP32-S3 2.8 VIDEO & AUDIO CONVERTER (MJPEG + WAV PCM)         " -ForegroundColor Cyan
Write-Host "==================================================================" -ForegroundColor Cyan

# 1. Tim kiem cong cu FFmpeg
$ffmpeg = Join-Path $PSScriptRoot "ffmpeg.exe"
if (!(Test-Path $ffmpeg)) {
    $ffmpegCmd = Get-Command ffmpeg -ErrorAction SilentlyContinue
    if ($ffmpegCmd) {
        $ffmpeg = $ffmpegCmd.Source
    } else {
        Write-Host "[ERROR] Khong tim thay ffmpeg.exe tai: $ffmpeg" -ForegroundColor Red
        Write-Host "Vui long tai FFmpeg tu https://www.ffmpeg.org/ va dat vao folder tools/" -ForegroundColor Yellow
        exit 1
    }
}
Write-Host "[1/4] Tim thay FFmpeg: $ffmpeg" -ForegroundColor Green

# 2. Kiem tra file video dau vao
if (!(Test-Path $InputVideo)) {
    Write-Host "[ERROR] Khong tim thay video nguon tai: $InputVideo" -ForegroundColor Red
    exit 1
}

$videoItem = Get-Item $InputVideo
$videoSizeMB = [math]::Round($videoItem.Length / 1MB, 2)
Write-Host "[2/4] Video nguon: $($videoItem.Name) ($videoSizeMB MB)" -ForegroundColor Green

# 3. Tao thu muc output
if (!(Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

$outputAvi   = Join-Path $OutputDir "video.avi"
$outputMjpeg = Join-Path $OutputDir "video.mjpeg"
$outputWav   = Join-Path $OutputDir "audio.wav"

Write-Host "------------------------------------------------------------------" -ForegroundColor DarkGray
Write-Host "Cau hinh convert:" -ForegroundColor White
Write-Host "  - Do phan giai video : $($Width)x$($Height) (Ti le 16:9)" -ForegroundColor White
Write-Host "  - Frame rate         : $Fps fps" -ForegroundColor White
Write-Host "  - Chat luong JPEG    : q:v = $Quality" -ForegroundColor White
Write-Host "  - Dinh dang Audio    : PCM 16-bit Mono @ $AudioRate Hz (WAV khong nen)" -ForegroundColor White
Write-Host "  - Output Video AVI   : $outputAvi (Play duoc ca tren VLC va ESP32)" -ForegroundColor White
Write-Host "  - Output Video MJPEG : $outputMjpeg (Raw stream)" -ForegroundColor White
Write-Host "  - Output Audio       : $outputWav" -ForegroundColor White
Write-Host "------------------------------------------------------------------" -ForegroundColor DarkGray

# 4. Convert Video sang MJPEG (AVI container de VLC play duoc ngay, va file raw mjpeg)
Write-Host "[3/4] Dang convert Video sang Motion JPEG ($($Width)x$($Height) @ $($Fps)fps)..." -ForegroundColor Yellow
$swVideo = [System.Diagnostics.Stopwatch]::StartNew()

$scaleFilter = "scale=$($Width):$($Height):force_original_aspect_ratio=decrease,pad=$($Width):$($Height):(ow-iw)/2:(oh-ih)/2"

# Xuất file video.avi chứa CẢ Video Motion JPEG và Audio PCM WAV:
# -> VLC trên máy tính phát cả tiếng lẫn hình mượt mà
# -> ESP32 chỉ cần đọc 1 file duy nhất
& $ffmpeg -y -i $InputVideo -vf $scaleFilter -r $Fps -c:v mjpeg -q:v $Quality -c:a pcm_s16le -ar $AudioRate -ac 1 $outputAvi

$swVideo.Stop()

if ($LASTEXITCODE -ne 0 -or !(Test-Path $outputAvi)) {
    Write-Host "[ERROR] Convert video that bai! Exit code: $LASTEXITCODE" -ForegroundColor Red
    exit 1
}

$aviItem = Get-Item $outputAvi
$aviSizeMB = [math]::Round($aviItem.Length / 1MB, 2)
Write-Host "  -> Video AVI convert thanh cong trong $($swVideo.Elapsed.ToString('mm\:ss')) ($aviSizeMB MB)" -ForegroundColor Green

# 5. Trich xuat Audio sang WAV PCM Mono
Write-Host "[4/4] Dang trich xuat Audio sang WAV PCM Mono ($AudioRate Hz)..." -ForegroundColor Yellow
$swAudio = [System.Diagnostics.Stopwatch]::StartNew()

& $ffmpeg -y -i $InputVideo -vn -c:a pcm_s16le -ar $AudioRate -ac 1 $outputWav

$swAudio.Stop()

if ($LASTEXITCODE -ne 0 -or !(Test-Path $outputWav)) {
    Write-Host "[ERROR] Trich xuat audio that bai! Exit code: $LASTEXITCODE" -ForegroundColor Red
    exit 1
}

$wavItem = Get-Item $outputWav
$wavSizeMB = [math]::Round($wavItem.Length / 1MB, 2)
Write-Host "  -> Audio thanh cong trong $($swAudio.Elapsed.ToString('mm\:ss')) ($wavSizeMB MB)" -ForegroundColor Green

# 6. Tong ket & Bang thong
Write-Host "==================================================================" -ForegroundColor Green
Write-Host " CONVERT HOAN TAT THANH CONG!" -ForegroundColor Green
Write-Host "==================================================================" -ForegroundColor Green
Write-Host "Ket qua tai: $OutputDir" -ForegroundColor White
    Write-Host ("  1. video.avi   : {0:N2} MB  [ALL-IN-ONE: Chứa cả Video & Audio]" -f ($aviItem.Length / 1MB)) -ForegroundColor Cyan
    if (Test-Path $outputMjpeg) {
        Write-Host ("  2. video.mjpeg : {0:N2} MB  [Raw MJPEG stream]" -f ((Get-Item $outputMjpeg).Length / 1MB)) -ForegroundColor Cyan
    }
    Write-Host ("  3. audio.wav   : {0:N2} MB  [WAV PCM Mono @ 22.05kHz]" -f ($wavItem.Length / 1MB)) -ForegroundColor Cyan

# Uoc tinh bang thong the nho SD cho video ~58 giay
$durationSec = 57.6
$totalMB = ($aviItem.Length + $wavItem.Length) / 1MB
$mbPerSec = [math]::Round($totalMB / $durationSec, 2)
$kbPerSec = [math]::Round(($totalMB * 1024) / $durationSec, 1)

Write-Host ""
Write-Host "Uoc tinh bang thong doc the SD: ~$kbPerSec KB/s (~$mbPerSec MB/s)" -ForegroundColor Yellow
Write-Host "-> Bang thong nay nam trong gioi han toi uu cua the MicroSD SPI tren ESP32!" -ForegroundColor Green

Write-Host ""
Write-Host "HUONG DAN COPY VAO THE NHO:" -ForegroundColor White
Write-Host "  Copy file video va audio vao thu muc tren the nho MicroSD (dinh dang FAT32):" -ForegroundColor White
Write-Host "  [SD_CARD]:\esp32-video\video.avi   (hoac video.mjpeg)" -ForegroundColor Magenta
Write-Host "  [SD_CARD]:\esp32-video\audio.wav" -ForegroundColor Magenta
Write-Host "==================================================================" -ForegroundColor Green
