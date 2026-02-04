@echo off
chcp 65001 >nul
echo ===== 回滚 Content 修改 =====
echo.
echo 此脚本用于撤销最近的 Content 提交
echo.

:: 显示最近5次提交
echo [最近的 Content 提交记录]
echo ─────────────────────────────────────────
cd Content
git --no-pager log --oneline -5
cd ..
echo ─────────────────────────────────────────
echo.

:: 选择回滚方式
echo 请选择回滚方式：
echo   1. 撤销最近一次提交（推荐）
echo   2. 撤销最近N次提交
echo   3. 回滚到指定提交
echo   4. 取消
echo.
set /p choice="请输入选项 (1/2/3/4): "

if "%choice%"=="1" goto ROLLBACK_ONE
if "%choice%"=="2" goto ROLLBACK_N
if "%choice%"=="3" goto ROLLBACK_HASH
if "%choice%"=="4" goto CANCEL
echo [错误] 无效选项
pause
exit /b 1

:ROLLBACK_ONE
set count=1
goto DO_ROLLBACK

:ROLLBACK_N
set /p count="请输入要撤销的提交数量: "
goto DO_ROLLBACK

:ROLLBACK_HASH
set /p target_hash="请输入目标 commit hash (前7位即可): "
echo.
echo [1/3] 回滚 Content 子模块...
cd Content
git reset --hard %target_hash%
if errorlevel 1 (
    echo [错误] 回滚失败，请检查 commit hash 是否正确
    cd ..
    pause
    exit /b 1
)
git push origin main --force
cd ..
goto UPDATE_MAIN

:DO_ROLLBACK
echo.
echo [1/3] 回滚 Content 子模块（撤销最近 %count% 次提交）...
cd Content
git reset --hard HEAD~%count%
if errorlevel 1 (
    echo [错误] 回滚失败
    cd ..
    pause
    exit /b 1
)
git push origin main --force
cd ..

:UPDATE_MAIN
echo.
echo [2/3] 更新主仓库子模块引用...
git add Content
git commit -m "Rollback content"
git push origin main

echo.
echo [3/3] 同步本地文件...
git submodule update --init --recursive

echo.
echo ===== 回滚完成 =====
echo.
echo 请重新打开 UE 编辑器查看效果
pause
exit /b 0

:CANCEL
echo 已取消
pause
exit /b 0
