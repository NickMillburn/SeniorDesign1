Import("env")
import os

PROJECT_DIR = env.subst("$PROJECT_DIR")
ENV_PATH = os.path.join(PROJECT_DIR, ".env")

values = {}
if os.path.exists(ENV_PATH):
    with open(ENV_PATH, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            values[key.strip()] = value.strip().strip('"').strip("'")

api_key = values.get("SENDGRID_API_KEY")
if api_key:
    # Inject as a quoted C string literal: -DSENDGRID_API_KEY="..."
    escaped = api_key.replace("\\", "\\\\").replace('"', '\\"')
    env.Append(BUILD_FLAGS=[f'-DSENDGRID_API_KEY=\\"{escaped}\\"'])
