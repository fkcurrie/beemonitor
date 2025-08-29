# build_littlefs.py
Import("env")

env.Replace(PROGNAME="firmware")
env.Execute("/home/fcurrie/Projects/wokwi/venv/bin/platformio run -t buildfs")
