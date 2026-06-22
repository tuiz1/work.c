#!/bin/bash
# ==========================================
# oslab 快捷命令 —— 在 Ubuntu 容器中执行编译/运行
# 用法:
#   ./oslab.sh gcc -o myapp myapp.c    # 编译
#   ./oslab.sh ./myapp                 # 运行
#   ./oslab.sh gdb ./myapp             # 调试
#   ./oslab.sh bash                    # 进入交互 shell
# ==========================================

CONTAINER="oslab"
docker exec -it -w /work "$CONTAINER" "$@"
