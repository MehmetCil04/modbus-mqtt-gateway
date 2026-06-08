# PlatformIO post-build hook: copies slave firmware artifacts into
# ../firmware/wokwi-extra so the Wokwi diagram in the master project
# can reference them as wokwi-extra/slave.bin (Wokwi resolves bootloader
# and partitions from the same folder).
#
# Runs automatically after every `pio run -e wokwi-slave`.

import os
import shutil

Import("env")


def copy_to_wokwi(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    dest = os.path.normpath(
        os.path.join(project_dir, "..", "firmware", "wokwi-extra")
    )
    os.makedirs(dest, exist_ok=True)

    mapping = {
        "firmware.bin": "slave.bin",
        "firmware.elf": "slave.elf",
        "bootloader.bin": "bootloader.bin",
        "partitions.bin": "partitions.bin",
    }
    for src_name, dst_name in mapping.items():
        src_path = os.path.join(build_dir, src_name)
        if os.path.exists(src_path):
            shutil.copy2(src_path, os.path.join(dest, dst_name))
            print(f"[wokwi-copy] {src_name} -> {dst_name}")

    print(f"[wokwi-copy] slave artefacts ready in {dest}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_to_wokwi)
