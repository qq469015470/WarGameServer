#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <list>
#include <memory>
#include <unordered_map>

class ISnakeBody
{
public:
	virtual ~ISnakeBody() = default;

	virtual void SetPosition(glm::vec2 _position) = 0;
	virtual const glm::vec2& GetPosition() const = 0;
};

class ISnake
{
public:
	virtual ~ISnake() = default;

	virtual void AddBody() = 0;

	virtual std::vector<ISnakeBody*> GetBodys() = 0;

	virtual void Move(glm::vec2 _dir) = 0;
	
	virtual void FixedUpdate(const float& _deltaTime) = 0;
};

class SnakeBody: virtual public ISnakeBody
{
private:
	glm::vec2 position;

public:
	SnakeBody(glm::vec2 _position);

	virtual void SetPosition(glm::vec2 _position) override;
	virtual const glm::vec2& GetPosition() const override;
};

class Snake: virtual public ISnake
{
private:
	std::list<std::unique_ptr<ISnakeBody>> bodys;

	glm::vec2 dir;

	//距离可以移动的时间
	float moveTime;

public:
	Snake();

	virtual std::vector<ISnakeBody*> GetBodys() override;

	virtual void AddBody() override;

	virtual void Move(glm::vec2 _dir) override;

	virtual void FixedUpdate(const float& _deltaTime) override;
};

class GameScene
{
private:
	std::unordered_map<ISnake*, std::unique_ptr<ISnake>> snakes;
	std::vector<ISnake*> snakesVec;

public:
	ISnake* AddSnake(int _snakeId);
	std::vector<ISnake*> GetSnakes();
	void RemoveSnake(ISnake* _snake);
	
	void FixedUpdate(const float& _deltaTime);
};
