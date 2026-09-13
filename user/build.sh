#!/bin/bash
# =====================================================================
# LS2K0300 智能车巡线工程 —— 一键编译 + 上传脚本
#
# ★ 用法（两种）★
#   在工程 out/ 目录下执行：
#     ../user/build.sh                       # 使用脚本内默认板子 IP
#     ../user/build.sh 192.168.110.96        # 直接传板子 IP，不用改文件
#
# 流程：清理 out -> cmake(user/CMakeLists.txt) -> make -> 清理板子旧目录 -> scp 上传到板子
#
# 重要：本脚本必须在【开发虚拟机/有工具链的 Linux 机器】上执行，
#       不要在板子上执行（板子 buildroot 精简系统没有 cmake/make）。
#       编译依赖：/opt/ls_2k0300_env/loongson-gnu-toolchain-*/bin 与 opencv_4_10_build
# =====================================================================

# ===================== 核心配置区（一般不用改） =====================
WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"  # user 目录绝对路径
OUT_DIR="${WORK_DIR}/../out"                              # out 目录绝对路径
USER_DIR="${WORK_DIR}"                                    # user 目录绝对路径（CMakeLists 所在处）
REMOTE_IP="${1:-192.168.43.150}"                          # 板子 IP：可用命令行参数覆盖
REMOTE_USER="root"                                        # 板子登录用户（buildroot 默认 root）
REMOTE_PATH="/home/root/"                                 # 板子上传路径
MAKE_JOBS=$(nproc)                                        # make 编译线程数
RESERVE_FILE="本文件夹作用.txt"                            # out 中保留的文件，不删除

# ===================== 工具链自动探测 =====================
# 兼容工具链目录名差异（loongson-gnu-toolchain-*），找到即加入 PATH
TOOLCHAIN_DIR=$(ls -d /opt/ls_2k0300_env/loongson-gnu-toolchain-* 2>/dev/null | head -1)
if [ -n "${TOOLCHAIN_DIR}" ]; then
    export PATH="${TOOLCHAIN_DIR}/bin:${PATH}"
    info_echo "已加载工具链: ${TOOLCHAIN_DIR}"
fi

# ===================== 全局通用函数 =====================
error_exit() {
    echo -e "\033[31m[ERROR] $1\033[0m"
    exit 1
}
info_echo() {
    echo -e "\033[32m[INFO] $1\033[0m"
}

# ===================== 编译前环境检查 =====================
for cmd in cmake make loongarch64-linux-gnu-g++; do
    if ! command -v "${cmd}" >/dev/null 2>&1; then
        error_exit "缺少命令: ${cmd}。请确认本机是开发虚拟机且已配置 /opt/ls_2k0300_env 工具链！"
    fi
done
if [ ! -d /opt/ls_2k0300_env/opencv_4_10_build ]; then
    error_exit "缺少 OpenCV 交叉库: /opt/ls_2k0300_env/opencv_4_10_build"
fi

# ===================== 脚本主逻辑 =====================
# 1. 进入 out 目录
info_echo "准备进入目录: ${OUT_DIR}"
cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR} 目录，请检查目录是否存在！"

# 2. 清理 out 下所有内容，仅保留指定文件
info_echo "开始清理当前目录，仅保留 ${RESERVE_FILE}"
find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + || error_exit "目录清理失败，请检查目录权限！"

# 3. cmake 生成 Makefile
info_echo "执行cmake编译: cmake ${USER_DIR}"
cmake "${USER_DIR}" || error_exit "cmake 编译失败，请检查CMakeLists.txt或编译依赖！"

# 4. make 编译
info_echo "cmake执行成功，开始执行 make -j${MAKE_JOBS} 编译项目..."
make -j${MAKE_JOBS} || error_exit "make 编译失败，编译日志如上！"

# 5. 获取工程目录名（用于上传）
parent_dir_name=$(basename "$(dirname "$(pwd)")")
info_echo "待上传文件/目录：${parent_dir_name}"

# 6. 清理板子上旧的工程目录，避免 scp 目录嵌套
info_echo "清理板子上旧目录: ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}${parent_dir_name}"
ssh "${REMOTE_USER}@${REMOTE_IP}" "rm -rf '${REMOTE_PATH}${parent_dir_name}'" \
    || error_exit "清理板子旧目录失败，请检查板子 IP/密码/网络！"

# 7. SCP 上传整个工程目录到板子（用绝对路径源，避免相对路径找不到/误命中同名文件）
cd "${OUT_DIR}/.." || error_exit "无法进入工程根目录"
PROJ_DIR_ABS="$(pwd)"
info_echo "待上传目录：${PROJ_DIR_ABS}"
info_echo "开始上传文件到 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
scp -O -r "${PROJ_DIR_ABS}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" \
    || error_exit "文件上传失败，请检查网络/远程权限/文件是否存在！"

# 8. 全部执行完成
info_echo "所有操作执行完成：清理目录 -> cmake编译 -> make编译 -> 文件上传 均成功！"
info_echo "板子上运行：cd ${REMOTE_PATH}${parent_dir_name}/out && ./${parent_dir_name}"
exit 0
