@echo off
chcp 65001 >nul
echo ===== 提交 Content 修改 =====
echo.

:: ===== 远程地址配置（写死防止出错）=====
set GITHUB_REPO=https://github.com/PolyUCapstonPrj/CapstonePrj
set HF_REPO=https://huggingface.co/PolyUCapstoneContent/Content

set /p msg="请输入提交说明: "

echo.
echo [1/2] 提交 Content 到 HuggingFace (%HF_REPO%)
cd Content
git add .
git commit -m "%msg%"
git push %HF_REPO% main

cd ..

:: 检查主仓库分支
for /f "tokens=*" %%a in ('git branch --show-current') do set main_branch=%%a
echo.
echo [2/2] 更新主仓库子模块引用 (%GITHUB_REPO%)
echo 当前主仓库分支：%main_branch%

git add Content
git commit -m "Update content: %msg%"
git push %GITHUB_REPO% %main_branch%

echo.
echo ===== 提交完成 =====
pause
