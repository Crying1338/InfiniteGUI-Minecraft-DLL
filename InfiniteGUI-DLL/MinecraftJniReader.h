#pragma once

#include <string>
#include <mutex>
#include <chrono>

#include "jni/jni.h"
#include "jni/jvmti.h"

// ============================================================
// JniTargetSnapshot · 准星目标数据快照（仅玩家）
// ============================================================
struct JniTargetSnapshot
{
	bool valid = false;
	std::string name;   // UTF-8
	float health = 0.0f;
	float maxHealth = 20.0f;
	std::chrono::steady_clock::time_point time;
};

// ============================================================
// MinecraftJniReader · 读取准星指向的玩家
//
// 关键：Forge 1.17+ 运行时是「官方类名 + SRG 成员名」
//   net.minecraft.client.Minecraft.m_91087_()   ← 方法名是混淆的
// 因此成员一律按【描述符】解析（JVMTI 枚举类成员 + 签名匹配），
// 名字只作为优先提示，避免依赖任何映射表。
//
// 安全约定（违反任意一条都会让 JVM 原生崩溃）：
//   1. jmethodID / jfieldID 使用前必须判空
//   2. 可能抛异常的 JNI 调用后要 ExceptionCheck / ExceptionClear
//   3. 局部引用用 PushLocalFrame / PopLocalFrame 成对管理
//   4. 解析失败整体回退，不留半初始化状态
//   5. 失败重试有上限，超过后永久放弃
// ============================================================
class MinecraftJniReader
{
public:
	enum Status
	{
		Status_NotTried,
		Status_NoJvm,
		Status_AttachFailed,
		Status_ClassNotFound,
		Status_InitFailed,
		Status_Ready,
	};

	static MinecraftJniReader& Instance() {
		static MinecraftJniReader instance;
		return instance;
	}

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

	jclass FindGameClass(const char* name);
	jclass FindLoadedClassJvmti(const char* name);

	// 按描述符查找成员（名字提示命中优先）
	jmethodID FindMethodByDesc(jclass cls, const char* desc, bool wantStatic,
		const char* const* nameHints, int hintCount);
	jfieldID FindFieldByDesc(jclass cls, const char* desc,
		const char* const* nameHints, int hintCount);

	bool IsPlayerEntity(jobject entity, jclass clsEntity);

	std::mutex mutex;
	Status status = Status_NotTried;
	std::chrono::steady_clock::time_point nextRetry{};
	int classAttempts = 0;

	JavaVM* vm = nullptr;
	JNIEnv* env = nullptr;
	jvmtiEnv* jvmti = nullptr;
	bool attached = false;

	jclass mcClass = nullptr;
	jclass playerClass = nullptr;
	jclass entityHitResultClass = nullptr;
	jobject gameClassLoader = nullptr;
	jclass clsClassRef = nullptr;
	jmethodID mClassForName = nullptr;

	jmethodID mGetInstance = nullptr;
	jfieldID fHitResult = nullptr;
	jfieldID fPlayer = nullptr;
	jmethodID mGetEntity = nullptr;
	jmethodID mGetHealth = nullptr;
	jmethodID mGetMaxHealth = nullptr;
	jmethodID mGetName = nullptr;
	jmethodID mGetString = nullptr;
	jmethodID mClassName = nullptr;

	bool dumpedHealthCandidates = false;
};
