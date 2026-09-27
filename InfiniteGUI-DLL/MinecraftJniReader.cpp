#include "MinecraftJniReader.h"

#include <Windows.h>
#include <cstdarg>

#include "Log.hpp"

// ============================================================
// MinecraftJniReader 实现
//
// 安全约定（每一处都必须遵守，否则会导致 JVM 原生崩溃）:
//   1. 任何 jmethodID / jfieldID 在使用前必须判空；
//      把无效 ID 传给 JNI 会让 JVM 直接访问违例（EXCEPTION_ACCESS_VIOLATION）
//   2. 每次可能抛异常的 JNI 调用后都要 ExceptionCheck/ExceptionClear，
//      带未处理异常继续调用 JNI 属于未定义行为
//   3. 局部引用必须用 PushLocalFrame/PopLocalFrame 成对管理，
//      循环内每轮独立成帧，避免局部引用表膨胀
//   4. 解析失败要整体回退（全局引用与字段 ID 一起清掉），不能留下半初始化状态
//   5. 失败重试有上限，超过后永久放弃，不再做任何 JNI 调用
// ============================================================

static const int kMaxThreadScan = 64;   // 最多扫描的线程数
static const int kMaxClassAttempts = 3; // 类解析最大尝试次数

MinecraftJniReader::~MinecraftJniReader()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (env)
	{
		if (mcClass) env->DeleteGlobalRef(mcClass);
		if (playerClass) env->DeleteGlobalRef(playerClass);
		if (hitTypeClass) env->DeleteGlobalRef(hitTypeClass);
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
	case Status_ClassNotFound: return u8"游戏类未找到（需 Forge 1.17+/NeoForge）";
	case Status_InitFailed:    return u8"映射不匹配";
	case Status_Ready:         return u8"已就绪";
	}
	return u8"未知";
}

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

	// ---- 获取 JVMTI 环境：用 GetLoadedClasses 直接枚举已加载的类，
	//      这是 Forge/ModLauncher 分层类加载器下最可靠的查找方式 ----
	//      注：GetLoadedClasses 不需要申请任何 capability
	if (!jvmti)
	{
		jvmtiEnv* jt = nullptr;
		if (vm->GetEnv(reinterpret_cast<void**>(&jt), JVMTI_VERSION_1_2) == JNI_OK && jt)
		{
			jvmti = jt;
			InfGuiLog("JNI: JVMTI 已就绪（可按已加载类查找）");
		}
	}

	return true;
}

// 在 JVM 已加载的类里按名字查找（签名形如 Lnet/minecraft/client/Minecraft;）
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

// 解析游戏类；成功时 mcClass 为全局引用，mGetInstance / fHitResult / playerClass 均有效
bool MinecraftJniReader::EnsureClass()
{
	if (mcClass && mGetInstance && fHitResult)
	{
		status = Status_Ready;
		return true;
	}

	// 半初始化状态：整体回退，避免用到无效 ID
	if (mcClass && (!mGetInstance || !fHitResult))
	{
		env->DeleteGlobalRef(mcClass);
		mcClass = nullptr;
		mGetInstance = nullptr;
		fHitResult = nullptr;
		fPlayer = nullptr;
		if (playerClass) { env->DeleteGlobalRef(playerClass); playerClass = nullptr; }
	}

	JNIEnv* e = env;
	if (e->PushLocalFrame(128) != JNI_OK)
	{
		status = Status_InitFailed;
		return false;
	}

	jclass foundGlobal = nullptr;   // 成功后是全局引用

	// ---------------- 1) 直接用系统类加载器查找 ----------------
	{
		jclass direct = e->FindClass("net/minecraft/client/Minecraft");
		if (!direct)
		{
			if (e->ExceptionCheck()) e->ExceptionClear();
		}
		else
		{
			foundGlobal = reinterpret_cast<jclass>(e->NewGlobalRef(direct));
		}
	}

	// ---------------- 2) JVMTI：在 JVM 已加载的类里查找（Forge/ModLauncher 下最可靠）----------------
	if (!foundGlobal)
	{
		foundGlobal = FindLoadedClassJvmti("net/minecraft/client/Minecraft");
		if (foundGlobal)
			InfGuiLog("JNI: 通过 JVMTI 已加载类找到 Minecraft");
	}

	// ---------------- 3) 遍历线程 ContextClassLoader（兜底）----------------
	if (!foundGlobal)
	{
		jclass clsThread = e->FindClass("java/lang/Thread");
		if (!clsThread && e->ExceptionCheck()) e->ExceptionClear();

		if (clsThread)
		{
			jmethodID mAllStackTraces = e->GetStaticMethodID(clsThread, "getAllStackTraces", "()Ljava/util/Map;");
			jmethodID mCtxLoader = e->GetMethodID(clsThread, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
			if (e->ExceptionCheck()) e->ExceptionClear();

			// 关键：两个 ID 都必须有效，否则绝不调用（历史上这里少判空导致 JVM 崩溃）
			if (mAllStackTraces && mCtxLoader)
			{
				jobject map = e->CallStaticObjectMethod(clsThread, mAllStackTraces);
				if (e->ExceptionCheck()) { e->ExceptionClear(); map = nullptr; }

				jobject collection = nullptr;
				jobjectArray arr = nullptr;
				jclass clsClass = nullptr;
				jmethodID mForName = nullptr;

				if (map)
				{
					jclass clsMap = e->GetObjectClass(map);
					jmethodID mValues = clsMap ? e->GetMethodID(clsMap, "values", "()Ljava/util/Collection;") : nullptr;
					if (e->ExceptionCheck()) { e->ExceptionClear(); mValues = nullptr; }
					if (mValues) collection = e->CallObjectMethod(map, mValues);
					if (e->ExceptionCheck()) { e->ExceptionClear(); collection = nullptr; }
				}

				if (collection)
				{
					jclass clsCol = e->GetObjectClass(collection);
					jmethodID mToArray = clsCol ? e->GetMethodID(clsCol, "toArray", "()[Ljava/lang/Object;") : nullptr;
					if (e->ExceptionCheck()) { e->ExceptionClear(); mToArray = nullptr; }
					if (mToArray)
						arr = reinterpret_cast<jobjectArray>(e->CallObjectMethod(collection, mToArray));
					if (e->ExceptionCheck()) { e->ExceptionClear(); arr = nullptr; }
				}

				if (arr)
				{
					clsClass = e->FindClass("java/lang/Class");
					if (!clsClass && e->ExceptionCheck()) e->ExceptionClear();
					if (clsClass)
						mForName = e->GetStaticMethodID(clsClass, "forName",
							"(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
					if (e->ExceptionCheck()) { e->ExceptionClear(); mForName = nullptr; }
				}

				if (arr && clsClass && mForName)
				{
					// 记录 Class.forName 供后续解析其它游戏类（玩家类）复用
					if (!clsClassRef)
						clsClassRef = reinterpret_cast<jclass>(e->NewGlobalRef(clsClass));
					if (!mClassForName)
						mClassForName = mForName;

					jsize count = e->GetArrayLength(arr);
					if (count > kMaxThreadScan) count = kMaxThreadScan;

					for (jsize i = 0; i < count && !foundGlobal; i++)
					{
						// 每轮独立局部帧：局部引用不会堆积
						if (e->PushLocalFrame(16) != JNI_OK)
							break;

						jobject thread = e->GetObjectArrayElement(arr, i);
						if (thread && e->IsInstanceOf(thread, clsThread))
						{
							jobject loader = e->CallObjectMethod(thread, mCtxLoader);
							if (e->ExceptionCheck()) { e->ExceptionClear(); loader = nullptr; }

							if (loader)
							{
								jstring className = e->NewStringUTF("net/minecraft/client/Minecraft");
								if (className)
								{
									jclass cls = reinterpret_cast<jclass>(
										e->CallStaticObjectMethod(clsClass, mForName, className,
											static_cast<jboolean>(JNI_TRUE), loader));
									if (e->ExceptionCheck()) { e->ExceptionClear(); cls = nullptr; }
									if (cls)
									{
										foundGlobal = reinterpret_cast<jclass>(e->NewGlobalRef(cls));
										// 记住这个能加载游戏类的类加载器（后面解析玩家类要用）
										if (!gameClassLoader)
											gameClassLoader = e->NewGlobalRef(loader);
									}
								}
							}
						}

						e->PopLocalFrame(nullptr);
					}
				}
			}
		}
	}

	e->PopLocalFrame(nullptr);

	if (!foundGlobal)
	{
		status = Status_ClassNotFound;
		InfGuiLog("JNI: 未找到 net.minecraft.client.Minecraft（非 Forge/官方映射运行时？）");
		return false;
	}

	// ---------------- 3) 解析入口方法与字段（全部判空，任一失败即整体回退）----------------
	mcClass = foundGlobal;
	mGetInstance = env->GetStaticMethodID(mcClass, "getInstance", "()Lnet/minecraft/client/Minecraft;");
	if (env->ExceptionCheck()) { env->ExceptionClear(); mGetInstance = nullptr; }

	fHitResult = env->GetFieldID(mcClass, "hitResult", "Lnet/minecraft/world/phys/HitResult;");
	if (env->ExceptionCheck()) { env->ExceptionClear(); fHitResult = nullptr; }

	// 本地玩家字段（用于排除自己）；字段声明类型在不同版本略有差异，逐级回退
	if (!fPlayer)
	{
		fPlayer = env->GetFieldID(mcClass, "player", "Lnet/minecraft/client/player/LocalPlayer;");
		if (env->ExceptionCheck()) { env->ExceptionClear(); fPlayer = nullptr; }
	}
	if (!fPlayer)
	{
		fPlayer = env->GetFieldID(mcClass, "player", "Lnet/minecraft/client/player/AbstractClientPlayer;");
		if (env->ExceptionCheck()) { env->ExceptionClear(); fPlayer = nullptr; }
	}

	// 玩家类：TargetHUD 仅识别玩家（对应 Rise 的 AbstractClientPlayer 过滤）
	if (env->PushLocalFrame(16) == JNI_OK)
	{
		playerClass = FindGameClass("net/minecraft/world/entity/player/Player");
		env->PopLocalFrame(nullptr);
	}

	// 注意：playerClass 解析失败不再直接失败 —— 会用类名判定回退，仍然只显示玩家
	if (!mGetInstance || !fHitResult)
	{
		if (playerClass) { env->DeleteGlobalRef(playerClass); playerClass = nullptr; }
		env->DeleteGlobalRef(mcClass);
		mcClass = nullptr;
		mGetInstance = nullptr;
		fHitResult = nullptr;
		fPlayer = nullptr;
		status = Status_InitFailed;
		InfGuiLog("JNI: 映射不匹配 (getInstance=%d hitResult=%d player字段=%d)",
			mGetInstance != nullptr, fHitResult != nullptr, fPlayer != nullptr);
		return false;
	}

	status = Status_Ready;
	InfGuiLog("JNI: 就绪 (Player类=%s, 本地玩家字段=%s)",
		playerClass ? "已解析" : "未解析(改用类名判定)", fPlayer ? "有" : "无");
	return true;
}

// 用记录下来的游戏类加载器解析类；失败返回 nullptr（会抛的异常一律清掉）
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

	// 2) JVMTI：在已加载类里查（Forge/ModLauncher 下最可靠）
	{
		jclass viaJvmti = FindLoadedClassJvmti(name);
		if (viaJvmti) return viaJvmti;
	}

	// 3) 游戏类加载器（Forge/NeoForge 的 TransformingClassLoader 等）
	if (!gameClassLoader || !mClassForName || !clsClassRef)
		return nullptr;

	jstring jname = e->NewStringUTF(name);
	if (!jname) return nullptr;

	jclass cls = reinterpret_cast<jclass>(
		e->CallStaticObjectMethod(clsClassRef, mClassForName, jname,
			static_cast<jboolean>(JNI_TRUE), gameClassLoader));
	if (e->ExceptionCheck()) { e->ExceptionClear(); cls = nullptr; }

	jclass result = nullptr;
	if (cls) result = reinterpret_cast<jclass>(e->NewGlobalRef(cls));
	e->DeleteLocalRef(jname);
	return result;
}

// 目标是否是玩家：优先 Player 类型判定；playerClass 未解析时按类名层级回退
bool MinecraftJniReader::IsPlayerEntity(jobject entity, jclass clsEntity)
{
	JNIEnv* e = env;
	if (!e || !entity || !clsEntity) return false;

	// 1) 类型判定（最可靠）
	if (playerClass)
		return e->IsInstanceOf(entity, playerClass) == JNI_TRUE;

	// 2) 回退：遍历类名（自身 + 最多 8 层父类）
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
			std::string name = c ? c : "";
			if (c) e->ReleaseStringUTFChars(js, c);

			if (name.find("LocalPlayer") != std::string::npos) return false;    // 自己
			if (name == "net.minecraft.world.entity.player.Player") return true;
			if (name.find(".player.") != std::string::npos) return true;        // RemotePlayer / AbstractClientPlayer
		}

		jclass super = e->GetSuperclass(cur);
		if (e->ExceptionCheck()) { e->ExceptionClear(); break; }
		if (!super) break;
		cur = super;
	}
	return false;
}

bool MinecraftJniReader::ReadTargetLocked(JniTargetSnapshot& out)
{
	auto now = std::chrono::steady_clock::now();
	JNIEnv* e = env;

	if (!mcClass || !mGetInstance || !fHitResult)
		return false;

	jobject instance = e->CallStaticObjectMethod(mcClass, mGetInstance);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!instance) return false; // 不在世界中

	jobject hit = e->GetObjectField(instance, fHitResult);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!hit) return false;

	jclass clsHit = e->GetObjectClass(hit);
	if (!clsHit) return false;

	if (!mGetType)
	{
		mGetType = e->GetMethodID(clsHit, "getType", "()Lnet/minecraft/world/phys/HitResult$Type;");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetType = nullptr; }
	}
	if (!mGetType) return false;

	jobject type = e->CallObjectMethod(hit, mGetType);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!type) return false;

	// hitTypeClass 与 fEntityEnum 必须同时有效，否则整体回退（不能留下半初始化状态）
	if (hitTypeClass && !fEntityEnum)
	{
		e->DeleteGlobalRef(hitTypeClass);
		hitTypeClass = nullptr;
	}
	if (!hitTypeClass)
	{
		jclass t = e->GetObjectClass(type);
		if (!t) return false;
		hitTypeClass = reinterpret_cast<jclass>(e->NewGlobalRef(t));
		if (!hitTypeClass) return false;

		fEntityEnum = e->GetStaticFieldID(hitTypeClass, "ENTITY", "Lnet/minecraft/world/phys/HitResult$Type;");
		if (e->ExceptionCheck()) { e->ExceptionClear(); fEntityEnum = nullptr; }
		if (!fEntityEnum)
		{
			e->DeleteGlobalRef(hitTypeClass);
			hitTypeClass = nullptr;
			return false;
		}
	}

	jobject entityEnum = e->GetStaticObjectField(hitTypeClass, fEntityEnum);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!entityEnum) return false;
	if (!e->IsSameObject(type, entityEnum))
		return false; // 准星指向方块或未命中

	if (!mGetEntity)
	{
		mGetEntity = e->GetMethodID(clsHit, "getEntity", "()Lnet/minecraft/world/entity/Entity;");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetEntity = nullptr; }
	}
	if (!mGetEntity) return false;

	jobject entity = e->CallObjectMethod(hit, mGetEntity);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!entity) return false;

	jclass clsEntity = e->GetObjectClass(entity);
	if (!clsEntity) return false;

	// ---- 仅识别玩家（对应 Rise 的 AbstractClientPlayer 过滤）----
	// 怪物 / 动物 / 盔甲架等一律忽略，避免 TargetHUD 对着生物乱显示
	if (!IsPlayerEntity(entity, clsEntity))
		return false;

	// ---- 排除本地玩家自己 ----
	if (fPlayer)
	{
		jobject localPlayer = e->GetObjectField(instance, fPlayer);
		if (e->ExceptionCheck()) { e->ExceptionClear(); localPlayer = nullptr; }
		if (localPlayer && e->IsSameObject(localPlayer, entity))
			return false;
	}

	if (!mGetId)
	{
		mGetId = e->GetMethodID(clsEntity, "getId", "()I");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetId = nullptr; }
	}
	if (!mGetId) return false;

	jint entityId = e->CallIntMethod(entity, mGetId);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }

	if (!mGetHealth)
	{
		mGetHealth = e->GetMethodID(clsEntity, "getHealth", "()F");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetHealth = nullptr; }
	}
	if (!mGetHealth) return false; // 非生物实体（箭、掉落物等）

	jfloat health = e->CallFloatMethod(entity, mGetHealth);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }

	jfloat maxHealth = 20.0f;
	if (!mGetMaxHealth)
	{
		mGetMaxHealth = e->GetMethodID(clsEntity, "getMaxHealth", "()F");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetMaxHealth = nullptr; }
	}
	if (mGetMaxHealth)
	{
		maxHealth = e->CallFloatMethod(entity, mGetMaxHealth);
		if (e->ExceptionCheck()) { e->ExceptionClear(); maxHealth = 20.0f; }
	}

	std::string name;
	if (!mGetName)
	{
		mGetName = e->GetMethodID(clsEntity, "getName", "()Lnet/minecraft/network/chat/Component;");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetName = nullptr; }
	}
	jobject component = mGetName ? e->CallObjectMethod(entity, mGetName) : nullptr;
	if (e->ExceptionCheck()) { e->ExceptionClear(); component = nullptr; }

	if (component)
	{
		jclass clsComponent = e->GetObjectClass(component);
		if (clsComponent)
		{
			if (!mGetString)
			{
				mGetString = e->GetMethodID(clsComponent, "getString", "()Ljava/lang/String;");
				if (e->ExceptionCheck()) { e->ExceptionClear(); mGetString = nullptr; }
			}
			if (!mGetText)
			{
				mGetText = e->GetMethodID(clsComponent, "getText", "()Ljava/lang/String;");
				if (e->ExceptionCheck()) { e->ExceptionClear(); mGetText = nullptr; }
			}

			jstring jname = nullptr;
			if (mGetString)
			{
				jname = reinterpret_cast<jstring>(e->CallObjectMethod(component, mGetString));
				if (e->ExceptionCheck()) { e->ExceptionClear(); jname = nullptr; }
			}
			if (!jname && mGetText)
			{
				jname = reinterpret_cast<jstring>(e->CallObjectMethod(component, mGetText));
				if (e->ExceptionCheck()) { e->ExceptionClear(); jname = nullptr; }
			}

			if (jname)
			{
				const char* chars = e->GetStringUTFChars(jname, nullptr);
				if (chars)
				{
					name = chars;
					e->ReleaseStringUTFChars(jname, chars);
				}
			}
		}
	}

	out.valid = true;
	out.entityId = (int)entityId;
	out.name = name;
	out.health = health;
	out.maxHealth = maxHealth > 1.0f ? maxHealth : 20.0f;
	out.time = now;
	return true;
}

bool MinecraftJniReader::GetTarget(JniTargetSnapshot& out)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (status == Status_Ready)
	{
		if (!env) return false;
	}
	else
	{
		// 失败次数达上限后永久放弃（不再做任何 JNI 调用）
		if (classAttempts >= kMaxClassAttempts)
			return false;

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
