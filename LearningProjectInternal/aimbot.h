#pragma once
#include <Windows.h>
#include "assaultcube.h"

class Aimbot {
public:
	int* gameMode = (int*)(0x50F49C);
	int* numOfPlayers = (int*)(0x50f500);
	float* matrix = (float*)(0x501AE8);
	Entity* localPlayer = *(Entity**)0x50F4F4;
	EntityList* entityList = *(EntityList**)0x50F4F8;

	bool IsTeamGame();
	bool IsEnemy(Entity* entity);
	bool IsValidEntity(Entity* entity);

	bool CanSee(Entity* to);
	void Update();

	float WrapXAngle(float angle) {
		if (angle > 90) angle = 90;
		else if (angle < -90) angle = -90;

		return angle;
	}

	float WrapYZAngle(float angle) {
		while (angle >= 360.0f) angle -= 360.0f;
		while (angle < 0.0f) angle += 360.0f;

		return angle;
	}
};

