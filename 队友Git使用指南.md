# 队友 Git 使用指南

## 项目结构

本项目使用**两个仓库**，分别管理不同内容：

| 内容 | 托管平台 | 操作方式 |
|------|----------|----------|
| 代码、配置、插件 | GitHub | GitHub Desktop |
| Content 资源文件（.uasset 等） | Hugging Face | 双击 bat 脚本 |

> **为什么分两个仓库？** UE 的 Content 文件夹包含大量二进制资源（几十 GB），GitHub 不适合存这些，所以用 Hugging Face + Git LFS 单独托管。

---

## 日常工作流程（必读！）

### 1. 开始工作前：拉取最新代码

**双击运行 `拉取最新代码.bat`**，等待完成。

它会自动完成：
- 拉取主仓库（代码/配置/插件）的最新修改
- 拉取 Content 子模块的最新资源
- 拉取 Git LFS 大文件

> ⚠️ **每次打开 UE 编辑器之前都要先拉取！** 不拉取就工作 = 基于旧版本改东西 = 后面一定会出问题。

### 2. 提交你的修改

#### 情况 A：修改了代码/配置/插件（非 Content 文件夹）

使用 **GitHub Desktop**：

1. 打开 GitHub Desktop
2. 点击 `Current Branch` → `New Branch`
3. 分支命名：`feature/你的名字-功能描述`（例如：`feature/ycj-combat`）
4. 写好 Commit 信息，点击 `Commit`
5. 点击 `Push origin`
6. 点击 `Create Pull Request`，在网页上提交 PR
7. 通知维护者 Review

#### 情况 B：修改了 Content 文件夹（资源文件）

**双击运行 `提交Content修改.bat`**，按提示输入提交说明即可。

脚本会自动完成以下流程：
1. **安全检查** — 先连接远程仓库，检查网络是否正常
2. **自动拉取** — 如果本地落后于远程，自动拉取最新代码再提交（防止冲突）
3. **提交 Content** — 将修改推送到 Hugging Face
4. **更新主仓库引用** — 同步子模块指针到 GitHub

> 如果脚本报错中止，**不要慌**，按照提示信息处理或找维护者。

### 3. 完整的一天工作流

```
开始工作 → 双击「拉取最新代码.bat」→ 打开 UE 编辑器 → 工作 → 
保存 → 关闭 UE → 双击「提交Content修改.bat」→ 输入说明 → 完成
```

---

## 首次使用：配置 Hugging Face Token

Content 资源托管在 Hugging Face，首次推送需要配置 Token。

### 步骤

1. 打开 https://huggingface.co/settings/tokens
2. 点击 **New token**
3. 名称随便填，**Type 选择 Write**
4. 点击 **Generate**
5. **复制 Token**（只显示一次！）

### 使用

运行 `提交Content修改.bat` 时：
- 用户名：你的 HF 用户名
- 密码：粘贴刚才的 Token

**提示**：输入一次后会自动保存，以后不用再输。

---

## ⚠️ 重要规则

### 必须做的

1. **每次工作前先拉取** — 双击 `拉取最新代码.bat`
2. **只用 bat 脚本操作 Content** — 不要用 GitHub Desktop 或手动 git 命令操作 Content 文件夹
3. **代码修改必须新建分支** — 不要在 main 分支上直接改代码
4. **提交前关闭 UE 编辑器** — UE 运行时会锁定文件，可能导致提交不完整
5. **改动较大先在群里说一声** — 避免多人同时改同一个资源文件

### 绝对不要做的

1. ❌ **不拉取就开始工作** — 这是仓库混乱的头号原因
2. ❌ **不要用 GitHub Desktop 操作 Content** — 会导致子模块配置错误
3. ❌ **不要手动在 Content 目录下执行 git 命令** — 除非维护者指导你这么做
4. ❌ **不要强制推送（`git push --force`）** — 会覆盖别人的提交
5. ❌ **不要手动删除 `.git` 相关文件夹** — 会破坏仓库结构

---

## 常见问题排查

### Q1：`提交Content修改.bat` 报"无法连接到 HuggingFace 远程仓库"

**原因**：网络问题，Hugging Face 在国内访问不稳定。

**解决**：
- 检查代理/VPN 是否开启
- 如果开了代理，确认 Git 代理配置正确：
  ```bash
  git config --global http.proxy    # 查看当前代理设置
  ```
- 换个网络节点重试

### Q2：拉取时报"untracked working tree files would be overwritten"

**原因**：本地有 UE 自动生成的文件（截图、缓存等），和远程的同名文件冲突。

**解决**：
```bash
cd Content
git clean -fdn    # 先预览会删什么（不会真删）
git clean -fd     # 确认后删除这些未跟踪文件
git pull           # 重新拉取
```

### Q3：拉取时弹出 Vim 编辑器（黑底白字，一堆 `~`）

**原因**：Git 让你输入合并提交信息。

**解决**：直接输入 `:wq` 然后按回车，即可保存退出。

### Q4：`git stash` 显示 "No local changes to save" 但还是拉不下来

**原因**：`git stash` 只保存已跟踪文件的修改。报错的是未跟踪文件（untracked），stash 管不了。

**解决**：用 `git clean -fd` 清理未跟踪文件（参考 Q2）。

### Q5：报 "rebase-merge directory already exists"

**原因**：之前的 rebase 操作没有完成就被中断了。

**解决**：
```bash
cd Content
git rebase --abort
git pull
```

### Q6：提交后队友拉不到我的修改

**检查清单**：
1. 确认脚本运行时没有报错
2. 确认你的修改确实在 Content 文件夹内
3. 让队友运行 `拉取最新代码.bat`

如果以上都正常还是拉不到，找维护者检查远程仓库状态。

---

## 遇到其他问题？

**截图报错信息，发到群里找维护者**。不要自己瞎搞，尤其不要执行不确定的 git 命令 😅
