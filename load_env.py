import os
from pathlib import Path

Import("env")

def load_env_file(path):
    if not path.exists():
        return
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if "=" in line:
                key, value = line.split("=", 1)
                os.environ[key.strip()] = value.strip()

load_env_file(Path(".env"))
load_env_file(Path(".env.local"))

def is_enabled(value: str) -> bool:
    return value.lower() in ["1", "true", "yes", "on"]

def add_define(name: str, value=None, numeric=False):
    if value is None:
        env.AppendUnique(BUILD_FLAGS=[f"-D{name}"])
    else:
        if numeric:
            env.AppendUnique(BUILD_FLAGS=[f"-D{name}={value}"])
        else:
            env.AppendUnique(BUILD_FLAGS=[f'-D{name}=\\"{value}\\"'])

FEATURES = [
    "INCLUDE_MQTT",
    "INCLUDE_IR",
    "INCLUDE_OTA",
]

STRING_VARS = [
    "ENV_MQTT_HOST",
    "ENV_MQTT_UID",
    "ENV_MQTT_USER",
    "ENV_MQTT_PASS",
    "ENV_WIFI_SSID",
    "ENV_WIFI_PASS",
    "ENV_HOSTNAME",
    "ENV_OTAPASSWORD",
]

NUMERIC_VARS = [
    "ENV_MQTT_PORT",
]

UPLOAD_OTA = os.environ.get("UPLOAD_OTA", "0") in ["1", "true", "yes", "on"]
if UPLOAD_OTA:
    hostname = os.environ.get("ENV_HOSTNAME", "logitech-z906")
    otapassword = os.environ.get("ENV_OTAPASSWORD", "password")

    if not hostname.endswith(".local"):
        hostname += ".local"
    
    env.Replace(
        UPLOAD_PROTOCOL="espota",
        UPLOAD_PORT=hostname,
        UPLOAD_FLAGS=[f"--auth={otapassword}"]
    )

for feature in FEATURES:
    if is_enabled(os.environ.get(feature, "0")):
        add_define(feature)

for var in STRING_VARS:
    if var in os.environ:
        add_define(var, os.environ[var])

for var in NUMERIC_VARS:
    if var in os.environ:
        add_define(var, os.environ[var], numeric=True)

print("=== BUILD FLAGS ADDED ===")
for flag in env.get("BUILD_FLAGS", []):
    print(flag)
