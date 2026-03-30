@echo off
chcp 65001 >nul
echo ===== 提交 Content 修改 =====
echo.

:: ===== 远程地址配置（写死防止出错）=====
set GITHUB_REPO=https://github.com/PolyUCapstonPrj/CapstonePrj
set HF_REPO=https://huggingface.co/PolyUCapstoneContent/Content

:: ===== 提交前强制拉取最新（防止基于旧版本提交）=====
echo [安全检查] 提交前先拉取远程最新代码...
echo.

echo [拉取 Content]
cd Content
git fetch %HF_REPO% main
if errorlevel 1 (
    echo.
    echo [错误] 无法连接到 HuggingFace 远程仓库，请检查网络后重试！
    cd ..
    pause
    exit /b 1
)

:: 检查本地是否落后于远程
for /f %%i in ('git rev-list HEAD..FETCH_HEAD --count') do set BEHIND=%%i
if %BEHIND% GTR 0 (
    echo.
    echo [提示] 本地落后远程 %BEHIND% 个提交，正在自动拉取合并...
    git pull %HF_REPO% main
    if errorlevel 1 (
        echo.
        echo [错误] 拉取合并失败！请先手动解决冲突再提交。
        cd ..
        pause
        exit /b 1
    )
    echo [OK] 已同步到最新。
) else (
    echo [OK] 本地已是最新，无需拉取。
)
cd ..

echo.
echo [拉取主仓库]
git pull
if errorlevel 1 (
    echo.
    echo [错误] 主仓库拉取失败！请检查后重试。
    pause
    exit /b 1
)
echo.

:: ===== 开始提交 =====
set /p msg="请输入提交说明: "

echo.
echo [1/2] 提交 Content 到 HuggingFace (%HF_REPO%)
cd Content
git add .
git commit -m "%msg%"
if errorlevel 1 (
    echo [提示] Content 没有新的修改需要提交。
)
git push %HF_REPO% main
if errorlevel 1 (
    echo.
    echo [错误] 推送到 HuggingFace 失败！请检查网络后重试。
    cd ..
    pause
    exit /b 1
)

cd ..

:: 检查主仓库分支
for /f "tokens=*" %%a in ('git branch --show-current') do set main_branch=%%a
echo.
echo [2/2] 更新主仓库子模块引用 (%GITHUB_REPO%)
echo 当前主仓库分支：%main_branch%

git add Content
git commit -m "Update content: %msg%"
git push %GITHUB_REPO% %main_branch%
if errorlevel 1 (
    echo.
    echo [错误] 推送到 GitHub 失败！请检查网络后重试。
    pause
    exit /b 1
)

echo.
echo ===== 提交完成 =====
pause
