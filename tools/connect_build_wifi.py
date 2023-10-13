import os
import time

Import("env")
config = env.GetProjectConfig()

password = config.get("custom", "esp_wifi_password")
ssid = config.get("custom", "esp_wifi_ssid")

print("=> Trying to connect to WiFi %s..." % ssid)

while os.popen("m wifi status").read().find(ssid) == -1:
    print("=> WiFi not found, retrying...")

    os.system("m wifi connect %s %s" % (ssid, password))


