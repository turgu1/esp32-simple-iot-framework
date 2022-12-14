Import("env")
print("Replacing MKSPIFFSTOOL with mklittlefs")
env.Replace(MKFSTOOL = env.get("PROJECT_DIR") + '/mklittlefs')