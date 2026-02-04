@echo off
chcp 65001 >nul
echo ===== 拉取最新代码 =====

:: ===== 远程地址配置（写死防止出错）=====
set GITHUB_REPO=https://github.com/PolyUCapstonPrj/CapstonePrj
set HF_REPO=https://huggingface.co/PolyUCapstoneContent/Content

echo.
echo [主仓库] 拉取中... (%GITHUB_REPO%)
git pull %GITHUB_REPO% main

echo.
echo [Content子模块] 同步并更新中...
git submodule sync
git submodule update --init
cd Content
git checkout main
echo [Content子模块] 拉取中... (%HF_REPO%)
git pull %HF_REPO% main
git lfs pull
cd ..

echo.
echo ===== 拉取完成 =====
pause
