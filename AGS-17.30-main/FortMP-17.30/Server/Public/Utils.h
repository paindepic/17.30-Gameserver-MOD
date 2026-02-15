#pragma once

#include "Framework.h"
#include "Offsets.h"

#define RESULT_DECL void*const RESULT_PARAM
#define LOG(fmt, ...) std::cout << "LogFortMP: " << std::format(fmt, __VA_ARGS__) << "\n";
#define DEBUG
#ifdef DEBUG
#define DEBUG_LOG(fmt, ...) std::cout << "LogFortMPDebug: " << std::format(fmt, __VA_ARGS__) << "\n";
#else
#define DEBUG_LOG(fmt, ...)
#endif
inline bool bLategame = true;
inline bool EnableTeams = false;
inline bool bEnableSiphon = true;
class THook
{
private:
    void* Hook;
    void** Original;

public:
    inline THook(void* Hook, auto Original = nullptr)
    {
        this->Hook = Hook;
        this->Original = (void**)Original;
    }

    void VFT(void** VTable, int Index);
    void MinHook(uint64_t Address, bool bEnable = false);
    void Exec(UFunction* Function);

    static inline void Init()
    {
        if (MH_Initialize() != MH_OK)
            LOG("Failed to Init MinHook");
    }

    static inline void Ret() { return; }
    static inline bool RetFalse() { return false; }
    static inline bool RetTrue() { return true; }
};

template<class T>
inline T* Cast(UObject* Src)
{
    return Src && (Src->IsA(T::StaticClass())) ? (T*)Src : nullptr;
}

inline __int64(*DispatchRequest)(__int64 a1, __int64* a2, int a3);
inline __int64 DispatchRequestHook(__int64 a1, __int64* a2, int a3)
{
    return DispatchRequest(a1, a2, 3);
}

namespace Utils
{
	inline void Null(uintptr_t Address)
	{
		DWORD oldProtection;
		if (VirtualProtect(reinterpret_cast<void*>(Address), sizeof(uint8_t), PAGE_EXECUTE_READWRITE, &oldProtection))
		{
			*reinterpret_cast<uint8_t*>(Address) = 0xC3;

			DWORD tempProtection;
			VirtualProtect(reinterpret_cast<void*>(Address), sizeof(uint8_t), oldProtection, &tempProtection);
		}
	}
	
	inline void Patch(uintptr_t Func, uint8_t Byte)
	{
		DWORD dwProt;
		VirtualProtect((PVOID)Func, 1, PAGE_EXECUTE_READWRITE, &dwProt);

		*(uint8_t*)Func = Byte;

		DWORD dwTemp;
		VirtualProtect((PVOID)Func, 1, dwProt, &dwTemp);
	}

    inline void SwapVFTs(void* Base, uintptr_t Index, void* Detour, void** Original)
    {
        auto VTable = (*(void***)Base);
        if (!VTable)
            return;

        if (!VTable[Index])
            return;

        if (Original)
            *Original = VTable[Index];

        DWORD dwOld;
        VirtualProtect(&VTable[Index], 8, PAGE_EXECUTE_READWRITE, &dwOld);
        VTable[Index] = Detour;
        DWORD dwTemp;
        VirtualProtect(&VTable[Index], 8, dwOld, &dwTemp);
    }

	

	inline UObject* (*StaticFindObject_)(UClass* Class, UObject* Package, const wchar_t* OrigInName, bool ExactClass) = decltype(StaticFindObject_)(InSDKUtils::GetImageBase() + Addresses::StaticFindObject);
	template <typename T>
	inline T* StaticFindObject(std::string ObjectPath, UClass* Class = nullptr)
	{
		return (T*)StaticFindObject_(Class, nullptr, std::wstring(ObjectPath.begin(), ObjectPath.end()).c_str(), false);
	}

	inline UObject* (*StaticLoadObject_)(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, uint32_t LoadFlags, UObject* Sandbox, bool bAllowObjectReconciliation) = decltype(StaticLoadObject_)(InSDKUtils::GetImageBase() + Addresses::StaticLoadObject);
	template <typename T>
	inline T* StaticLoadObject(std::string Path, UClass* InClass = T::StaticClass(), UObject* Outer = nullptr)
	{
		return (T*)StaticLoadObject_(InClass, Outer, std::wstring(Path.begin(), Path.end()).c_str(), nullptr, 0, nullptr, false);
	}

    inline int GetOffset(UObject* Object, string name)
    {
        FProperty* Property = nullptr;

        for (UStruct* Cls = Object->Class; Cls; Cls = Cls->Super)
        {
            FField* ChildProperties = Cls->ChildProperties;
            if (ChildProperties)
            {
                Property = (FProperty*)ChildProperties;
                string PropStr = ChildProperties->Name.ToString();
                while (Property)
                {
                    if (PropStr == name)
                        return Property->Offset;

                    Property = (FProperty*)Property->Next;
                    PropStr = Property ? Property->Name.ToString() : "Invalid Property";
                }
            }
        }
        if (!Property)
            return 0;
        return Property->Offset;
    }

    template <class T>
    T* SpawnActor(FVector Location, FRotator Rotation = FRotator{ 0, 0, 0 }, UClass* Class = T::StaticClass(), FVector Scale3D = { 1,1,1 })
    {
        FTransform Transform{};
        Transform.Rotation = UKismetMathLibrary::Conv_RotatorToTransform(Rotation).Rotation;
        Transform.Scale3D = Scale3D;
        Transform.Translation = Location;

        auto Actor = UGameplayStatics::GetDefaultObj()->BeginSpawningActorFromClass(UWorld::GetWorld(), Class, Transform, false, nullptr);
        if (Actor)
            UGameplayStatics::GetDefaultObj()->FinishSpawningActor(Actor, Transform);
        return (T*)Actor;
    }

    template <class T>
    inline TArray<T*> GetAllActorsOfClass() {
        TArray<T*> ResultActors;

        if (UWorld* World = UWorld::GetWorld()) {
            TArray<AActor*> OutActors;
            UGameplayStatics::GetAllActorsOfClass(World, T::StaticClass(), &OutActors);

            for (AActor* Actor : OutActors) {
                if (T* CastedActor = Cast<T>(Actor)) {
                    ResultActors.Add(CastedActor);
                }
            }
        }
        return ResultActors;
    }

    inline void HookVTable(void* instance, uintptr_t methodIndex, void* hookFunction, void** originalFunction = nullptr) {
        if (!instance || !hookFunction)
            return;

        auto vtable = *reinterpret_cast<void***>(instance);
        if (!vtable || !vtable[methodIndex])
            return;

        if (originalFunction)
            *originalFunction = vtable[methodIndex];

        DWORD oldProtection;
        if (VirtualProtect(&vtable[methodIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection)) {
            vtable[methodIndex] = hookFunction;
            VirtualProtect(&vtable[methodIndex], sizeof(void*), oldProtection, &oldProtection);
        }
    }

    inline float FastAsin(float Value)
    {
        // Clamp input to [-1,1].
        bool nonnegative = (Value >= 0.0f);
        float x = UKismetMathLibrary::Abs(Value);
        float omx = 1.0f - x;
        if (omx < 0.0f)
        {
            omx = 0.0f;
        }
        float root = UKismetMathLibrary::Sqrt(omx);
        // 7-degree minimax approximation
        float result = ((((((-0.0012624911f * x + 0.0066700901f) * x - 0.0170881256f) * x + 0.0308918810f) * x - 0.0501743046f) * x + 0.0889789874f) * x - 0.2145988016f) * x + 1.5707963050f;
        result *= root;  // acos(|x|)
        // acos(x) = pi - acos(-x) when x < 0, asin(x) = pi/2 - acos(x)
        return (nonnegative ? 1.5707963050f - result : result - 1.5707963050f);
    }

    inline FRotator FQuatToRot(FQuat Quat)
    {
        const float SingularityTest = Quat.Z * Quat.X - Quat.W * Quat.Y;
        const float YawY = 2.f * (Quat.W * Quat.Z + Quat.X * Quat.Y);
        const float YawX = (1.f - 2.f * (UKismetMathLibrary::Square(Quat.Y) + UKismetMathLibrary::Square(Quat.Z)));
        //const float PI = 3.1415926535897932f;

        // reference 
        // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
        // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/

        // this value was found from experience, the above websites recommend different values
        // but that isn't the case for us, so I went through different testing, and finally found the case 
        // where both of world lives happily. 
        const float SINGULARITY_THRESHOLD = 0.4999995f;
        const float RAD_TO_DEG = (180.f) / PI;
        FRotator RotatorFromQuat;

        if (SingularityTest < -SINGULARITY_THRESHOLD)
        {
            RotatorFromQuat.Pitch = -90.f;
            RotatorFromQuat.Yaw = UKismetMathLibrary::Atan2(YawY, YawX) * RAD_TO_DEG;
            RotatorFromQuat.Roll = UKismetMathLibrary::NormalizeAxis(-RotatorFromQuat.Yaw - (2.f * UKismetMathLibrary::Atan2(Quat.X, Quat.W) * RAD_TO_DEG));
        }
        else if (SingularityTest > SINGULARITY_THRESHOLD)
        {
            RotatorFromQuat.Pitch = 90.f;
            RotatorFromQuat.Yaw = UKismetMathLibrary::Atan2(YawY, YawX) * RAD_TO_DEG;
            RotatorFromQuat.Roll = UKismetMathLibrary::NormalizeAxis(RotatorFromQuat.Yaw - (2.f * UKismetMathLibrary::Atan2(Quat.X, Quat.W) * RAD_TO_DEG));
        }
        else
        {
            RotatorFromQuat.Pitch = FastAsin(2.f * (SingularityTest)) * RAD_TO_DEG;
            RotatorFromQuat.Yaw = UKismetMathLibrary::Atan2(YawY, YawX) * RAD_TO_DEG;
            RotatorFromQuat.Roll = UKismetMathLibrary::Atan2(-2.f * (Quat.W * Quat.X + Quat.Y * Quat.Z), (1.f - 2.f * (UKismetMathLibrary::Square(Quat.X) + UKismetMathLibrary::Square(Quat.Y)))) * RAD_TO_DEG;
        }

        return RotatorFromQuat;
    }

    inline AFortGameStateAthena* GetGameState()
    {
        return (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
    }

    inline void sinCos(float* ScalarSin, float* ScalarCos, float Value)
    {
        float quotient = (0.31830988618f * 0.5f) * Value;
        if (Value >= 0.0f)
        {
            quotient = (float)((int)(quotient + 0.5f));
        }
        else
        {
            quotient = (float)((int)(quotient - 0.5f));
        }
        float y = Value - (2.0f * 3.1415926535897932f) * quotient;

        float sign;
        if (y > 1.57079632679f)
        {
            y = 3.1415926535897932f - y;
            sign = -1.0f;
        }
        else if (y < -1.57079632679f)
        {
            y = -3.1415926535897932f - y;
            sign = -1.0f;
        }
        else
        {
            sign = +1.0f;
        }

        float y2 = y * y;

        *ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

        float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
        *ScalarCos = sign * p;
    }

    inline FQuat FRotToQuat(FRotator Rot)
    {
        const float DEG_TO_RAD = 3.1415926535897932f / (180.f);
        const float DIVIDE_BY_2 = DEG_TO_RAD / 2.f;
        float SP, SY, SR;
        float CP, CY, CR;

        sinCos(&SP, &CP, Rot.Pitch * DIVIDE_BY_2);
        sinCos(&SY, &CY, Rot.Yaw * DIVIDE_BY_2);
        sinCos(&SR, &CR, Rot.Roll * DIVIDE_BY_2);

        FQuat RotationQuat;
        RotationQuat.X = CR * SP * SY - SR * CP * CY;
        RotationQuat.Y = -CR * SP * CY - SR * CP * SY;
        RotationQuat.Z = CR * CP * SY - SR * SP * CY;
        RotationQuat.W = CR * CP * CY + SR * SP * SY;

        return RotationQuat;
    }

   inline UFortKismetLibrary* GetFortKismet()
    {
        return (UFortKismetLibrary*)UFortKismetLibrary::StaticClass()->DefaultObject;
    }
    template<typename T>
    inline  T* SpawnActor22(FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
    {
        static UGameplayStatics* statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

        FTransform Transform{};
        Transform.Scale3D = FVector(1, 1, 1);
        Transform.Translation = Loc;

        return (T*)statics->FinishSpawningActor(statics->BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), T::StaticClass(), Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner), Transform);
    }

    template<typename T>
    inline T* SpawnActor22(UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
    {
        static UGameplayStatics* statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

        FTransform Transform{};
        Transform.Scale3D = FVector(1, 1, 1);
        Transform.Translation = Loc;
        Transform.Rotation = FRotToQuat(Rot);

        return (T*)statics->FinishSpawningActor(statics->BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner), Transform);
    }

    inline AFortGameModeAthena* GetGameMode()
    {
        return (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    }

    inline AFortPickupAthena* SpawnPickup(FFortItemEntry* ItemEntry, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, int OverrideCount = -1)
    {
        auto SpawnedPickup = Utils::SpawnActor22<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Loc);
        SpawnedPickup->bRandomRotation = true;

        auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
        PickupEntry.ItemDefinition = ItemEntry->ItemDefinition;
        PickupEntry.Count = OverrideCount != -1 ? OverrideCount : ItemEntry->Count;
        PickupEntry.LoadedAmmo = ItemEntry->LoadedAmmo;
        SpawnedPickup->OnRep_PrimaryPickupItemEntry();

        SpawnedPickup->TossPickup(Loc, nullptr, -1, true, false, SourceType, Source);

        if (SourceType == EFortPickupSourceTypeFlag::Container)
        {
            SpawnedPickup->bTossedFromContainer = true;
            SpawnedPickup->OnRep_TossedFromContainer();
        }

        return SpawnedPickup;
    }

    inline AFortPickupAthena* SpawnPickup(FVector Loc, UFortItemDefinition* Def, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource, int Count, int LoadedAmmo)
    {
        FTransform Transform{};
        Transform.Translation = Loc;
        Transform.Scale3D = FVector{ 1,1,1 };
        AFortPickupAthena* Pickup = Cast<AFortPickupAthena>(UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), AFortPickupAthena::StaticClass(), (FTransform&)Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, nullptr), (FTransform&)Transform));
        if (!Pickup)
            return nullptr;
        Pickup->bRandomRotation = true;
        Pickup->PrimaryPickupItemEntry.ItemDefinition = Def;
        Pickup->PrimaryPickupItemEntry.Count = Count;
        Pickup->PrimaryPickupItemEntry.LoadedAmmo = LoadedAmmo;
        Pickup->OnRep_PrimaryPickupItemEntry();
        Pickup->TossPickup(Loc, nullptr, -1, true, false, SourceTypeFlag, SpawnSource);

        if (SourceTypeFlag == EFortPickupSourceTypeFlag::Container)
        {
            Pickup->bTossedFromContainer = true;
            Pickup->OnRep_TossedFromContainer();
        }
        return Pickup;
    }

    inline AFortPickupAthena* SpawnPickup(UFortItemDefinition* ItemDef, int OverrideCount, int LoadedAmmo, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source)
    {
        auto SpawnedPickup = Utils::SpawnActor22<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Loc);
        SpawnedPickup->bRandomRotation = true;

        auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
        PickupEntry.ItemDefinition = ItemDef;
        PickupEntry.Count = OverrideCount;
        PickupEntry.LoadedAmmo = LoadedAmmo;
        SpawnedPickup->OnRep_PrimaryPickupItemEntry();

        SpawnedPickup->TossPickup(Loc, nullptr, -1, true, false, SourceType, Source);

        if (SourceType == EFortPickupSourceTypeFlag::Container)
        {
            SpawnedPickup->bTossedFromContainer = true;
            SpawnedPickup->OnRep_TossedFromContainer();
        }

        return SpawnedPickup;
    }

     uintptr_t inline GetOffsetA(uintptr_t offset)
    {
        return __int64(GetModuleHandleW(0)) + offset;
    }
}