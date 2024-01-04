#include "GetRequest.hpp"

GetRequest::GetRequest(){}

GetRequest::~GetRequest(){}

bool isCgiScript(const std::string& filePath) 
{
    if (filePath.size() >= 4 && filePath.substr(filePath.size() - 3) == ".sh")
        return (true);
    if (filePath.size() >= 4 && filePath.substr(filePath.size() - 3) == ".py")
        return (true);
    return (false);
}

std::string GetRequest::openFile(const std::string& filePath)
{
    std::cout << "In GetRequest::openFile   file psth = " << filePath << std::endl;    
    std::ifstream file(filePath);
    std::string content;
    std::string line;

    if (file.is_open()) 
    {
        content = "200 OK";
        file.close();
    } 
    else
    {
        content = "404 Not Found";
    }

    std::cout << "content = " << content << std::endl;
    return content;
}

std::string GetRequest::getBody(const std::string& status, const std::string& filePath)
{
    std::string body;
    if (status == "200 OK")
    {
        std::ifstream file(filePath);
        std::string line;
        while (getline(file, line))
        {
            body += line;
        }
        file.close();
    }
    else
    {
        body = "<html><body><h1>404 Not Found</h1></body></html>";
    }
    return body;
}

// void GetRequest::executeCgiScript(Request& req, Response& res)
// {
//     std::string path = req.getUri();
//     if (access(path.c_str(), F_OK | X_OK) != 0) 
//     {
//         res.setStatus("404 Not Found");
//         res.setBody("<html><body><h1>404 Not Found</h1><p>Requested script not found.</p></body></html>");
//         return;
//     }
//     int pipefd[2];
//     pipe(pipefd);
//     pid_t pid = fork();
//     if (pid == 0) //child
//     {
//         close(pipefd[0]);
//         dup2(pipefd[1], STDOUT_FILENO); 
//         dup2(pipefd[1], STDERR_FILENO);
//         close(pipefd[1]);
//         execve(path.c_str(), NULL, NULL);
//         exit(500);
//     }
//     else if (pid > 0) 
//     {
//         close(pipefd[1]);
//         int status;
//         std::string output;
//         char buf[1024];
//         ssize_t len;
//         while ((len = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
//             buf[len] = '\0';
//             output += buf;
//         }
//         close(pipefd[0]);
//         waitpid(pid, &status, 0);
//         if (WIFEXITED(status))
//         {
//             int exitStatus = WEXITSTATUS(status);
//             if (exitStatus == 500) 
//             {
//                 res.setStatus("500 Internal Server Error");
//                 res.setBody("");
//                 return;
//             }
//         }
//         res.setStatus("200 OK");
//         res.setHeaders("Content-Type: ", "text/html");
//         res.setBody(output);
//         res.setHeaders("Content-Length: ", std::to_string(output.size()));
//         return;
//     }
// }

void GetRequest::handleGetRequest(Request& req, Response& res)
{
    if (isCgiScript(req.getUri()))
    {
        std::cout << "In GetRequest::handleGetRequest" << std::endl;
       ExecCgi::executeCgiScript(req, res);
    }
    else
    {
        // if (req.getFilepath() != "")
        // {
        //     std::cout << "-------------handleget filepath != 0 --------------- \n";
        //     res.setStatus(openFile(req.getFilepath()));
        // }
        // else
            res.setStatus(openFile(req.getUri()));
        res.setHeaders("Content-Type: ", "text/html");
        res.setBody(getBody(res.getStatus(), req.getUri()));
        res.setHeaders("Content-Length: ", std::to_string(res.getBody().size()));
    }
    res.setResponse();
}
