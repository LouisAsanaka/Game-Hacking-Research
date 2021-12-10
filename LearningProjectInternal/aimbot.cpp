#include "aimbot.h"
#include <iostream>
#include <cmath>

constexpr float PI = 3.14159265358979323846f;

bool Aimbot::IsTeamGame() {
    int mode = *gameMode;
    return mode == 0 || mode == 4 || mode == 5 || mode == 7 || mode == 11 || mode == 13 ||
        mode == 14 || mode == 16 || mode == 17 || mode == 20 || mode == 21;
}

bool Aimbot::IsEnemy(Entity* entity) {
    return localPlayer->Team != entity->Team;
}

bool Aimbot::IsValidEntity(Entity* entity) {
    return entity != nullptr && (entity->vtable == 0x4e4a98 || entity->vtable == 0x4e4ac0);
}

bool Aimbot::CanSee(Entity* to) {
    uintptr_t tracelineFunction = 0x48a310;
    traceresult_s traceresult;
    traceresult.collided = false;
    Vector3 from_vec = localPlayer->HeadLocation;
    Vector3 to_vec = to->HeadLocation;

    __asm {
        push 0; // SkipTags
        push 0; // CheckPlayers
        push localPlayer;
        push to_vec.z;
        push to_vec.y;
        push to_vec.x;
        push from_vec.z;
        push from_vec.y;
        push from_vec.x;
        lea eax, [traceresult];
        call tracelineFunction;
        add esp, 36;
    }
    return !traceresult.collided;
}

void Aimbot::Update() {
    Entity* closest = nullptr;
    float shortestDistance = 9999999.0f;
    for (int i = 0; i < *numOfPlayers; i++) {
        if (IsValidEntity(entityList->entities[i])) {
            Entity* entity = entityList->entities[i];
            if (CanSee(entity) && entity->Team != localPlayer->Team && entity->State == 0) {
                float distance = entity->HeadLocation.DistanceSquared(localPlayer->HeadLocation);
                if (distance < shortestDistance) {
                    shortestDistance = distance;
                    closest = entity;
                }
            }
        }
    }
    if (closest == nullptr) {
        localPlayer->Attacking = false;
        return;
    }
    //std::cout << "Closest: " << closest->Name << std::endl;

    float dx = closest->HeadLocation.x - localPlayer->HeadLocation.x;
    float dy = closest->HeadLocation.y - localPlayer->HeadLocation.y;
    float angleYaw = atan2f(dy, dx) * 180 / PI;

    float distance = sqrtf(dx * dx + dy * dy);
    float dz = closest->HeadLocation.z - localPlayer->HeadLocation.z;
    float anglePitch = atan2f(dz, distance) * 180 / PI;

    localPlayer->Angle.x = angleYaw + 90;
    localPlayer->Angle.y = anglePitch;

    localPlayer->Attacking = true;
}