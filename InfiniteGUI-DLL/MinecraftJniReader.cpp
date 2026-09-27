#include "MinecraftJniReader.h"

#include <Windows.h>
#include <cstdarg>
#include <cstring>
#include <algorithm>

#include "Log.hpp"

// ============================================================
// MinecraftJniReader 实现
//
// 核心策略：成员全部按【描述符】解析
//   Forge 1.17+ 运行时是「官方类名 + SRG 成员名」，例如
//     net.minecraft.client.Minecraft.m_91087_()   // getInstance()
//     ...f_91077_                                 // hitResult
//   直接写 getInstance/hitResult 找不到，所以用 JVMTI 枚举类成员、
//   按签名匹配，名字仅作为优先提示。
//
// 安全约定（违反任意一条都会让 JVM 原生崩溃）：
//   1. jmethodID / jfieldID 使用前必须判空
//   2. 可能抛异常的 JNI 调用后要 ExceptionCheck/ExceptionClear
//   3. 局部引用用 PushLocalFrame/PopLocalFrame 成对管理
//   4. 解析失败整体回退；失败次数有上限
// ============================================================

static const int kMaxThreadScan = 64;
static const int kMaxClassAttempts = 5;

static constexpr jint kAccStatic = 0x0008;

// ------------------------------------------------------------
MinecraftJniReader::~MinecraftJniReader()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (env)
	{
		if (mcClass) env->DeleteGlobalRef(mcClass);
		if (playerClass) env->DeleteGlobalRef(playerClass);
		if (entityHitResultClass) env->DeleteGlobalRef(entityHitResultClass);
		if (gameClassLoader) env->DeleteGlobalRef(gameClassLoader);
		if (clsClassRef) env->DeleteGlobalRef(clsClassRef);
	}
	if (vm && attached)
		vm->DetachCurrentThread();
}

const char* MinecraftJniReader::GetStatusText() const
{
	switch (status)
	{
	case Status_NotTried:      return u8"未尝试";
	case Status_NoJvm:         return u8"未检测到 JVM";
	case Status_AttachFailed:  return u8"JVM 附加失败";
	case Status_ClassNotFound: return u8"游戏类未找到";
	case Status_InitFailed:    return u8"成员解析失败";
	case Status_Ready:         return u8"已就绪";
	}
	return u8"未知";
}

// ------------------------------------------------------------
bool MinecraftJniReader::EnsureJvm()
{
	if (!vm)
	{
		HMODULE jvmModule = GetModuleHandleW(L"jvm.dll");
		if (!jvmModule)
		{
			status = Status_NoJvm;
			return false;
		}
		using GetCreatedJavaVMsFn = jint(WINAPI*)(JavaVM**, jsize, jsize*);
		auto getVMs = reinterpret_cast<GetCreatedJavaVMsFn>(
			GetProcAddress(jvmModule, "JNI_GetCreatedJavaVMs"));
		if (!getVMs)
		{
			status = Status_NoJvm;
			return false;
		}
		JavaVM* vms[4] = {};
		jsize count = 0;
		getVMs(vms, 4, &count);
		if (count <= 0 || !vms[0])
		{
			status = Status_NoJvm;
			return false;
		}
		vm = vms[0];
	}

	JNIEnv* localEnv = nullptr;
	jint rc = vm->GetEnv(reinterpret_cast<void**>(&localEnv), JNI_VERSION_1_8);
	if (rc == JNI_EDETACHED)
	{
		JavaVMAttachArgs args;
		args.version = JNI_VERSION_1_8;
		args.name = const_cast<char*>("InfiniteGUI-JNI");
		args.group = nullptr;
		if (vm->AttachCurrentThreadAsDaemon(reinterpret_cast<void**>(&localEnv), &args) != JNI_OK)
		{
			status = Status_AttachFailed;
			return false;
		}
		attached = true;
	}
	else if (rc != JNI_OK)
	{
		status = Status_AttachFailed;
		return false;
	}

	env = localEnv;

	// JVMTI 环境：枚举已加载类与类成员（GetLoadedClasses 不需要 capability）
	if (!jvmti)
	{
		jvmtiEnv* jt = nullptr;
		if (vm->GetEnv(reinterpret_cast<void**>(&jt), JVMTI_VERSION_1_2) == JNI_OK && jt)
		{
			jvmti = jt;
			InfGuiLog("JNI: JVMTI 已就绪");
		}
	}

	return true;
}

// ------------------------------------------------------------
jclass MinecraftJniReader::FindLoadedClassJvmti(const char* name)
{
	if (!jvmti || !env || !name) return nullptr;

	std::string want = std::string("L") + name + ";";

	jint count = 0;
	jclass* classes = nullptr;
	if (jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE || !classes)
		return nullptr;

	jclass result = nullptr;
	for (jint i = 0; i < count && !result; i++)
	{
		char* sig = nullptr;
		if (jvmti->GetClassSignature(classes[i], &sig, nullptr) == JVMTI_ERROR_NONE && sig)
		{
			if (want == sig)
				result = reinterpret_cast<jclass>(env->NewGlobalRef(classes[i]));
			jvmti->Deallocate(reinterpret_cast<unsigned char*>(sig));
		}
	}

	jvmti->Deallocate(reinterpret_cast<unsigned char*>(classes));
	return result;
}

// 按描述符找方法；nameHints 命中会优先返回
jmethodID MinecraftJniReader::FindMethodByDesc(jclass cls, const char* desc, bool wantStatic,
	const char* const* nameHints, int hintCount)
{
	if (!jvmti || !cls || !desc) return nullptr;

	jint count = 0;
	jmethodID* methods = nullptr;
	if (jvmti->GetClassMethods(cls, &count, &methods) != JVMTI_ERROR_NONE || !methods)
		return nullptr;

	jmethodID first = nullptr;
	jmethodID hinted = nullptr;

	for (jint i = 0; i < count; i++)
	{
		char* name = nullptr; char* sig = nullptr; char* gen = nullptr;
		if (jvmti->GetMethodName(methods[i], &name, &sig, &gen) == JVMTI_ERROR_NONE)
		{
			bool match = (sig != nullptr && std::strcmp(sig, desc) == 0);
			if (match && wantStatic)
			{
				jint mods = 0;
				if (jvmti->GetMethodModifiers(methods[i], &mods) == JVMTI_ERROR_NONE)
					match = (mods & kAccStatic) != 0;
				else
					match = false;
			}
			if (match)
			{
				if (!first) first = methods[i];
				if (!hinted && name && nameHints)
				{
					for (int h = 0; h < hintCount; h++)
					{
						if (nameHints[h] && std::strcmp(name, nameHints[h]) == 0)
						{
							hinted = methods[i];
							break;
						}
					}
				}
			}
			if (name) jvmti->Deallocate(reinterpret_cast<unsigned char*>(name));
			if (sig) jvmti->Deallocate(reinterpret_cast<unsigned char*>(sig));
			if (gen) jvmti->Deallocate(reinterpret_cast<unsigned char*>(gen));
		}
	}

	jvmti->Deallocate(reinterpret_cast<unsigned char*>(methods));
	return hinted ? hinted : first;
}

// 按描述符找字段；nameHints 命中会优先返回
jfieldID MinecraftJniReader::FindFieldByDesc(jclass cls, const char* desc,
	const char* const* nameHints, int hintCount)
{
	if (!jvmti || !cls || !desc) return nullptr;

	jint count = 0;
	jfieldID* fields = nullptr;
	if (jvmti->GetClassFields(cls, &count, &fields) != JVMTI_ERROR_NONE || !fields)
		return nullptr;

	jfieldID first = nullptr;
	jfieldID hinted = nullptr;

	for (jint i = 0; i < count; i++)
	{
		char* name = nullptr; char* sig = nullptr; char* gen = nullptr;
		// 注意：C++ 包装版 GetFieldName 需要先传所属类
		if (jvmti->GetFieldName(cls, fields[i], &name, &sig, &gen) == JVMTI_ERROR_NONE)
		{
			if (sig && std::strcmp(sig, desc) == 0)
			{
				if (!first) first = fields[i];
				if (!hinted && name && nameHints)
				{
					for (int h = 0; h < hintCount; h++)
					{
						if (nameHints[h] && std::strcmp(name, nameHints[h]) == 0)
						{
							hinted = fields[i];
							break;
						}
					}
				}
			}
			if (name) jvmti->Deallocate(reinterpret_cast<unsigned char*>(name));
			if (sig) jvmti->Deallocate(reinterpret_cast<unsigned char*>(sig));
			if (gen) jvmti->Deallocate(reinterpret_cast<unsigned char*>(gen));
		}
	}

	jvmti->Deallocate(reinterpret_cast<unsigned char*>(fields));
	return hinted ? hinted : first;
}

// ------------------------------------------------------------
jclass MinecraftJniReader::FindGameClass(const char* name)
{
	JNIEnv* e = env;
	if (!e || !name) return nullptr;

	// 1) 系统类加载器
	jclass direct = e->FindClass(name);
	if (direct)
	{
		jclass g = reinterpret_cast<jclass>(e->NewGlobalRef(direct));
		e->DeleteLocalRef(direct);
		return g;
	}
	if (e->ExceptionCheck()) e->ExceptionClear();

	// 2) JVMTI 已加载类
	{
		jclass viaJvmti = FindLoadedClassJvmti(name);
		if (viaJvmti) return viaJvmti;
	}

	// 3) 游戏类加载器
	if (gameClassLoader && mClassForName && clsClassRef)
	{
		jstring jname = e->NewStringUTF(name);
		if (jname)
		{
			jclass cls = reinterpret_cast<jclass>(
				e->CallStaticObjectMethod(clsClassRef, mClassForName, jname,
					static_cast<jboolean>(JNI_TRUE), gameClassLoader));
			if (e->ExceptionCheck()) { e->ExceptionClear(); cls = nullptr; }
			e->DeleteLocalRef(jname);
			if (cls) return reinterpret_cast<jclass>(e->NewGlobalRef(cls));
		}
	}
	return nullptr;
}

// ------------------------------------------------------------
bool MinecraftJniReader::EnsureClass()
{
	if (mcClass && mGetInstance && fHitResult && playerClass && entityHitResultClass)
	{
		status = Status_Ready;
		return true;
	}

	// 半初始化：整体回退
	{
		auto drop = [this](jclass& c) { if (c) { env->DeleteGlobalRef(c); c = nullptr; } };
		if (mcClass && (!mGetInstance || !fHitResult))
		{
			drop(mcClass); drop(playerClass); drop(entityHitResultClass);
			mGetInstance = nullptr; fHitResult = nullptr; fPlayer = nullptr;
			mGetEntity = nullptr; mGetHealth = nullptr; mGetMaxHealth = nullptr;
			mGetName = nullptr; mGetString = nullptr;
		}
	}

	JNIEnv* e = env;
	if (e->PushLocalFrame(64) != JNI_OK)
	{
		status = Status_InitFailed;
		return false;
	}

	// ---------------- 1) 找 Minecraft 类 ----------------
	jclass mc = FindLoadedClassJvmti("net/minecraft/client/Minecraft");
	if (!mc)
	{
		jclass direct = e->FindClass("net/minecraft/client/Minecraft");
		if (direct)
		{
			mc = reinterpret_cast<jclass>(e->NewGlobalRef(direct));
			e->DeleteLocalRef(direct);
		}
		else if (e->ExceptionCheck()) e->ExceptionClear();
	}
	if (!mc)
	{
		// 兜底：遍历线程 ContextClassLoader
		jclass clsThread = e->FindClass("java/lang/Thread");
		if (clsThread)
		{
			jmethodID mAll = e->GetStaticMethodID(clsThread, "getAllStackTraces", "()Ljava/util/Map;");
			jmethodID mCtx = e->GetMethodID(clsThread, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
			jclass clsClass = e->FindClass("java/lang/Class");
			jmethodID mForName = clsClass ? e->GetStaticMethodID(clsClass, "forName",
				"(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;") : nullptr;
			if (e->ExceptionCheck()) e->ExceptionClear();

			if (mAll && mCtx && mForName)
			{
				if (!clsClassRef) clsClassRef = reinterpret_cast<jclass>(e->NewGlobalRef(clsClass));
				if (!mClassForName) mClassForName = mForName;

				jobject map = e->CallStaticObjectMethod(clsThread, mAll);
				if (e->ExceptionCheck()) { e->ExceptionClear(); map = nullptr; }
				if (map)
				{
					jclass clsMap = e->GetObjectClass(map);
					jmethodID mValues = e->GetMethodID(clsMap, "values", "()Ljava/util/Collection;");
					jobject col = mValues ? e->CallObjectMethod(map, mValues) : nullptr;
					if (e->ExceptionCheck()) { e->ExceptionClear(); col = nullptr; }
					jobjectArray arr = nullptr;
					if (col)
					{
						jclass clsCol = e->GetObjectClass(col);
						jmethodID mToArray = e->GetMethodID(clsCol, "toArray", "()[Ljava/lang/Object;");
						arr = mToArray ? reinterpret_cast<jobjectArray>(e->CallObjectMethod(col, mToArray)) : nullptr;
						if (e->ExceptionCheck()) { e->ExceptionClear(); arr = nullptr; }
					}
					if (arr)
					{
						jsize n = e->GetArrayLength(arr);
						if (n > kMaxThreadScan) n = kMaxThreadScan;
						for (jsize i = 0; i < n && !mc; i++)
						{
							if (e->PushLocalFrame(16) != JNI_OK) break;
							jobject th = e->GetObjectArrayElement(arr, i);
							if (th && e->IsInstanceOf(th, clsThread))
							{
								jobject loader = e->CallObjectMethod(th, mCtx);
								if (e->ExceptionCheck()) { e->ExceptionClear(); loader = nullptr; }
								if (loader)
								{
									jstring cn = e->NewStringUTF("net/minecraft/client/Minecraft");
									jclass c = cn ? reinterpret_cast<jclass>(e->CallStaticObjectMethod(
										clsClass, mForName, cn, static_cast<jboolean>(JNI_TRUE), loader)) : nullptr;
									if (e->ExceptionCheck()) { e->ExceptionClear(); c = nullptr; }
									if (c)
									{
										mc = reinterpret_cast<jclass>(e->NewGlobalRef(c));
										if (!gameClassLoader) gameClassLoader = e->NewGlobalRef(loader);
									}
								}
							}
							e->PopLocalFrame(nullptr);
						}
					}
				}
			}
			if (e->ExceptionCheck()) e->ExceptionClear();
		}
	}

	if (!mc)
	{
		e->PopLocalFrame(nullptr);
		status = Status_ClassNotFound;
		InfGuiLog("JNI: 未找到 net.minecraft.client.Minecraft");
		return false;
	}

	// ---------------- 2) 按描述符解析成员 ----------------
	static const char* hintsGetInstance[] = { "getInstance", "m_91087_" };
	static const char* hintsHitResult[] = { "hitResult", "f_91077_" };
	static const char* hintsPlayer[] = { "player", "f_91074_" };

	mGetInstance = FindMethodByDesc(mc, "()Lnet/minecraft/client/Minecraft;", true,
		hintsGetInstance, 2);

	fHitResult = FindFieldByDesc(mc, "Lnet/minecraft/world/phys/HitResult;", hintsHitResult, 2);

	fPlayer = FindFieldByDesc(mc, "Lnet/minecraft/client/player/LocalPlayer;", hintsPlayer, 2);
	if (!fPlayer)
		fPlayer = FindFieldByDesc(mc, "Lnet/minecraft/client/player/AbstractClientPlayer;", hintsPlayer, 2);
	if (!fPlayer)
		fPlayer = FindFieldByDesc(mc, "Lnet/minecraft/world/entity/player/Player;", hintsPlayer, 2);

	if (!mGetInstance || !fHitResult)
	{
		e->PopLocalFrame(nullptr);
		env->DeleteGlobalRef(mc);
		mcClass = nullptr;
		mGetInstance = nullptr;
		fHitResult = nullptr;
		fPlayer = nullptr;
		status = Status_InitFailed;
		InfGuiLog("JNI: 成员解析失败 (getInstance=%d hitResult=%d player=%d)",
			mGetInstance != nullptr, fHitResult != nullptr, fPlayer != nullptr);
		return false;
	}

	mcClass = mc;

	// 相关类（都用官方类名，Forge 运行时类名不混淆）
	if (!playerClass)
		playerClass = FindLoadedClassJvmti("net/minecraft/world/entity/player/Player");
	if (!entityHitResultClass)
		entityHitResultClass = FindLoadedClassJvmti("net/minecraft/world/phys/EntityHitResult");

	e->PopLocalFrame(nullptr);

	status = Status_Ready;
	InfGuiLog("JNI: 就绪 (Player类=%d, EntityHitResult=%d, player字段=%d)",
		playerClass != nullptr, entityHitResultClass != nullptr, fPlayer != nullptr);
	return true;
}

// ------------------------------------------------------------
bool MinecraftJniReader::IsPlayerEntity(jobject entity, jclass clsEntity)
{
	JNIEnv* e = env;
	if (!e || !entity || !clsEntity) return false;

	if (playerClass)
		return e->IsInstanceOf(entity, playerClass) == JNI_TRUE;

	// 回退：类名层级判定
	if (!mClassName)
	{
		jclass clsClass = e->FindClass("java/lang/Class");
		if (clsClass)
		{
			mClassName = e->GetMethodID(clsClass, "getName", "()Ljava/lang/String;");
			if (e->ExceptionCheck()) { e->ExceptionClear(); mClassName = nullptr; }
			e->DeleteLocalRef(clsClass);
		}
	}
	if (!mClassName) return false;

	jclass cur = clsEntity;
	for (int depth = 0; depth < 8 && cur; depth++)
	{
		jstring js = reinterpret_cast<jstring>(e->CallObjectMethod(cur, mClassName));
		if (e->ExceptionCheck()) { e->ExceptionClear(); break; }
		if (js)
		{
			const char* c = e->GetStringUTFChars(js, nullptr);
			std::string n = c ? c : "";
			if (c) e->ReleaseStringUTFChars(js, c);
			if (n.find("LocalPlayer") != std::string::npos) return false;
			if (n == "net.minecraft.world.entity.player.Player") return true;
			if (n.find(".player.") != std::string::npos) return true;
		}
		jclass super = e->GetSuperclass(cur);
		if (e->ExceptionCheck()) { e->ExceptionClear(); break; }
		if (!super) break;
		cur = super;
	}
	return false;
}

// ------------------------------------------------------------
bool MinecraftJniReader::ReadTargetLocked(JniTargetSnapshot& out)
{
	auto now = std::chrono::steady_clock::now();
	JNIEnv* e = env;

	if (!mcClass || !mGetInstance || !fHitResult)
		return false;

	jobject instance = e->CallStaticObjectMethod(mcClass, mGetInstance);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!instance) return false;   // 不在世界中

	jobject hit = e->GetObjectField(instance, fHitResult);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!hit) return false;

	// 只处理实体命中（用类型判定替代 HitResult.Type 枚举，避免枚举名混淆问题）
	if (entityHitResultClass && e->IsInstanceOf(hit, entityHitResultClass) != JNI_TRUE)
		return false;

	if (!mGetEntity)
	{
		jclass clsHit = e->GetObjectClass(hit);
		if (clsHit)
		{
			mGetEntity = FindMethodByDesc(clsHit, "()Lnet/minecraft/world/entity/Entity;", false, nullptr, 0);
			e->DeleteLocalRef(clsHit);
		}
	}
	if (!mGetEntity) return false;

	jobject entity = e->CallObjectMethod(hit, mGetEntity);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!entity) return false;

	jclass clsEntity = e->GetObjectClass(entity);
	if (!clsEntity) return false;

	// 仅玩家
	if (!IsPlayerEntity(entity, clsEntity))
	{
		e->DeleteLocalRef(clsEntity);
		return false;
	}

	// 排除自己
	if (fPlayer)
	{
		jobject local = e->GetObjectField(instance, fPlayer);
		if (e->ExceptionCheck()) { e->ExceptionClear(); local = nullptr; }
		if (local && e->IsSameObject(local, entity))
		{
			e->DeleteLocalRef(clsEntity);
			return false;
		}
	}

	// ---- 血量：按描述符取 ()F，官方/SRG 名字仅作提示 ----
	static const char* hintsHealth[] = { "getHealth", "m_21223_" };
	static const char* hintsMaxHealth[] = { "getMaxHealth", "m_21233_" };

	if (!mGetHealth)
		mGetHealth = FindMethodByDesc(clsEntity, "()F", false, hintsHealth, 2);
	if (!mGetMaxHealth)
		mGetMaxHealth = FindMethodByDesc(clsEntity, "()F", false, hintsMaxHealth, 2);

	if (!mGetHealth)
	{
		e->DeleteLocalRef(clsEntity);
		return false;   // 非生物实体
	}

	float health = e->CallFloatMethod(entity, mGetHealth);
	if (e->ExceptionCheck()) { e->ExceptionClear(); e->DeleteLocalRef(clsEntity); return false; }

	float maxHealth = health;
	if (mGetMaxHealth && mGetMaxHealth != mGetHealth)
	{
		maxHealth = e->CallFloatMethod(entity, mGetMaxHealth);
		if (e->ExceptionCheck()) { e->ExceptionClear(); maxHealth = health; }
	}
	// 名字提示可能反了：按大小纠正（血量不可能大于上限）
	if (maxHealth < health)
		std::swap(health, maxHealth);
	if (maxHealth <= 0.0f) maxHealth = 20.0f;

	// ---- 名字：Entity.getName() -> Component.getString() ----
	std::string name;
	if (!mGetName)
		mGetName = FindMethodByDesc(clsEntity, "()Lnet/minecraft/network/chat/Component;", false, nullptr, 0);
	if (mGetName)
	{
		jobject comp = e->CallObjectMethod(entity, mGetName);
		if (e->ExceptionCheck()) { e->ExceptionClear(); comp = nullptr; }
		if (comp)
		{
			jclass clsComp = e->GetObjectClass(comp);
			if (clsComp)
			{
				if (!mGetString)
				{
					static const char* hintsGetString[] = { "getString", "m_7532_" };
					mGetString = FindMethodByDesc(clsComp, "()Ljava/lang/String;", false, hintsGetString, 2);
				}
				if (mGetString)
				{
					jstring js = reinterpret_cast<jstring>(e->CallObjectMethod(comp, mGetString));
					if (e->ExceptionCheck()) { e->ExceptionClear(); js = nullptr; }
					if (js)
					{
						const char* c = e->GetStringUTFChars(js, nullptr);
						if (c)
						{
							name = c;
							e->ReleaseStringUTFChars(js, c);
						}
					}
				}
				e->DeleteLocalRef(clsComp);
			}
		}
	}

	e->DeleteLocalRef(clsEntity);

	out.valid = true;
	out.name = name;
	out.health = health;
	out.maxHealth = maxHealth;
	out.time = now;
	return true;
}

// ------------------------------------------------------------
bool MinecraftJniReader::GetTarget(JniTargetSnapshot& out)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (status != Status_Ready)
	{
		if (classAttempts >= kMaxClassAttempts)
			return false;   // 永久放弃，不再做 JNI 调用

		auto now = std::chrono::steady_clock::now();
		if (now < nextRetry)
			return false;
		nextRetry = now + std::chrono::milliseconds(3000);

		if (!EnsureJvm()) return false;
		if (!EnsureClass())
		{
			classAttempts++;
			return false;
		}
		classAttempts = 0;
	}

	if (!env) return false;
	if (env->PushLocalFrame(32) != JNI_OK)
		return false;

	bool ok = false;
	JniTargetSnapshot snapshot;
	if (ReadTargetLocked(snapshot))
	{
		out = snapshot;
		ok = true;
	}

	env->PopLocalFrame(nullptr);
	return ok;
}
