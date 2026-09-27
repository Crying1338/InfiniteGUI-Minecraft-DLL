#include "MinecraftJniReader.h"

#include <Windows.h>

// ============================================================
// MinecraftJniReader 实现
// 所有 JNI 调用都做了异常检查（ExceptionCheck/ExceptionClear），
// 任何失败路径都会安全返回 false，不会向 JVM 抛出未处理异常。
// ============================================================

MinecraftJniReader::~MinecraftJniReader()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (env)
	{
		if (mcClass) env->DeleteGlobalRef(mcClass);
		if (hitTypeClass) env->DeleteGlobalRef(hitTypeClass);
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
	return true;
}

bool MinecraftJniReader::EnsureClass()
{
	if (mcClass)
	{
		status = Status_Ready;
		return true;
	}

	JNIEnv* e = env;
	if (e->PushLocalFrame(64) != JNI_OK)
	{
		status = Status_InitFailed;
		return false;
	}

	jclass found = nullptr;

	// 1) 直接查找（系统类加载器可见时）
	found = e->FindClass("net/minecraft/client/Minecraft");
	if (!found && e->ExceptionCheck())
		e->ExceptionClear();

	// 2) 遍历线程的 ContextClassLoader 后 Class.forName（兼容 Forge/NeoForge 分层类加载器）
	if (!found)
	{
		jclass clsThread = e->FindClass("java/lang/Thread");
		if (!clsThread)
		{
			if (e->ExceptionCheck()) e->ExceptionClear();
		}
		else
		{
			jmethodID mAllStackTraces = e->GetStaticMethodID(clsThread, "getAllStackTraces", "()Ljava/util/Map;");
			jmethodID mCtxLoader = e->GetMethodID(clsThread, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
			jobject map = mAllStackTraces ? e->CallStaticObjectMethod(clsThread, mAllStackTraces) : nullptr;
			if (e->ExceptionCheck()) { e->ExceptionClear(); map = nullptr; }

			if (map)
			{
				jclass clsMap = e->GetObjectClass(map);
				jmethodID mValues = e->GetMethodID(clsMap, "values", "()Ljava/util/Collection;");
				jobject collection = mValues ? e->CallObjectMethod(map, mValues) : nullptr;
				if (e->ExceptionCheck()) { e->ExceptionClear(); collection = nullptr; }

				if (collection)
				{
					jclass clsCol = e->GetObjectClass(collection);
					jmethodID mToArray = e->GetMethodID(clsCol, "toArray", "()[Ljava/lang/Object;");
					jobjectArray arr = mToArray
						? reinterpret_cast<jobjectArray>(e->CallObjectMethod(collection, mToArray))
						: nullptr;
					if (e->ExceptionCheck()) { e->ExceptionClear(); arr = nullptr; }

					if (arr)
					{
						jsize count = e->GetArrayLength(arr);
						jclass clsClass = e->FindClass("java/lang/Class");
						if (!clsClass && e->ExceptionCheck()) e->ExceptionClear();
						jmethodID mForName = clsClass
							? e->GetStaticMethodID(clsClass, "forName",
								"(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;")
							: nullptr;

						if (mForName)
						{
							for (jsize i = 0; i < count && !found; i++)
							{
								jobject thread = e->GetObjectArrayElement(arr, i);
								if (!thread) continue;

								jobject loader = e->CallObjectMethod(thread, mCtxLoader);
								if (e->ExceptionCheck()) { e->ExceptionClear(); loader = nullptr; }
								if (!loader) continue;

								jstring className = e->NewStringUTF("net.minecraft.client.Minecraft");
								jclass cls = reinterpret_cast<jclass>(
									e->CallStaticObjectMethod(clsClass, mForName, className, JNI_TRUE, loader));
								if (e->ExceptionCheck()) { e->ExceptionClear(); cls = nullptr; }
								if (cls)
									found = cls;
							}
						}
					}
				}
			}
		}
	}

	if (found)
	{
		mcClass = reinterpret_cast<jclass>(e->NewGlobalRef(found));
		mGetInstance = e->GetStaticMethodID(mcClass, "getInstance", "()Lnet/minecraft/client/Minecraft;");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetInstance = nullptr; }
		fHitResult = e->GetFieldID(mcClass, "hitResult", "Lnet/minecraft/world/phys/HitResult;");
		if (e->ExceptionCheck()) { e->ExceptionClear(); fHitResult = nullptr; }

		if (!mGetInstance || !fHitResult)
		{
			env->DeleteGlobalRef(mcClass);
			mcClass = nullptr;
			e->PopLocalFrame(nullptr);
			status = Status_InitFailed;
			return false;
		}
	}

	e->PopLocalFrame(nullptr);

	if (!mcClass)
	{
		status = Status_ClassNotFound;
		return false;
	}

	status = Status_Ready;
	return true;
}

bool MinecraftJniReader::ReadTargetLocked(JniTargetSnapshot& out)
{
	auto now = std::chrono::steady_clock::now();
	JNIEnv* e = env;

	jobject instance = e->CallStaticObjectMethod(mcClass, mGetInstance);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!instance) return false; // 不在世界中

	jobject hit = e->GetObjectField(instance, fHitResult);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!hit) return false;

	jclass clsHit = e->GetObjectClass(hit);

	if (!mGetType)
	{
		mGetType = e->GetMethodID(clsHit, "getType", "()Lnet/minecraft/world/phys/HitResult$Type;");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetType = nullptr; }
		if (!mGetType) return false;
	}

	jobject type = e->CallObjectMethod(hit, mGetType);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!type) return false;

	if (!hitTypeClass)
	{
		hitTypeClass = reinterpret_cast<jclass>(e->NewGlobalRef(e->GetObjectClass(type)));
		fEntityEnum = e->GetStaticFieldID(hitTypeClass, "ENTITY", "Lnet/minecraft/world/phys/HitResult$Type;");
		if (e->ExceptionCheck()) { e->ExceptionClear(); fEntityEnum = nullptr; }
		if (!fEntityEnum) return false;
	}

	jobject entityEnum = e->GetStaticObjectField(hitTypeClass, fEntityEnum);
	if (!entityEnum) return false;
	if (!e->IsSameObject(type, entityEnum))
		return false; // 准星指向方块或未命中

	if (!mGetEntity)
	{
		mGetEntity = e->GetMethodID(clsHit, "getEntity", "()Lnet/minecraft/world/entity/Entity;");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetEntity = nullptr; }
		if (!mGetEntity) return false;
	}

	jobject entity = e->CallObjectMethod(hit, mGetEntity);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }
	if (!entity) return false;

	jclass clsEntity = e->GetObjectClass(entity);

	if (!mGetId)
	{
		mGetId = e->GetMethodID(clsEntity, "getId", "()I");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetId = nullptr; }
		if (!mGetId) return false;
	}
	jint entityId = e->CallIntMethod(entity, mGetId);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }

	if (!mGetHealth)
	{
		mGetHealth = e->GetMethodID(clsEntity, "getHealth", "()F");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetHealth = nullptr; }
		if (!mGetHealth) return false; // 非生物实体（如箭、物品实体）
	}
	jfloat health = e->CallFloatMethod(entity, mGetHealth);
	if (e->ExceptionCheck()) { e->ExceptionClear(); return false; }

	if (!mGetMaxHealth)
	{
		mGetMaxHealth = e->GetMethodID(clsEntity, "getMaxHealth", "()F");
		if (e->ExceptionCheck()) { e->ExceptionClear(); mGetMaxHealth = nullptr; }
	}
	jfloat maxHealth = 20.0f;
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

	if (status != Status_Ready)
	{
		auto now = std::chrono::steady_clock::now();
		if (now < nextRetry)
			return false;
		nextRetry = now + std::chrono::milliseconds(3000);

		if (!EnsureJvm()) return false;
		if (!EnsureClass()) return false;
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
