@echo off
chcp 65001 >nul
echo ===== 提交 Content 修改 =====
echo.

:: 检查主仓库是否在 main 分支
for /f "tokens=*" %%a in ('git branch --show-current') do set current_branch=%%a
if not "%current_branch%"=="main" (
    echo [错误] 主仓库当前在 %current_branch% 分支，请先切换到 main 分支！
    echo.
    echo 请运行：git checkout main
    pause
    exit /b 1
)

set /p msg="请输入提交说明: "

:: 提交 Content 到 HF
echo.
echo [1/2] 提交 Content 到 Hugging Face...
cd Content
git add .
git commit -m "%msg%"
git push origin main
cd ..

:: 更新主仓库的子模块引用
echo.
echo [2/2] 更新主仓库子模块引用...
git add Content
git commit -m "Update content: %msg%"
git push origin main

echo.
echo ===== 提交完成 =====
pause
