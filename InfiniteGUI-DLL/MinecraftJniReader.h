#pragma once

#include <string>
#include <mutex>
#include <chrono>

#include "jni/jni.h"

// ============================================================
// JniTargetSnapshot · 准星目标数据快照
// ============================================================
struct JniTargetSnapshot
{
	bool valid = false;
	int entityId = -1;
	std::string name;   // UTF-8
	float health = 0.0f;
	float maxHealth = 20.0f;
	std::chrono::steady_clock::time_point time;
};

// ============================================================
// MinecraftJniReader
// 通过 JNI 读取注入进程（JVM）中 Minecraft 准星指向的目标实体信息。
// 类查找策略：
//   1. 直接 FindClass（系统类加载器可见时）
//   2. 遍历所有线程的 ContextClassLoader 后 Class.forName
//      （兼容 Forge 1.17+ / NeoForge 的 TransformingClassLoader）
// 类名/方法名使用 Mojang 官方映射（Forge 1.17+ 运行时名称）。
// 任何一步失败都会优雅降级（返回 false），不会使游戏崩溃。
// ============================================================
class MinecraftJniReader
{
public:
	enum Status
	{
		Status_NotTried,
		Status_NoJvm,         // 不在 JVM 进程内
		Status_AttachFailed,  // JVM 线程附加失败
		Status_ClassNotFound, // 游戏类未找到（非官方映射运行时 / 混淆环境）
		Status_InitFailed,    // 类已找到但映射不匹配
		Status_Ready,         // 就绪
	};

	static MinecraftJniReader& Instance() {
		static MinecraftJniReader instance;
		return instance;
	}

	// 尝试读取准星目标，成功时填充 out 并返回 true
	bool GetTarget(JniTargetSnapshot& out);

	Status GetStatus() const { return status; }
	const char* GetStatusText() const;

private:
	MinecraftJniReader() = default;
	~MinecraftJniReader();

	MinecraftJniReader(const MinecraftJniReader&) = delete;
	MinecraftJniReader& operator=(const MinecraftJniReader&) = delete;

	bool EnsureJvm();
	bool EnsureClass();
	bool ReadTargetLocked(JniTargetSnapshot& out);
	// 用记录下来的游戏类加载器解析类（找不到时回退到 FindClass）
	jclass FindGameClass(const char* name);
	// 目标是否是玩家（优先 Player 类型判定，失败时按类名层级回退）
	bool IsPlayerEntity(jobject entity, jclass clsEntity);

	std::mutex mutex;
	Status status = Status_NotTried;
	std::chrono::steady_clock::time_point nextRetry{};
	int classAttempts = 0;   // 类解析失败次数；达上限后永久放弃，不再调用 JNI

	JavaVM* vm = nullptr;
	JNIEnv* env = nullptr;
	bool attached = false;

	jclass mcClass = nullptr;         // GlobalRef
	jclass playerClass = nullptr;     // GlobalRef  net/minecraft/world/entity/player/Player
	jclass hitTypeClass = nullptr;    // GlobalRef
	jobject gameClassLoader = nullptr;// GlobalRef  找到 Minecraft 的类加载器（Forge/NeoForge 分层加载器）
	jclass clsClassRef = nullptr;     // GlobalRef  java/lang/Class
	jmethodID mClassForName = nullptr;// Class.forName(String, boolean, ClassLoader)

	jmethodID mGetInstance = nullptr;
	jfieldID fHitResult = nullptr;
	jfieldID fPlayer = nullptr;       // Minecraft.player（本地玩家，用于排除自己）
	jmethodID mGetType = nullptr;
	jfieldID fEntityEnum = nullptr;
	jmethodID mGetEntity = nullptr;
	jmethodID mGetId = nullptr;
	jmethodID mGetHealth = nullptr;
	jmethodID mGetMaxHealth = nullptr;
	jmethodID mGetName = nullptr;
	jmethodID mGetString = nullptr;
	jmethodID mGetText = nullptr;
	jmethodID mClassName = nullptr;   // java/lang/Class.getName（玩家判定回退用）
};
