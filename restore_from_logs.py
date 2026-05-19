import json
import os

log_path = r"C:\Users\macva\.gemini\antigravity\brain\6ac83617-390f-486b-8952-284fbb020d7a\.system_generated\logs\overview.txt"
dest_dir = r"d:\projects\esp32-project"

files_to_restore = {
    "home_screen.cpp": r"src\ui\dashboard\screens\home_screen.cpp",
    "calendar_screen.cpp": r"src\ui\dashboard\screens\calendar_screen.cpp",
    "player_screen.cpp": r"src\ui\dashboard\screens\player_screen.cpp",
    "settings_screen.cpp": r"src\ui\dashboard\screens\settings_screen.cpp",
    "dashboard_ui.cpp": r"src\ui\dashboard\dashboard_ui.cpp"
}

restored_contents = {}

print(f"Reading logs from {log_path}...")
with open(log_path, "r", encoding="utf-8", errors="ignore") as f:
    for line_idx, line in enumerate(f, 1):
        line = line.strip()
        if not line:
            continue
        
        # Remove line number prefix if present
        # Format of overview.txt lines shown in view_file: "<line_number>: <json_data>"
        # Let's strip the prefix "<number>: " if it exists
        if ":" in line:
            parts = line.split(":", 1)
            # Check if the prefix is indeed a number
            if parts[0].strip().isdigit():
                json_str = parts[1].strip()
            else:
                json_str = line
        else:
            json_str = line

        try:
            step_data = json.loads(json_str)
        except Exception as e:
            continue
        
        tool_calls = step_data.get("tool_calls", [])
        if not tool_calls:
            continue
            
        for tc in tool_calls:
            name = tc.get("name")
            args = tc.get("args", {})
            
            # We look for write_to_file or replace_file_content calls targeting our files
            if name in ["write_to_file", "replace_file_content"]:
                target_file = args.get("TargetFile", "")
                # Clean up target_file quotes and escapes
                target_file = target_file.strip('"').replace('\\\\', '\\')
                
                # Check which of our files this target_file is
                for filename, dest_rel_path in files_to_restore.items():
                    if filename in target_file:
                        content = args.get("CodeContent", args.get("ReplacementContent", ""))
                        content = content.strip('"')
                        # Keep the most recent code content
                        restored_contents[filename] = content
                        print(f"Found content for {filename} in line {line_idx} ({len(content)} chars)")

# Now write the restored files back
for filename, dest_rel_path in files_to_restore.items():
    if filename in restored_contents:
        full_dest_path = os.path.join(dest_dir, dest_rel_path)
        os.makedirs(os.path.dirname(full_dest_path), exist_ok=True)
        # Decode typical double escaped JSON strings like \n, \", \\
        raw_content = restored_contents[filename]
        decoded = raw_content.replace("\\n", "\n").replace('\\"', '"').replace("\\\\", "\\").replace("\\t", "\t")
        
        with open(full_dest_path, "w", encoding="utf-8") as out:
            out.write(decoded)
        print(f"RESTORED: {dest_rel_path} ({len(decoded)} chars)")
    else:
        print(f"WARNING: No content found in logs to restore for {filename}")
