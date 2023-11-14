#include "MainConfig.hpp"
MainConfig::MainConfig() {}

MainConfig::MainConfig(const std::string& filename) 
{
    file.open(filename);
    if (!file.is_open())
        throw std::runtime_error("[KO] file not found");
}

MainConfig::~MainConfig() {}

void MainConfig::parseLine(void)
{
    std::string line;
    while (std::getline(file, line))
    {
        size_t commentPos = line.find("#");
        if (commentPos != std::string::npos)
        {
            line = line.substr(0, commentPos);
        }
        if (line.empty())
            continue;
        lineToToken(line);
    }
}

void MainConfig::lineToToken(std::string& line)
{
    std::stringstream ss(line);
    std::string token;
    while (ss >> token)
        tokens.push_back(token);
}

void MainConfig::tokenSearchandSet() 
{
    std::vector<std::string>::iterator it = tokens.begin();
    while (it != tokens.end()) 
    {
        if (*it == "server") 
        {
            Servers server;
            ++it;
            if (it == tokens.end())
                throw std::runtime_error("Parse error: Unexpected end of file, expecting '{' for server block");
            if (*it != "{")
                throw std::runtime_error("Parse error: Expected '{' after server keyword");
            ++it;
            while (it != tokens.end() && *it != "}") 
            {
                inputServers(it, server);
                ++it;
            }
            if (it == tokens.end() && *it != "}")
                throw std::runtime_error("Parse error: server block not closed with '}'");
            servers.push_back(server);
        }
        else if (*it == "client_max_body_size")
        {
            it++;
            removeTrailingSemicolon(*it);
            setClientMaxBodySize(*it);
        }
        else
        {
            throw std::runtime_error("Parse error: Unexpected token in main block");
        }
        if (it != tokens.end()) {
            ++it;
        }
    }
}

bool MainConfig::checkPortNum(const size_t& port)
{
    for (std::vector<Servers>::iterator it = servers.begin(); it != servers.end(); ++it)
    {
        if (it->getPort() == port)
            return true;
    }
    return false;
}

size_t MainConfig::validatePort(const std::string& port)
{
     std::stringstream ss(port);
    size_t port_num;
    ss >> port_num;
    // std::cout << port_num << std::endl;
    if (port_num > 65535)
        throw std::runtime_error("port number is too big");
    else if (ss.fail() || (ss.peek() != EOF))
        throw std::runtime_error("port number is not a number");
    else if (checkPortNum(port_num))
        throw std::runtime_error("port number is not a number");
    return port_num;
}
void MainConfig::setClientMaxBodySize(const std::string& client_max_body_size)
{
    if (client_max_body_size.empty()) 
        throw std::runtime_error("Parse error: client_max_body_size is empty");
    if (this->client_max_body_size != 0) 
        throw std::runtime_error("Parse error: Duplicate client_max_body_size");
    size_t pos = 0;
    while (pos < client_max_body_size.size() && std::isdigit(client_max_body_size[pos]))
        pos++;
    std::string numberPart = client_max_body_size.substr(0, pos);
    std::string unitPart = client_max_body_size.substr(pos);
    if (!unitPart.empty() && unitPart.find_first_not_of("kKmMgG") != std::string::npos) 
        throw std::runtime_error("Parse error: Invalid unit for client_max_body_size");
    std::stringstream ss(numberPart);
    int value;
    ss >> value;
    if (ss.fail() || !ss.eof()) 
        throw std::runtime_error("Parse error: Invalid number format for client_max_body_size");
    if (unitPart.empty()) 
        this->client_max_body_size = value;
    else if (unitPart == "k" || unitPart == "K")
        this->client_max_body_size = value * 1024;
    else if (unitPart == "m" || unitPart == "M")
        this->client_max_body_size = value * 1024 * 1024;
    else if (unitPart == "g" || unitPart == "G")
        this->client_max_body_size = value * 1024 * 1024 * 1024;
    else
        throw std::runtime_error("Parse error: Invalid client_max_body_size");
}


void MainConfig::inputServers(std::vector<std::string>::iterator& it, Servers& server)
{
    if (*it == "listen")
    {
        it++;
        removeTrailingSemicolon(*it);
        size_t port = validatePort(*it);
        checkPortNum(port);
        server.setPort(port);
    }
    else if (*it == "server_name")
    {
        it++;
        removeTrailingSemicolon(*it);
        checkServerName(*it);
        server.setSeverNames(*it);
    }
    else if (*it == "index")
    {
        it++;
        while (it != tokens.end() && it->find(";") == std::string::npos)
        {
            checkFileExists(*it);
            checkFileAccess(*it);
            server.setIndex(*it);
            it++;
        }
        if (it == tokens.end())
            throw std::runtime_error("Parse error: Unexpected end of tokens before index block");
        checkFileExists(*it);
        checkFileAccess(*it);
        removeTrailingSemicolon(*it);
        server.setIndex(*it);
    }
    else if (*it == "location")
    {
        it++;
        std::vector<std::string>::iterator end = tokens.end();
        server.setLocations(it, end);
    }
    else if (*it == "client_max_body_size")
    {
        it++;
        removeTrailingSemicolon(*it);
        server.setClientMaxBodySize(*it);
    }
    else
        throw std::runtime_error("Parse error: Unexpected token in server block");
}

const size_t& MainConfig::getClientMaxBodySize(void) const
{
    return (this->client_max_body_size);
}


void MainConfig::checkServerName(const std::string& server_name)
{
    for (std::vector<Servers>::iterator it = servers.begin(); it != servers.end(); ++it)
    {
        const std::string names = it->getServerNames();
        if (names == server_name)
        {
            throw std::runtime_error("Parse error: Duplicate server name");
        }
    }
}