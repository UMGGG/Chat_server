#include "Server.hpp"

Server::Server(std::string& password)
: _name("ft_irc"), _password(password)
{
}

static void ft_bzero(void *s, size_t n)
{
    size_t	i;

    i = 0;
    while (i < n)
    {
        ((unsigned char *)s)[i] = 0;
        i++;
    }
}

void Server::startServer(int port)
{
    int listenSock, clientSock;
    struct sockaddr_in listenAddr, clientAddr;

    socklen_t clientAddrSize;
    int polled;

    listenSock = socket(PF_INET, SOCK_STREAM, 0);
    ft_bzero(&listenAddr, sizeof(listenAddr));
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    listenAddr.sin_port = htons(port);

    bind(listenSock, (struct sockaddr*)&listenAddr, sizeof(listenAddr));
    listen(listenSock, 512);

    struct pollfd listenPoll;
    listenPoll.fd = listenSock;
    listenPoll.events = POLLIN;
    _pollFD.push_back(listenPoll);

    while (1)
    {
        polled = poll(&_pollFD[0], _pollFD.size(), 5000);

        if (polled > 0)
        {
            if (_pollFD[0].revents & POLLIN)
            {
                clientAddrSize = sizeof(clientAddr);
                clientSock = accept(listenSock, (struct sockaddr *)&clientAddr, &clientAddrSize);
                fcntl(clientSock, F_SETFL, O_NONBLOCK);

                struct pollfd newPoll;
                newPoll.fd = clientSock;
                newPoll.events = POLLIN;
                _pollFD.push_back(newPoll);

                User *newUser = new User;

                newUser->setSocket(clientSock);
                newUser->setServer(this);
                _users.insert(std::pair<int, User*>(clientSock, newUser));

                newUser->setIP(inet_ntoa(clientAddr.sin_addr));
                std::cout << "connected (FD: " << clientSock << ", IPv4: " << newUser->getIP() << ")" << std::endl << std::endl;
                continue;
            }

            std::vector<struct pollfd>::iterator it = _pollFD.begin() + 1;
            while (it != _pollFD.end())
            {
                // 연결 종료
                if (it->revents & POLLHUP)
                {
                    disconnect(_users.at(it->fd));
                    continue;
                }

                // 소켓 읽기 및 command 실행
                if (it->revents & (POLLIN | POLLERR))
                {
                    if (_users.at(it->fd)->readMessage() <= 0)
                    //if (User::_state == DELETE)
                    {
                        disconnect(_users.at(it->fd));
                        continue;
                    }
                }
                ++it;
            }
        }
        std::map<int, User*>::iterator uit = _users.begin();
        while (uit != _users.end())
        {
            time_t checktime = time(NULL);
            if ((int)(checktime - uit->second->getPingtime()) > 120)
            {
                std::cout << "PING timeover (fd: " << uit->first << ")" << std::endl;
                //클라이언트 없에기
                disconnect(_users.at(uit->first));
                break;
            }
            uit++;
        }
    }
}

void Server::disconnect(User *user)
{
    std::cout << user->getNick() << "(FD: " << user->getSocket() << ")" << " has left the server." << std::endl << std::endl;
    std::cout << "FD: " << user->getSocket() << " shutdown."  << std::endl << std::endl;

    // leave joined channels;
    std::vector<Channel*> channels = user->getJoined();
    for (std::vector<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        (*it)->removeUser(user);
        if ((*it)->empty()) // terminate empty channel
        {
            delete *it;         // free channel object
            removeChannel(*it); // remove on Server::_channels
        }
    }

    close(user->getSocket());

    // remove on user list
    _users.erase(user->getSocket());

    // remove on pollfds
    for (std::vector<struct pollfd>::iterator it = _pollFD.begin(); it != _pollFD.end(); ++it)
    {
        if (it->fd == user->getSocket())
        {
            _pollFD.erase(it);
            break;
        }
    }

    // free
    delete user;
}

User* Server::getUser(std::string& nick)
{
    for (std::map<int, User*>::iterator it = _users.begin(); it != _users.end(); ++it)
    {
        if (!it->second->getNick().compare(nick))
            return it->second;
    }
    return NULL;
}

Channel* Server::getChannel(std::string channel)
{
    for (std::vector<Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        if (!(*it)->getName().compare(channel))
            return *it;
    }
    return NULL;
}

void Server::addChannel(Channel* channel)
{
    _channels.push_back(channel);
}

void Server::removeChannel(Channel* channel)
{
    for (std::vector<Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        if (*it == channel)
        {
            _channels.erase(it);
            return;
        }
    }
}

std::map<int, User*>& Server::getUsers()
{
    return _users;
}

std::vector<Channel*>& Server::getChannels()
{
    return _channels;
}

std::string Server::getPassword() const
{
    return _password;
}

std::string Server::getName() const
{
    return _name;
}
