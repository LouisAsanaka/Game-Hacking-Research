#pragma once

#include <Windows.h>
#include <cstdint>
#include "geometry.h"

// Created with ReClass.NET 1.2 by KN4CK3R

class Entity
{
public:
	DWORD vtable;
	Vector3 HeadLocation; //0x0004
	Vector3 Velocity; //0x0010
	Vector3 DeltaVelocity; //0x001C
	Vector3 DeltaPosition; //0x0028
	Vector3 NewPosition; //0x0034
	Vector3 Angle; //0x0040
	float PitchVelocity; //0x004C
	float MaxSpeed; //0x0050
	int TimeInAir; //0x0054
	float Radius; //0x0058
	float EyeHeight; //0x005C
	float MaxEyeHeight; //0x0060
	float AboveEye; //0x0064
	bool InWater; //0x0068
	bool OnFloor; //0x0069
	bool OnLadder; //0x006A
	bool JumpNext; //0x006B
	bool Crouching; //0x006C
	bool CrouchedInAir; //0x006D
	bool TryCrouch; //0x006E
	bool CanCollide; //0x006F
	bool Stuck; //0x0070
	bool Scoping; //0x0071
	bool Shoot; //0x0072
	char pad_0073[1]; //0x0073
	int LastJump; //0x0074
	float LastJumpHeight; //0x0078
	int LastSplash; //0x007C
	BYTE Move; //0x0080
	BYTE Strafe; //0x0081
	BYTE State; //0x0082
	BYTE Type; //0x0083
	float EyeHeightVelocity; //0x0084
	char pad_0088[112]; //0x0088
	int Health; //0x00F8
	int Armor; //0x00FC
	int Primary; //0x0100
	int NextPrimary; //0x0104
	int GunSelect; //0x0108
	bool Akimbo; //0x010C
	char pad_010D[279]; //0x010D
	bool Attacking; //0x0224
	char Name[16]; // 0x0225
	char pad_0235[247];
	BYTE Team; //0x032C
	char pad_032D[11]; //0x032D
	char pad_0338[60]; //0x0338
	class Weapon* CurrentWeapon; //0x0374
};

struct EntityList {
	Entity* entities[31];
};

class Weapon
{
public:
	char pad_0000[4]; //0x0000
	int WeaponId; //0x0004
	class Player* WeaponOwner; //0x0008
	class GunInfo* GunInfo; //0x000C
	class AmmoPtr* AmmoReserve; //0x0010
	class AmmoPtr* AmmoMag; //0x0014
	char pad_0018[16]; //0x0018
}; //Size: 0x0028

class GunInfo
{
public:
	char pad_0000[260]; //0x0000
	short Sound; //0x0104
	short Reload; //0x0106
	short ReloadTime; //0x0108
	short AttackDelay; //0x010A
	short Damage; //0x010C
	short Piercing; //0x010E
	short ProjSpeed; //0x0110
	short Part; //0x0112
	short Spread; //0x0114
	short Recoil; //0x0116
	short MagSize; //0x0118
	short mdl_kick_rot; //0x011A
	short mdl_kick_back; //0x011C
	short recoilincrease; //0x011E
	short recoilbase; //0x0120
	short maxrecoil; //0x0122
	short recoilbackfade; //0x0124
	short pushfactor; //0x0126
	short isauto; //0x0128
}; //Size: 0x012A

class AmmoPtr
{
public:
	int32_t Ammo; //0x0000
}; //Size: 0x0004

struct traceresult_s
{
	Vector3 end;
	bool collided;
};