@echo off
chcp 65001 >nul
echo ===== 拉取最新代码 =====

git pull
git submodule update --init
cd Content
git pull
git lfs pull
cd ..

echo.
echo ===== 拉取完成 =====
pause
