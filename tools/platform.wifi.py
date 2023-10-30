import os
Import("env")

def connect_to_esp32_wifi(source, target, env):
    print("=> After build: Connecting to ESP32...")

    config = env.GetProjectConfig()

    password = config.get("custom", "esp_wifi_password")
    ssid = config.get("custom", "esp_wifi_ssid")

    print("=> Trying to connect to WiFi %s..." % ssid)

    while os.popen("m wifi status").read().find(ssid) == -1:
        print("=> WiFi not found, retrying...")
        os.system("m wifi connect %s %s" % (ssid, password))

def restore_wifi(source, target, env):
    print("=> After upload: Restoring home WiFi...")

    config = env.GetProjectConfig()

    wifi_password = config.get("custom", "home_wifi_password")
    wifi_name = config.get("custom", "home_wifi_name")

    os.system("m wifi connect %s %s" % (wifi_name, wifi_password))

env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", connect_to_esp32_wifi)
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", connect_to_esp32_wifi)
env.AddPostAction("$UPLOADCMD", restore_wifi)
