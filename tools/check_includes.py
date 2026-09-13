# -*- coding: utf-8 -*-
"""检查 LS2K0300_SmartCar 工程源码 include 引用完整性（仅扫描自写代码与关键库头文件）。"""
import os
import re
import sys

ROOT = r"C:\Users\dimidimi\Desktop\LS2K0300_SmartCar"
SEARCH_DIRS = [
    os.path.join(ROOT, "user"),
    os.path.join(ROOT, "code"),
    os.path.join(ROOT, "model"),
    os.path.join(ROOT, "libraries"),
]

# 系统/三方头文件，允许缺失（由编译环境提供）
ALLOW_MISSING = {
    "opencv2/opencv.hpp",
    "cstdio", "cstdlib", "csignal", "cstring", "cmath",
    "string", "vector", "cstdint", "cstddef",
}

issues = []

def resolve(name):
    for d in SEARCH_DIRS:
        for dirpath, _, filenames in os.walk(d):
            if name in filenames:
                return os.path.join(dirpath, name)
    return None

# 收集所有源文件
src_files = []
for d in ("user", "code"):
    dpath = os.path.join(ROOT, d)
    for f in os.listdir(dpath):
        if f.endswith((".cpp", ".hpp", ".h")):
            src_files.append(os.path.join(dpath, f))

include_re = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')

for f in src_files:
    with open(f, "r", encoding="utf-8", errors="ignore") as fh:
        for lineno, line in enumerate(fh, 1):
            m = include_re.match(line)
            if not m:
                continue
            inc = m.group(1)
            if inc in ALLOW_MISSING:
                continue
            if not resolve(inc):
                issues.append(f"{os.path.relpath(f, ROOT)}:{lineno}: 找不到头文件 <{inc}>")

if issues:
    print("发现问题：")
    for i in issues:
        print("  " + i)
    sys.exit(1)
else:
    print("include 完整性检查通过：所有引用的头文件均可解析。")
