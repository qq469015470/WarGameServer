#include "GameCore.h"

SnakeBody::SnakeBody(glm::vec2 _position):
	position(_position)
{

}

void SnakeBody::SetPosition(glm::vec2 _position)
{
	this->position = _position;	
}

const glm::vec2& SnakeBody::GetPosition() const
{
	return this->position;
}

Snake::Snake(std::unique_ptr<ISnakeBody>&& _head):
	dir{0.0, 1.0}
{
	this->bodys.push_front(std::move(_head));
}

std::vector<ISnakeBody*> Snake::GetBodys()
{
	std::vector<ISnakeBody*> temp;
	for(const auto& item: this->bodys)
	{
		temp.push_back(item.get());	
	}

	return temp;
}

void Snake::Move(glm::vec2 _dir)
{
	if(_dir == glm::vec2{0,0})
		throw std::logic_error("direction should not be zero");
		
	this->dir = glm::normalize(_dir);
}

void Snake::FixedUpdate(const float& _deltaTime)
{
	this->moveTime += _deltaTime;

	while(this->moveTime >= 0.5f)
	{
		this->moveTime -= 0.5f;

		constexpr float moveSpeed = 1.0f;
	
		ISnakeBody* head(this->bodys.front().get());
		glm::vec2 headLasPos(head->GetPosition());
		head->SetPosition(headLasPos + this->dir * moveSpeed);
	
		ISnakeBody* foot(this->bodys.back().get());
		if(foot != head)
			foot->SetPosition(headLasPos);
	}
}


ISnake* GameScene::AddSnake(int _snakeId)
{
	std::unique_ptr<ISnakeBody> body(new SnakeBody({0, 0}));
	std::unique_ptr<ISnake> snake(new Snake(std::move(body)));

	ISnake* pointer(snake.get());

	this->snakes.insert(std::pair<ISnake*, std::unique_ptr<ISnake>>(pointer, std::move(snake)));

	this->snakesVec.clear();
	for(const auto& item: this->snakes)
	{
		this->snakesVec.push_back(item.first);
	}

	return pointer;
}

std::vector<ISnake*> GameScene::GetSnakes()
{
	return this->snakesVec;
}

void GameScene::RemoveSnake(ISnake* _snake)
{
	this->snakes.erase(_snake);

	this->snakesVec.clear();
	for(const auto& item: this->snakes)
	{
		this->snakesVec.push_back(item.first);
	}

}

void GameScene::FixedUpdate(const float& _deltaTime)
{
	for(auto& item: this->snakes)
	{
		item.second->FixedUpdate(_deltaTime);
	}
}
