@echo off
chcp 65001 >nul
echo ===== 拉取最新代码 =====

echo.
echo [主仓库] 拉取中...
git pull

echo.
echo [Content] 拉取中...
cd Content
git checkout main
git pull
git lfs pull
cd ..

echo.
echo ===== 拉取完成 =====
pause
