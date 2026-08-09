#!/usr/bin/env bash
#
# clang-format 格式化脚本
# 用法:
#   ./format.sh          格式化所有源文件 (in-place)
#   ./format.sh check    仅检查，不修改文件 (CI 模式)
#   ./format.sh diff     显示差异，不修改文件
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"

# 需要格式化的文件扩展名
EXTENSIONS=("h" "hpp" "c" "cpp" "cc" "cxx")

# 排除的目录
EXCLUDE_DIRS=("build" "third_party" "external")

# --------------------------------------------------
# 检查 clang-format 是否可用
# --------------------------------------------------
check_clang_format() {
    if command -v clang-format &>/dev/null; then
        return 0
    fi

    echo "❌ clang-format 未安装"
    echo ""
    echo "安装方式:"
    echo "  Ubuntu/Debian: sudo apt install clang-format"
    echo "  或者指定版本:   sudo apt install clang-format-14"
    echo ""
    echo "如果安装了特定版本，可创建软链接:"
    echo "  sudo ln -s /usr/bin/clang-format-14 /usr/local/bin/clang-format"
    exit 1
}

# --------------------------------------------------
# 收集需要格式化的文件
# --------------------------------------------------
collect_files() {
    local exclude_args=()
    for dir in "${EXCLUDE_DIRS[@]}"; do
        exclude_args+=(-path "*/$dir/*" -prune -o)
    done

    local ext_args=()
    for ext in "${EXTENSIONS[@]}"; do
        ext_args+=(-name "*.$ext" -o)
    done
    # 去掉最后一个 -o
    ext_args=("${ext_args[@]:0:${#ext_args[@]}-1}")

    find "$PROJECT_DIR" "${exclude_args[@]}" -type f \( "${ext_args[@]}" \) -print
}

# --------------------------------------------------
# 格式化所有文件
# --------------------------------------------------
do_format() {
    echo "🔧 格式化中..."
    local files
    files=$(collect_files)

    if [[ -z "$files" ]]; then
        echo "⚠️  未找到需要格式化的文件"
        return
    fi

    local count
    count=$(echo "$files" | wc -l)
    echo "$files" | xargs clang-format -i -style=file
    echo "✅ 已格式化 $count 个文件"
}

# --------------------------------------------------
# 检查格式 (不修改)
# --------------------------------------------------
do_check() {
    echo "🔍 检查格式..."
    local files
    files=$(collect_files)

    if [[ -z "$files" ]]; then
        echo "⚠️  未找到需要格式化的文件"
        return 0
    fi

    local has_diff=0
    while IFS= read -r file; do
        if ! diff -q <(clang-format -style=file "$file") "$file" &>/dev/null; then
            echo "  ❌ $file"
            has_diff=1
        fi
    done <<< "$files"

    if [[ $has_diff -eq 0 ]]; then
        echo "✅ 所有文件格式正确"
        return 0
    else
        echo ""
        echo "❌ 存在格式不一致的文件，请运行 ./format.sh 修复"
        return 1
    fi
}

# --------------------------------------------------
# 显示差异 (不修改)
# --------------------------------------------------
do_diff() {
    echo "📊 显示格式差异..."
    local files
    files=$(collect_files)

    if [[ -z "$files" ]]; then
        echo "⚠️  未找到需要格式化的文件"
        return
    fi

    local has_diff=0
    while IFS= read -r file; do
        local diff_output
        diff_output=$(diff -u --color=always "$file" <(clang-format -style=file "$file") 2>/dev/null || true)
        if [[ -n "$diff_output" ]]; then
            echo "$diff_output"
            has_diff=1
        fi
    done <<< "$files"

    if [[ $has_diff -eq 0 ]]; then
        echo "✅ 所有文件格式正确"
    fi
}

# --------------------------------------------------
# 主入口
# --------------------------------------------------
main() {
    check_clang_format

    local mode="${1:-format}"

    case "$mode" in
        format|"")
            do_format
            ;;
        check)
            do_check
            ;;
        diff)
            do_diff
            ;;
        *)
            echo "用法: $0 [format|check|diff]"
            echo ""
            echo "  format  格式化所有源文件 (默认)"
            echo "  check   仅检查格式是否正确"
            echo "  diff    显示格式差异"
            exit 1
            ;;
    esac
}

main "$@"
