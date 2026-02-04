@echo off
chcp 65001 >nul
echo ===== 拉取最新代码 =====

:: ===== 远程地址配置（写死防止出错）=====
set GITHUB_REPO=https://github.com/PolyUCapstonPrj/CapstonePrj
set HF_REPO=https://huggingface.co/PolyUCapstoneContent/Content

echo.
echo [主仓库] 拉取中... (%GITHUB_REPO%)
git fetch %GITHUB_REPO% main
git reset --hard FETCH_HEAD

echo.
echo [Content子模块] 同步并更新中...
git submodule sync
git submodule update --init --force
cd Content
git fetch %HF_REPO% main
git reset --hard FETCH_HEAD
git lfs pull
cd ..

echo.
echo ===== 拉取完成 =====
pause
