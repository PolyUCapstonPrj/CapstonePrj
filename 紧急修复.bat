@echo off
chcp 65001 >nul
echo ===== 紧急修复（强制同步到远程最新版本）=====
echo.
echo 此脚本会：
echo   1. 强制同步主仓库到GitHub最新版本
echo   2. 强制同步Content子模块到HuggingFace最新版本
echo   3. 更新所有bat脚本到最新版本
echo.
echo ⚠️ 警告：此操作会丢弃所有本地未提交的修改！
echo.
set /p confirm="确认执行？(Y/N): "
if /i not "%confirm%"=="Y" goto END

:: ===== 远程地址配置（写死防止出错）=====
set GITHUB_REPO=https://github.com/PolyUCapstonPrj/CapstonePrj
set HF_REPO=https://huggingface.co/PolyUCapstoneContent/Content

echo.
echo [1/4] 修复主仓库远程地址...
git remote set-url origin %GITHUB_REPO%

echo.
echo [2/4] 强制同步主仓库（包括所有bat脚本）...
git fetch %GITHUB_REPO% main
git reset --hard FETCH_HEAD

echo.
echo [3/4] 同步子模块配置...
git submodule sync
git submodule update --init --force

echo.
echo [4/4] 强制同步 Content 子模块...
cd Content
git remote set-url origin %HF_REPO%
git fetch %HF_REPO% main
git reset --hard FETCH_HEAD
git lfs pull
cd ..

echo.
echo ===== 修复完成！=====
echo 所有脚本已更新到最新版本，现在可以正常使用了。
:END
pause
