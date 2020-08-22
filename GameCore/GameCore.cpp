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
	constexpr float space = 0.0f;
                                                      

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

std::vector<ISnakeBody*> Snake::GetBodys() const
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

	constexpr float moveTick = 1.0f / 32.0f;

	while(this->moveTime >= moveTick)
	{
		this->moveTime -= moveTick;

		constexpr float moveSpeed = 7.5f;
	
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

Food::Food():
	pos(glm::vec2{0, 0})
{
}

uint8_t Food::GetId() const 
{
	return static_cast<uint8_t>(ItemId::Food);
}

void Food::SetPosition(glm::vec2 _position)
{
	this->pos = _position;
}

const glm::vec2& Food::GetPosition() const
{
	return this->pos;
}

GameScene::GameScene():
	foodTime(0),
	itemAddCallback([](const IItem*){}),
	itemRemoveCallback([](const IItem*){})
{

}

ISnake* GameScene::AddSnake(int _snakeId)
{	
	std::unique_ptr<ISnake> snake(new Snake());

	ISnake* pointer(snake.get());

	this->snakes.insert(std::pair<ISnake*, std::unique_ptr<ISnake>>(pointer, std::move(snake)));

	this->snakesVec.push_back(pointer);

	return pointer;
}

std::vector<ISnake*> GameScene::GetSnakes() const
{
	return this->snakesVec;
}

void GameScene::RemoveSnake(ISnake* _snake)
{
	this->snakes.erase(_snake);

	for(auto iter = this->snakesVec.begin(); iter != this->snakesVec.end(); iter++)
	{
		if(*iter == _snake)
		{
			this->snakesVec.erase(iter);
			return;
		}
	}

	throw std::logic_error("snake pointer not exists");
}

IItem* GameScene::AddItem(ItemId _id, glm::vec2 _position)
{
	IItem* result(nullptr);
	switch(_id)
	{
		case ItemId::Food:
		{
			std::unique_ptr<IItem> temp(new Food());
	
			result = temp.get();
			this->items.insert(std::pair<IItem*, std::unique_ptr<IItem>>(result, std::move(temp)));
			break;
		}
		default:
		{
			throw std::logic_error("item id not exists");
			break;
		}
	}

	this->itemsVec.push_back(result);
	result->SetPosition(_position);
	this->itemAddCallback(result);
	return result;
}

void GameScene::RemoveItem(IItem* _item)
{
	this->items.erase(_item);

	for(auto iter = this->itemsVec.begin(); iter != this->itemsVec.end(); iter++)
	{
		//尽量不要在遍历时删除
		//可以用一个数组记录索引，遍历完再删除
		if(*iter == _item)
		{
			this->itemsVec.erase(iter);
			this->itemRemoveCallback(_item);
			return;
		}
	}
	
	throw std::logic_error("can not find the item pointer!");
}

std::vector<IItem*> GameScene::GetItems() const
{
	return this->itemsVec;
}

void GameScene::SetItemNotifity(ItemAddCallback _addCallback, ItemRemoveCallback _removeCallback)
{
	this->itemAddCallback = _addCallback;
	this->itemRemoveCallback = _removeCallback;
}

void GameScene::FixedUpdate(const float& _deltaTime)
{
	if(this->itemsVec.size() < 20)
		this->foodTime -= _deltaTime;

	while(this->foodTime <= 0)
	{
		static std::uniform_real_distribution<float> range(0.1f, std::nextafter(0.5f, 1.0f));
		static std::uniform_real_distribution<float> xRange(-470.0f, std::nextafter(470.0f, 1.0f));
		static std::uniform_real_distribution<float> yRange(-310.0f, std::nextafter(310.0f, 1.0f));

		this->foodTime += range(rd);
		this->AddItem(ItemId::Food, glm::vec2{xRange(rd), yRange(rd)});
	}

	for(auto& item: this->snakes)
	{
		item.second->FixedUpdate(_deltaTime);
	}

	//反应器
	for(const auto& snake: this->snakesVec)
	{
		for(const auto& item: this->itemsVec)
		{
			if(glm::distance(item->GetPosition(), snake->GetBodys()[0]->GetPosition()) <= 10.0f)
			{
				switch(static_cast<ItemId>(item->GetId()))
				{
					case ItemId::Food:
					{
						snake->AddBody();	
						break;
					}
					default:
					{
						break;
					}
				}


				this->RemoveItem(item);
				break;
			}
		}
	}
}
