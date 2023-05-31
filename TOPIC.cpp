#include "Command.hpp"

void Command::TOPIC()
{
	std::cout << this->_trailing << std::endl;
	//채널에 존재하지않을때, 채널이 없을때
	if (_sender->getServer()->getChannel(this->_params[0]) == NULL)
	{
		sendReply(_sender->getSocket(), ERR_NOSUCHNICK(_server->getName(), _sender->getNick(), this->_params[0]));
		return ;
	} // op만 할 수 있고, op권한이 없을때(MODE에서 k옵션 들어있는지 불러와서 확인해야함)
	else if (_sender->getServer()->getChannel(this->_params[0])->isOperator(_sender) == false)
	{
		sendReply(_sender->getSocket(), ERR_CHANOPRIVSNEEDED_TOPIC(_server->getName(), _sender->getNick(), this->_params[0]));
		return ;
	}
	if (this->_trailing.empty() == false)
	{
		_sender->getServer()->getChannel(this->_params[0])->setTopic(this->_trailing);
		std::string message = RPL_SETTOPIC(_sender->getNick(), _sender->getUsername(), _sender->getIP(), this->_params[0], this->_trailing);
		_sender->getServer()->getChannel(this->_params[0])->sendReply(message);
		return ;
	}
	else //topic을 설정하는게아니고 topic을 불러오는건 client에서 관리되는듯?? 나중에 수정해야함
	{
		if (_sender->getServer()->getChannel(this->_params[0])->getTopic() == "")
		{
			std::string message = RPL_NOTOPIC(_sender->getServer()->getName(), _sender->getNick(), this->_params[0]);
			_sender->getServer()->getChannel(this->_params[0])->sendReply(message);
			return ;
		}
		else
		{
			std::string message = RPL_TOPIC(_sender->getServer()->getName(), _sender->getNick(), this->_params[0], _sender->getServer()->getChannel(this->_params[0])->getTopic());
			_sender->getServer()->getChannel(this->_params[0])->sendReply(message);
			return ;
		}
	}
	// topic지정한 시간(333 RPL_TOPICWHOTIME)도 설정해야함, join할때 현재의 topic을 332로, topic을 설정했던 시간과 설정한nick을 333으로 넘겨줘야 함
}
