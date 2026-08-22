# AGENTS.md

## Project Rules

### PlatformIO Execution Rules
- **Do NOT automatically run build, upload, or monitor commands**: Never execute commands such as `pio run`, `pio run -t upload`, `pio device monitor`, or related flashing/monitoring scripts (`upload.sh`, `monitor.sh`) via shell tools without explicit user request.
- **Instruct the User**: When code changes are made and compilation/testing is needed, output the exact commands for the user to execute manually in their terminal.
- **Exception**: Only run these commands if the user explicitly asks you to run/build/upload/monitor.
