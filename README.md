# 无限GUI DLL  
> A dynamic-link library for injecting HUD overlays into Java Edition Minecraft  
> 一个用于向 Java 版 Minecraft 注入信息显示层（HUD）的动态链接库

---

## ⚠ Disclaimer | 免责声明

**本程序不会读取、修改或写入任何 Minecraft 游戏内存数据。  
不会提供或造成任何不公平的游戏优势，也不涉及任何加速、透视、自动操作等功能。  
InfiniteGui 仅用于绘制独立的叠加界面（Overlay），与游戏逻辑无任何交互。**

另外需要特别说明：

**⚠ 请勿在网易版 Minecraft（网易MC）中使用本程序。**  
由于网易版存在独立的客户端保护机制，**DLL 注入行为可能会被错误识别为作弊行为，并导致封号风险。**  
如果用户在网易版 Minecraft 中注入本程序并导致账号封禁，作者概不负责。  

本项目使用了《**阿里巴巴普惠体**》，是一款由中国企业首次发布的可面向全场景使用的免费商用正文字体。

## ⚠️ 项目维护状态 | Maintenance Status

**该项目不再继续维护更新。新版 InfiniteGUI 已闭源，请访问 [无限Gui Client](https://www.Infinitegui.top) 获取详情。**

---

## 📌 Overview | 项目简介

**无限GUI** 是一个可以注入到 **大部分 Java 版 Minecraft** 的动态链接库（DLL）。  
注入成功后，你可以在游戏画面中显示各种实时信息，包括：

- 自定义文本  
- 时间  
- 粉丝数  
- 计数器  
- 游戏 FPS  
- B 站直播弹幕
- 按键显示
- CPS显示
  
适用于 **主播、视频作者、服务器玩家、工具开发者** 等希望在游戏中加入 HUD 信息显示的人群。

---

## ✨ Features | 功能特点

### ✔ 基础功能
- 游戏内文字绘制  
- 游戏帧率（FPS）显示  
- 自定义信息编辑  
- 时间显示  
- 计数器
- 动态模糊

### ✔ Drip 风格模块（本 Fork 新增）
- **ArrayList 模块列表**：在屏幕边缘显示所有已开启的模块，深色半透明条 + 彩色渐变强调边，
  按文字宽度/名称排序，开关模块时有滑动淡入淡出动画；位置、颜色、彩虹、渐变均可在设置中自定义  
- **TargetHUD 目标面板**：Drip 风格深色圆角面板，显示目标名称、平滑血量条（受击闪白）、HP 数字与连击数。
  数据来源三选一：
  - *自动*：JNI 实时读取优先，失败自动回退点击跟踪（默认）  
  - *JNI 实时数据*：通过注入的 JVM 读取准星指向目标的名字与血量，
    需要 **Forge 1.17+ / NeoForge** 等运行时使用 Mojang 官方映射的版本；其他环境自动不可用  
  - *点击跟踪*：纯叠加层实现，左键攻击时显示面板，目标名称与血量由设置提供（全版本可用，零风险）

### ✔ 直播相关功能
- B 站粉丝数显示  
- B 站直播间弹幕实时显示（需要配合第三方弹幕姬使用：https://www.danmuji.org )

### ✔ 技术特性
- 支持多版本 Java Minecraft-1.13~最新版本
- 自定义渲染  
- 独立 DLL，可用于二次开发

---

## 📦 Installation | 安装方式

### 1. 下载成品
你可以克隆代码并自行构建  

你也可以在 **爱发电** 以 8.8 元获得：  
- 编译好的成品
- 附赠注入器
- 自动更新  
- 技术支持  
- 使用教程  

（付费内容是“服务与编译成品”，不是源码）

---

## 🛠 Build | 构建教程（源码编译）
> 本 Fork 已将全部第三方依赖内置到仓库，**无需 vcpkg / 手动安装**：
> - GLEW 2.2.0（静态编译，`glew.c` + `GL/` 头文件，无需再放 `glew32.dll`）
> - stb_image.h
> - nlohmann/json（single header）
> - jni.h / jni_md.h（来自 OpenJDK，供 TargetHUD 的 JNI 数据源使用）

### ⚠️ 编码说明
本 Fork 已将全部源码统一为 **UTF-8 (BOM)** 编码，`dependencies.props` 已添加 `/utf-8`，
在系统区域设置开启 "Beta: UTF-8" (ACP=65001) 或传统 GBK (ACP=936) 的机器上均可直接编译。

### ✅ 编译步骤
1. 安装 Visual Studio 2022（需 “C++ 桌面开发” 组件）  
   平台工具集：MSVC v143，C++ 标准：C++17 或更高
2. 打开 `InfiniteGUI-DLL.sln`，选择 **Release | x64**，直接生成即可

### ⚠️ 注入教程
编译后生成：
- InfiniteGUI-DLL.dll

使用注入程序，如[CheatEngine](https://www.cheatengine.org/ "CheatEngine官方网站")，将**InfiniteGUI-DLL.dll**注入到Minecraft中即可使用。


