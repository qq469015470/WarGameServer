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

Snake::Snake():
	dir{0.0, 1.0},
	moveTime(0)
{
	std::unique_ptr<ISnakeBody> head(new SnakeBody({0, 0}));

	this->bodys.push_front(std::move(head));

	//蛇身距离间隔
	constexpr float space = 20.0f;
                                                      

	for(int i = 1; i <= 5; i++)
	{
		auto pos = this->bodys.back()->GetPosition();

		std::unique_ptr<ISnakeBody> body(new SnakeBody(pos + glm::vec2{space, 0}));
		this->bodys.push_back(std::move(body));
	}
}

void Snake::AddBody()
{
	auto pos = this->bodys.front()->GetPosition();

	std::unique_ptr<ISnakeBody> head(new SnakeBody(pos));
	this->bodys.push_back(std::move(head));	
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
		return;	

	this->dir = glm::normalize(_dir);
}

void Snake::FixedUpdate(const float& _deltaTime)
{
	this->moveTime += _deltaTime;

	constexpr float moveTick = 0.05f;

	while(this->moveTime >= moveTick)
	{
		this->moveTime -= moveTick;

		constexpr float moveSpeed = 5.0f;
	
		ISnakeBody* head(this->bodys.front().get());
		glm::vec2 headLasPos(head->GetPosition());
		head->SetPosition(headLasPos + this->dir * moveSpeed);
	
		ISnakeBody* foot(this->bodys.back().get());
		if(foot != head)
		{
			foot->SetPosition(headLasPos);

			//尾部移动到头部后方
			this->bodys.insert(++this->bodys.begin(), nullptr);//插入空元素方便移动

			std::iter_swap(--this->bodys.end(), ++this->bodys.begin());
			this->bodys.erase(--this->bodys.end());

		}
	}
}


ISnake* GameScene::AddSnake(int _snakeId)
{	
	std::unique_ptr<ISnake> snake(new Snake());

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
