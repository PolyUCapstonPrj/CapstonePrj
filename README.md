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

## 分支工作流（重要！）

**请勿直接推送到 main 分支！** 所有功能开发都应该在自己的分支上进行。

### 使用 GitHub Desktop（推荐）

#### 1. 创建新分支（开始新功能前）
- 点击顶部 `Current Branch` → `New Branch`
- 命名规则：`feature/你的名字-功能名`
- 例如：`feature/ycj-boss-skill`、`feature/law-ui-design`

#### 2. 开发过程中
- 正常修改代码
- 在 GitHub Desktop 左侧看到改动
- 写 commit 信息，点击 `Commit to feature/xxx`
- 可以多次 commit

#### 3. 推送分支
- 点击 `Push origin`（新分支首次推送会显示 `Publish branch`）

#### 4. 创建 Pull Request（完成开发后）
- 推送后 GitHub Desktop 会显示 `Create Pull Request` 按钮
- 点击后跳转到 GitHub 网页
- 填写 PR 标题和描述
- 点击 `Create Pull Request`
- **等待项目管理员 review 后合并**

#### 5. 合并后更新本地
- 切换回 main 分支：`Current Branch` → `main`
- 点击 `Fetch origin` 然后 `Pull origin`
- 可以删除已合并的本地分支

### 使用命令行

```bash
# 1. 确保在最新的 main 分支上
git checkout main
git pull origin main

# 2. 创建并切换到新分支
git checkout -b feature/你的名字-功能名

# 3. 开发完成后提交
git add .
git commit -m "完成xxx功能"

# 4. 推送到远程
git push origin feature/你的名字-功能名

# 5. 去 GitHub 网页创建 Pull Request

# 6. PR 合并后，切换回 main 并更新
git checkout main
git pull origin main
git branch -d feature/你的名字-功能名  # 删除本地分支
```

---

## Content 子模块提交（蓝图/资源）

Content 存放在 Hugging Face，**直接推送到 main 分支**（HF 没有 PR 功能）。

**方法1：双击脚本**
- 双击项目根目录的 `提交Content修改.bat`
- 输入提交说明，回车

**方法2：命令行**
```bash
# 1. 进入 Content 目录
cd Content

# 2. 确保在 main 分支
git checkout main

# 3. 提交修改
git add .
git commit -m "你的提交说明"
git push origin main

# 4. 回到主目录，更新引用（在你的功能分支上）
cd ..
git add Content
git commit -m "Update content: xxx"
git push origin feature/你的分支名
```

**注意**：
- 修改 Content 后，也需要在主仓库提交 Content 的引用更新
- **提交 Content 前请先在群里沟通，避免多人同时修改同一个蓝图**

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

**方法3：GitHub Desktop**
- `Fetch origin` → `Pull origin`
- 然后运行 `拉取最新代码.bat` 更新 Content

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

### Q：我在错误的分支上提交了怎么办？
```bash
# 1. 查看最近的 commit
git log --oneline -3

# 2. 撤销最近一次 commit（保留修改）
git reset --soft HEAD~1

# 3. 切换到正确的分支
git checkout feature/正确的分支名

# 4. 重新提交
git add .
git commit -m "你的提交说明"
```

---

## 重要提醒

1. **主仓库：请勿直接推送到 main 分支，使用 PR 流程**
2. **Content：直接推送到 main，但提交前先在群里沟通**
3. **每次开始工作前先拉取最新代码**
4. **推送到 Hugging Face 需要开梯子**
5. **尽量避免多人同时修改同一个蓝图**
6. **主仓库分支命名：`feature/你的名字-功能名`**
