import os
Import("env")
print("Connect to OTA wifi (u701)...")
config = env.GetProjectConfig()
password = config.get("custom", "esp_wifi_password")
os.system("m wifi connect u701 " + password)
