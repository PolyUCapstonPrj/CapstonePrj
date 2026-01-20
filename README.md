# CapstonePrj 项目使用说明

## 项目结构

本项目使用 **GitHub + Hugging Face** 的组合方式管理：
- **GitHub**：存放代码（Source、Config、Plugins）
- **Hugging Face**：存放资源（Content 文件夹，包括蓝图、模型、材质等）

Content 文件夹是一个 Git 子模块，需要额外的命令来同步。

---

## 首次克隆项目

**不要用 GitHub Desktop 克隆**，请用命令行：

1. 打开 PowerShell 或 CMD
2. 进入你想存放项目的目录，比如 `E:\UnrealProjects`
3. 运行以下命令：

```bash
git clone --recurse-submodules https://github.com/PolyUCapstonPrj/CapstonePrj.git
cd CapstonePrj\Content
git lfs pull
```

等待下载完成（Content 约 4.5GB）。

---

## 日常使用

### 拉取最新代码（每次开始工作前）

**方法1：双击脚本**
- 双击项目根目录的 `拉取最新代码.bat`

**方法2：命令行**
```bash
cd 项目路径
git pull
git submodule update --init
cd Content
git pull
git lfs pull
```

---

### 提交修改

#### 情况1：只修改了代码（Source、Config 等）

可以用 **GitHub Desktop**：
1. 打开 GitHub Desktop
2. 选择修改的文件
3. 写提交说明
4. 点击 Commit，然后 Push

#### 情况2：修改了蓝图/资源（Content 文件夹）

**方法1：双击脚本**
- 双击项目根目录的 `提交Content修改.bat`
- 输入提交说明，回车

**方法2：命令行**
```bash
# 1. 进入 Content 目录
cd Content

# 2. 提交修改
git add .
git commit -m "你的提交说明"
git push

# 3. 回到主目录，更新引用
cd ..
git add Content
git commit -m "Update content"
git push
```

#### 情况3：同时修改了代码和蓝图

1. 先用命令行提交 Content（见上方）
2. 再用 GitHub Desktop 提交代码

---

## 常见问题

### Q：Content 文件夹是空的？
运行：
```bash
git submodule update --init
cd Content
git lfs pull
```

### Q：拉取时提示冲突？
如果是 Content 里的冲突：
```bash
cd Content
git stash        # 暂存你的修改
git pull
git stash pop    # 恢复你的修改
```

### Q：提交时提示需要梯子？
Hugging Face 在国外，推送 Content 修改时需要开梯子。

### Q：推送 Content 时提示需要用户名/密码？
首次推送需要配置 Hugging Face 认证：

1. 注册 Hugging Face 账号：https://huggingface.co/join
2. 告诉项目管理员你的用户名，等待被加入组织
3. 创建 Token：https://huggingface.co/settings/tokens
   - 点击 "Create new token"
   - 勾选 **Write** 权限
   - 复制生成的 token
4. 推送时：
   - Username：输入你的 HF 用户名
   - Password：输入你的 token（不是密码）

让 Git 记住密码（只需执行一次）：
```bash
git config --global credential.helper store
```

---

## 重要提醒

1. **每次开始工作前先拉取最新代码**
2. **修改 Content 需要用命令行或脚本提交**
3. **推送到 Hugging Face 需要开梯子**
4. **尽量避免多人同时修改同一个蓝图**
