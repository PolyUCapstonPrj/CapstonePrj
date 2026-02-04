@echo off
chcp 65001 >nul
echo ===== 拉取最新代码 =====

echo.
echo [主仓库] 拉取中...
git pull

echo.
echo [Content子模块] 更新中...
git submodule update --init
cd Content
git checkout main
git pull origin main
git lfs pull
cd ..

echo.
echo ===== 拉取完成 =====
pause
