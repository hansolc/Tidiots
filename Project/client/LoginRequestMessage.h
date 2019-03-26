#pragma once
#include "nlohmann\json.hpp"
#include <iostream>

class LoginRequestMessage
{
private:
	int type;
	int id;
	int password;
public:
	LoginRequestMessage(int type_, int id_, int password_) : type(type_), id(id_), password(password_) {};
	int GetType() {};
};
