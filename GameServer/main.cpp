#include "../GameCore/GameCore.h"

#include <iostream>
#include <chrono>

void GameLoop()
{
	GameScene scene;
	
	scene.AddSnake(0);

	std::chrono::steady_clock::time_point past(std::chrono::steady_clock::now());

	while(true)
	{
		auto now = std::chrono::steady_clock::now();
		auto timespan = std::chrono::duration_cast<std::chrono::milliseconds>(now - past);

		constexpr float deltaTime = 1.0f / 20.0f;
		constexpr float deltaTimeMills = deltaTime * 1000;

		if(timespan.count() < deltaTimeMills || timespan.count() == 0)
			continue;

		past = now;

		int updateCount(timespan.count() / deltaTimeMills);

		past += std::chrono::milliseconds(static_cast<int>(updateCount * deltaTimeMills));

		for(int i = 0; i < updateCount; i++)
		{
			scene.FixedUpdate(deltaTime);	
		}

		for(const auto& item: scene.GetSnakes())
		{
			const glm::vec2 pos = item->GetBodys()[0]->GetPosition();
			std::cout << "x: " << pos.x << " y:" << pos.y << std::endl;
		}
	}
}

int main(int _argc, char** _args)
{
	GameLoop();

	return 0;
}
