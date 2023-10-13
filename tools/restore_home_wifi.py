import os
Import("env")

def after_upload(source, target, env):
    print("=> Post upload: Resoring home WiFi...")
    config = env.GetProjectConfig()

    wifi_password = config.get("custom", "home_wifi_password")
    wifi_name = config.get("custom", "home_wifi_name")

    os.system("m wifi connect %s %s" % (wifi_name, wifi_password))

env.AddPostAction("$UPLOADCMD", after_upload)
