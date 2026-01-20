@echo off
chcp 65001 >nul
echo ===== 提交 Content 修改 =====
set /p msg="请输入提交说明: "

cd Content
git add .
git commit -m "%msg%"
git push

cd ..
git add Content
git commit -m "Update content: %msg%"
git push

echo.
echo ===== 提交完成 =====
pause
