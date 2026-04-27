#!/bin/bash
#
# RealtimeCollabEngine 生产环境部署脚本
# 使用方法: ./deploy.sh [install|start|stop|restart|status|logs]
#

set -e

APP_NAME="collab-engine"
APP_DIR="/opt/collab-engine"
SERVICE_FILE="/etc/systemd/system/${APP_NAME}.service"
LOG_DIR="${APP_DIR}/logs"
DATA_DIR="${APP_DIR}/data"
CONFIG_FILE="${APP_DIR}/config.json"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查root权限
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "此脚本需要root权限运行"
        exit 1
    fi
}

# 安装应用
install() {
    log_info "开始安装 ${APP_NAME}..."
    
    # 创建应用目录
    mkdir -p ${APP_DIR}
    mkdir -p ${LOG_DIR}
    mkdir -p ${DATA_DIR}
    
    # 复制文件
    cp -r * ${APP_DIR}/
    
    # 构建项目
    cd ${APP_DIR}
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    
    # 创建用户
    if ! id -u collab >/dev/null 2>&1; then
        useradd -r -s /bin/false collab
        log_info "创建系统用户 collab"
    fi
    
    # 设置权限
    chown -R collab:collab ${APP_DIR}
    chmod 755 ${APP_DIR}/build/bin/RealtimeCollabEngine
    
    # 安装systemd服务
    cp ${APP_DIR}/deploy/${APP_NAME}.service ${SERVICE_FILE}
    systemctl daemon-reload
    systemctl enable ${APP_NAME}
    
    log_info "安装完成！"
    log_info "使用 'systemctl start ${APP_NAME}' 启动服务"
}

# 启动服务
start() {
    log_info "启动 ${APP_NAME}..."
    systemctl start ${APP_NAME}
    log_info "服务已启动"
}

# 停止服务
stop() {
    log_info "停止 ${APP_NAME}..."
    systemctl stop ${APP_NAME}
    log_info "服务已停止"
}

# 重启服务
restart() {
    log_info "重启 ${APP_NAME}..."
    systemctl restart ${APP_NAME}
    log_info "服务已重启"
}

# 查看状态
status() {
    systemctl status ${APP_NAME}
}

# 查看日志
logs() {
    journalctl -u ${APP_NAME} -f
}

# 卸载应用
uninstall() {
    log_warn "即将卸载 ${APP_NAME}，此操作不可逆！"
    read -p "确认卸载？(yes/no): " confirm
    
    if [[ $confirm == "yes" ]]; then
        systemctl stop ${APP_NAME}
        systemctl disable ${APP_NAME}
        rm -f ${SERVICE_FILE}
        systemctl daemon-reload
        rm -rf ${APP_DIR}
        userdel -r collab 2>/dev/null || true
        log_info "卸载完成"
    else
        log_info "取消卸载"
    fi
}

# 显示帮助
show_help() {
    echo "用法: $0 {install|start|stop|restart|status|logs|uninstall}"
    echo ""
    echo "命令:"
    echo "  install   - 安装应用"
    echo "  start     - 启动服务"
    echo "  stop      - 停止服务"
    echo "  restart   - 重启服务"
    echo "  status    - 查看服务状态"
    echo "  logs      - 查看服务日志"
    echo "  uninstall - 卸载应用"
}

# 主函数
main() {
    check_root
    
    case "$1" in
        install)
            install
            ;;
        start)
            start
            ;;
        stop)
            stop
            ;;
        restart)
            restart
            ;;
        status)
            status
            ;;
        logs)
            logs
            ;;
        uninstall)
            uninstall
            ;;
        *)
            show_help
            exit 1
            ;;
    esac
}

main "$@"
