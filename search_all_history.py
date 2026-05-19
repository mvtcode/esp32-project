import os
import shutil

history_dir = os.path.expandvars(r"%APPDATA%\Code\User\History")
if not os.path.exists(history_dir):
    history_dir = r"C:\Users\macva\AppData\Roaming\Code\User\History"

print(f"Deep scanning history in: {history_dir}")

target_names = ["home_screen.cpp", "calendar_screen.cpp", "player_screen.cpp", "settings_screen.cpp", "dashboard_ui.cpp"]
found = {}

# Let's count total subfolders first
if os.path.exists(history_dir):
    subdirs = os.listdir(history_dir)
    print(f"Total subdirectories in history: {len(subdirs)}")
    
    # We walk the history folder
    for root, dirs, files in os.walk(history_dir):
        for file in files:
            full_path = os.path.join(root, file)
            # Check if there is an entries.json which maps the actual filenames!
            # VS Code stores history in subdirectories with names like '1a2b3c4d' and the files themselves have random names,
            # but there is an 'entries.json' file in each subfolder or in the history folder that maps them!
            # Or the file might have the content we want. Let's search by reading the full content.
            try:
                with open(full_path, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                
                # Check for unique class keywords anywhere in the file
                for name in target_names:
                    sig = ""
                    if name == "home_screen.cpp":
                        sig = "HomeScreen::HomeScreen"
                    elif name == "calendar_screen.cpp":
                        sig = "CalendarScreen::CalendarScreen"
                    elif name == "player_screen.cpp":
                        sig = "PlayerScreen::PlayerScreen"
                    elif name == "settings_screen.cpp":
                        sig = "SettingsScreen::SettingsScreen"
                    elif name == "dashboard_ui.cpp":
                        sig = "DashboardUI::DashboardUI"
                    
                    if sig in content:
                        mtime = os.path.getmtime(full_path)
                        if name not in found or mtime > found[name]["mtime"]:
                            found[name] = {
                                "path": full_path,
                                "mtime": mtime,
                                "size": len(content)
                            }
            except Exception as e:
                continue

    # Let's see what we found
    for name, info in found.items():
        dest_dir = r"d:\projects\esp32-project\src\ui\dashboard"
        if name == "dashboard_ui.cpp":
            dest_file = os.path.join(dest_dir, name)
        else:
            dest_file = os.path.join(dest_dir, "screens", name)
            
        os.makedirs(os.path.dirname(dest_file), exist_ok=True)
        shutil.copy2(info["path"], dest_file)
        print(f"SUCCESS: Restored {name} (size {info['size']} bytes) from {info['path']}")
else:
    print("History directory does not exist.")
