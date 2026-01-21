@echo off
chcp 65001 >nul
echo ===== 提交 Content 修改 =====
echo.

set /p msg="请输入提交说明: "

cd Content
git add .
git commit -m "%msg%"
git push origin main

cd ..

:: 检查主仓库分支
for /f "tokens=*" %%a in ('git branch --show-current') do set main_branch=%%a
echo.
echo 当前主仓库分支：%main_branch%

git add Content
git commit -m "Update content: %msg%"
git push origin %main_branch%

echo.
echo ===== 提交完成 =====
pause
