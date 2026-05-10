import wave
import struct
import sys
import os
import math

def analyze_wav(filename):
    if not os.path.exists(filename):
        print(f"File not found: {filename}")
        return

    with wave.open(filename, 'rb') as wav:
        params = wav.getparams()
        nframes = params.nframes
        sampwidth = params.sampwidth
        framerate = params.framerate
        
        print(f"\n--- ANALYZING: {os.path.basename(filename)} ---")
        print(f"Frames: {nframes}, Rate: {framerate}Hz, Channels: {params.nchannels}")
        
        raw_data = wav.readframes(nframes)
        # Assuming 16-bit mono
        samples = struct.unpack(f"<{nframes}h", raw_data)
        
        if not samples:
            print("No audio data found.")
            return

        # Basic Stats
        max_val = max(samples)
        min_val = min(samples)
        abs_max = max(abs(max_val), abs(min_val))
        avg_abs = sum(abs(s) for s in samples) / len(samples)
        
        # RMS Calculation
        sum_sq = sum(float(s)**2 for s in samples)
        rms = math.sqrt(sum_sq / len(samples))
        
        print(f"Peak Amplitude: {abs_max} ({(abs_max/32768)*100:.1f}% of FS)")
        print(f"Average Level: {avg_abs:.1f}")
        print(f"RMS Energy: {rms:.1f}")
        
        if abs_max < 100:
            print("[!] WARNING: Signal is EXTREMELY quiet. Check wiring or mic power.")
        elif abs_max > 32000:
            print("[!] WARNING: Signal is CLIPPING. Audio will sound distorted.")

        # DC Offset check
        dc_offset = sum(samples) / len(samples)
        print(f"DC Offset: {dc_offset:.1f}")

        # ASCII Waveform (simplified)
        print("\n--- WAVEFORM ENVELOPE (ASCII) ---")
        rows = 10
        cols = 60
        chunk_size = len(samples) // cols
        if chunk_size < 1: chunk_size = 1
        
        for r in range(rows, -rows-1, -1):
            line = ""
            threshold = (r / rows) * 32768
            for c in range(cols):
                chunk = samples[c*chunk_size : (c+1)*chunk_size]
                if not chunk: continue
                val = max(chunk) if r >= 0 else min(chunk)
                
                if r == 0:
                    line += "-"
                elif (r > 0 and val >= threshold) or (r < 0 and val <= threshold):
                    line += "#"
                else:
                    line += " "
            print(line)
        print("-" * cols)

if __name__ == "__main__":
    folder = "tools/records"
    if not os.path.exists(folder):
        print(f"No records folder found at {folder}")
        sys.exit(1)
        
    files = [f for f in os.listdir(folder) if f.endswith(".wav")]
    if not files:
        print("No .wav files found in records folder.")
    else:
        # Sort by creation time to get the latest
        files.sort(key=lambda x: os.path.getmtime(os.path.join(folder, x)), reverse=True)
        latest_file = os.path.join(folder, files[0])
        analyze_wav(latest_file)
