#include "../Service/ChatService.h"
#include "../Service/Database.h"

TEST(TestChatService, SendVaildMessage)
{
	ChatService chatService;
	UserService userService;
	
	const char* userCookie = "5349b4ddd2781d08c09890f3";

	auto user = userService.GetUser(userCookie);

	EXPECT_NO_THROW(chatService.SendMessage("5f29092ac5a7f99f37afc548", user->name, "hello!"));
	EXPECT_THROW(chatService.SendMessage("none", user->name, "hello!"), std::logic_error);

	db::Database db;

	EXPECT_TRUE(db.Query().Equal("name", "Chat5f29092ac5a7f99f37afc548")
			.FindOne()
			->Query()
			.Equal("name", "testB")
			.Equal("message", "hello!")
			.FindOne().has_value());
}
