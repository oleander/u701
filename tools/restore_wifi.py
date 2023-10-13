import os
Import("env")

def after_upload(source, target, env):
    print("Post upload: Switching WiFi")
    os.system("m wifi connect boat")

# Custom upload command
env.AddPostAction("$UPLOADCMD", after_upload)
