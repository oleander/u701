import os
Import("env")

def after_upload(source, target, env):
    print("Post upload: Switching WiFi")
    config = env.GetProjectConfig()

    wifi_password = config.get("custom", "home_wifi_password")
    wifi_name = config.get("custom", "home_wifi_name")
    os.system("m wifi connect " + wifi_name + " " + wifi_password)

# Custom upload command
env.AddPostAction("$UPLOADCMD", after_upload)
