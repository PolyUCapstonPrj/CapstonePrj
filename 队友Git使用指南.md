# 队友 Git 使用指南

## 你只需要记住两件事

### 1. 开始工作前

**双击 `拉取最新代码.bat`**

### 2. 完成工作后

**双击 `提交Content修改.bat`**，输入提交说明

就这么简单！

---

## 首次使用：配置 Hugging Face Token

Content 资源托管在 Hugging Face，首次推送需要配置 Token。

### 创建 Token

1. 打开 https://huggingface.co/settings/tokens
2. 点击 **New token**
3. 名称随便填，**Type 选择 Write**
4. 点击 **Generate**
5. **复制 Token**（只显示一次！）

### 使用 Token

运行 `提交Content修改.bat` 时会弹出登录框：
- 用户名：你的 HF 用户名
- 密码：粘贴 Token

输入一次后会自动保存，以后不用再输。

---

## ⚠️ 注意事项

1. **每天开始前先拉取** —— 避免冲突

2. **不要用 GitHub Desktop** —— 会搞乱配置

3. **遇到报错别慌** —— 截图发群里，找维护者解决
