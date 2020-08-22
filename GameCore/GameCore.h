#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <list>
#include <memory>
#include <unordered_map>
#include <random>
#include <functional>

enum class ItemId:uint8_t 
{
	Food = 0
};

//物品接口(涵盖果实)
class IItem
{
public:
	virtual ~IItem() = default;

	virtual uint8_t GetId() const = 0;
	virtual void SetPosition(glm::vec2 _position) = 0;
	virtual const glm::vec2& GetPosition() const = 0;	
};

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

	virtual std::vector<ISnakeBody*> GetBodys() const = 0;

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

	virtual std::vector<ISnakeBody*> GetBodys() const override;

	virtual void AddBody() override;

	virtual void Move(glm::vec2 _dir) override;

	virtual void FixedUpdate(const float& _deltaTime) override;
};

class Food: virtual public IItem
{
private:
	glm::vec2 pos;

public:
	Food();

	virtual uint8_t GetId() const override;
	virtual void SetPosition(glm::vec2 _position) override;	
	virtual const glm::vec2& GetPosition() const override;
};

class GameScene
{
private:
	using ItemAddCallback = std::function<void(const IItem*)>;
	using ItemRemoveCallback = std::function<void(const IItem*)>;

	std::random_device rd;

	std::unordered_map<ISnake*, std::unique_ptr<ISnake>> snakes;
	std::vector<ISnake*> snakesVec;

	std::unordered_map<IItem*, std::unique_ptr<IItem>> items;
	std::vector<IItem*> itemsVec;

	ItemAddCallback itemAddCallback;
	ItemRemoveCallback itemRemoveCallback;

	//生成果实的时间
	float foodTime;
	
	IItem* AddItem(ItemId _id, glm::vec2 _position);
	void RemoveItem(IItem* _item);

public:
	GameScene();

	ISnake* AddSnake(int _snakeId);
	std::vector<ISnake*> GetSnakes() const;
	void RemoveSnake(ISnake* _snake);

	std::vector<IItem*> GetItems() const;

	void SetItemNotifity(ItemAddCallback _addCallback, ItemRemoveCallback _removeCallback);
	
	void FixedUpdate(const float& _deltaTime);
};
