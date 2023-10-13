import os
Import("env")

def after_build(source, target, env):
    print("=> After build: COnnecting to ESP32...")

    config = env.GetProjectConfig()

    password = config.get("custom", "esp_wifi_password")
    ssid = config.get("custom", "esp_wifi_ssid")

    print("=> Trying to connect to WiFi %s..." % ssid)

    while os.popen("m wifi status").read().find(ssid) == -1:
        print("=> WiFi not found, retrying...")
        os.system("m wifi connect %s %s" % (ssid, password))

def after_upload(source, target, env):
    print("=> After upload: Resoring home WiFi...")

    config = env.GetProjectConfig()

    wifi_password = config.get("custom", "home_wifi_password")
    wifi_name = config.get("custom", "home_wifi_name")

    os.system("m wifi connect %s %s" % (wifi_name, wifi_password))

env.AddPostAction("$UPLOADCMD", after_upload)
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)
