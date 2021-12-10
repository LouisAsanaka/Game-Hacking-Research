#include "esp.h"

bool ESP::IsTeamGame() {
	int mode = *gameMode;
	return mode == 0 || mode == 4 || mode == 5 || mode == 7 || mode == 11 || mode == 13 ||
		mode == 14 || mode == 16 || mode == 17 || mode == 20 || mode == 21;
}

bool ESP::IsEnemy(Entity* entity) {
	return localPlayer->Team != entity->Team;
}

bool ESP::IsValidEntity(Entity* entity) {
	return entity != nullptr && (entity->vtable == 0x4e4a98 || entity->vtable == 0x4e4ac0);
}

void ESP::DrawESPBox(Entity* entity, Vector3 screen, GL::Font& font) {
	const GLubyte* color = nullptr;
	if (IsTeamGame() && !IsEnemy(entity)) {
		color = rgb::green;
	} else {
		color = rgb::red;
	}
	float d = localPlayer->NewPosition.Distance(entity->NewPosition);
	
	float scale = (GAME_UNIT_MAGIC / d) * (viewport[2] / VIRTUAL_SCREEN_WIDTH);
	float x = screen.x - scale;
	float y = screen.y - scale * PLAYER_ASPECT_RATIO;
	float width = scale * 2;
	float height = scale * PLAYER_ASPECT_RATIO * 2;

	GL::DrawOutline(x, y, width, height, 2.0f, color);

	float textX = font.CenterText(x, width, static_cast<float>(strlen(entity->Name)) * ESP_FONT_WIDTH);
	float textY = y - ESP_FONT_HEIGHT / 2;
	font.Print(textX, textY, color, "%s", entity->Name);
}

void ESP::Draw(GL::Font& font) {
	glGetIntegerv(GL_VIEWPORT, viewport);
	for (int i = 0; i < *numOfPlayers; i++) {
		if (IsValidEntity(entityList->entities[i])) {
			Entity* entity = entityList->entities[i];
			Vector3 center = entity->HeadLocation;
			center.z = center.z - EYE_HEIGHT + PLAYER_HEIGHT / 2;

			Vector3 screenCoords;
			if (WorldToScreen(center, screenCoords, matrix, viewport[2], viewport[3])) {
				DrawESPBox(entity, screenCoords, font);
			}
		}
	}
}