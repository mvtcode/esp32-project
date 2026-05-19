import os
import shutil

history_dir = r"C:\Users\macva\AppData\Roaming\Code\User\History"
dest_dir = r"d:\projects\esp32-project\src\ui\dashboard"

print(f"Searching VS Code History in: {history_dir}...")

if not os.path.exists(history_dir):
    print("VS Code History directory does not exist or is in a different path!")
    # Let's also check Local AppData or other typical paths
    alt_paths = [
        os.path.expandvars(r"%APPDATA%\Code\User\History"),
        os.path.expandvars(r"%USERPROFILE%\AppData\Roaming\Code\User\History"),
    ]
    for p in alt_paths:
        if os.path.exists(p):
            history_dir = p
            print(f"Found alternative path: {history_dir}")
            break
    else:
        print("Could not find any VS Code History folder.")
        exit(1)

# We want to find files that contain specific class signatures
signatures = {
    "home_screen.cpp": "HomeScreen::HomeScreen",
    "calendar_screen.cpp": "CalendarScreen::CalendarScreen",
    "player_screen.cpp": "PlayerScreen::PlayerScreen",
    "settings_screen.cpp": "SettingsScreen::SettingsScreen",
    "dashboard_ui.cpp": "DashboardUI::DashboardUI"
}

found_backups = {}

for root, dirs, files in os.walk(history_dir):
    for file in files:
        full_path = os.path.join(root, file)
        try:
            # Only read the first 1KB to be fast and safe
            with open(full_path, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read(2048)
                
            for key, sig in signatures.items():
                if sig in content:
                    # We found a potential backup! Let's get its size and modified time
                    mtime = os.path.getmtime(full_path)
                    if key not in found_backups or mtime > found_backups[key]["mtime"]:
                        found_backups[key] = {
                            "path": full_path,
                            "mtime": mtime,
                            "size": os.path.getsize(full_path)
                        }
        except Exception as e:
            continue

for name, info in found_backups.items():
    src_file = info["path"]
    if name == "dashboard_ui.cpp":
        dest_file = os.path.join(dest_dir, name)
    else:
        dest_file = os.path.join(dest_dir, "screens", name)
        
    os.makedirs(os.path.dirname(dest_file), exist_ok=True)
    shutil.copy2(src_file, dest_file)
    print(f"SUCCESSFULLY RESTORED {name} from {src_file} (Size: {info['size']} bytes, Time: {info['mtime']})")

if len(found_backups) < len(signatures):
    print(f"Only restored {len(found_backups)} out of {len(signatures)} files.")
