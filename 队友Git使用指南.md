# 队友 Git 使用指南

## 项目结构

本项目有两个仓库，用不同工具操作：

| 内容 | 工具 |
|------|------|
| 代码、配置、插件 | GitHub Desktop |
| Content 资源文件 | 双击 bat 脚本 |

---

## 每天开始工作前

**双击运行 `拉取最新代码.bat`**

等待完成即可。

---

## 提交你的修改

### 情况 A：修改了代码/配置/插件（非 Content 文件夹，你们大多数人用不上）

使用 **GitHub Desktop**：

1. 打开 GitHub Desktop
2. 点击 `Current Branch` → `New Branch`
3. 分支命名：`feature/你的名字-功能描述`（例如：`feature/ycj-combat`）
4. 写好 Commit 信息，点击 `Commit`
5. 点击 `Push origin`
6. 点击 `Create Pull Request`，在网页上提交 PR
7. 通知维护者 Review

### 情况 B：修改了 Content 文件夹（资源文件）

**双击运行 `提交Content修改.bat`**

按提示输入提交说明即可。

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

## ⚠️ 重要提醒

1. **每天开始工作前先拉取**
   - 双击 `拉取最新代码.bat`

2. **代码修改必须新建分支**
   - 不要在 main 分支上直接改
   - 用 GitHub Desktop 新建分支

3. **不要用 GitHub Desktop 操作 Content 文件夹**
   - 会导致配置错误
   - Content 只用 bat 脚本提交

4. **先问再改**
   - 改动较大的功能先在群里说一声
   - 避免冲突

---

## 遇到问题？

找维护者帮忙解决，不要自己瞎搞 😅
