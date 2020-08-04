#include "../ChatService.h"
#include "../Database.h"

TEST(TestChatService, SendVaildMessage)
{
	ChatService chatService;
	UserService userService;
	
	const char* userCookie = "5349b4ddd2781d08c09890f3";

	auto user = userService.GetUser(userCookie);

	EXPECT_NO_THROW(chatService.SendMessage("5f29092ac5a7f99f37afc548", user->name, "hello!"));

	db::Database db;

	EXPECT_TRUE(db.GetTable("Chat5f29092ac5a7f99f37afc548")
			->Query()
			.Equal("name", "testB")
			.Equal("message", "hello!")
			.FindOne().has_value());
}
