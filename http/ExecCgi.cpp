#include "ExecCgi.hpp"

ExecCgi::ExecCgi() {}

ExecCgi::~ExecCgi() {}

bool isDirectory(const std::string &filePath)
{
    if (filePath.substr(filePath.size() - 1) == "/")
        return (true);
    return (false);
}

void ExecCgi::executeCgiScript(Request &req, Response &res)
{
    std::string path;
    if (isDirectory(req.getUri()))
        path = "./autoindex/autoindex.py";
    else
       path = req.getUri();
    // std::cout << path << std::endl;
    if (!isScriptAccessible(path))
    {
        res.setStatus("404 Not Found");
        res.setBody("<html><body><h1>404 Not Found</h1><p>Requested script not found.</p></body></html>");
        return;
    }
    // CGI実行のための共通処理
    executeCommonCgiScript(req, res, path);
}

bool ExecCgi::isScriptAccessible(const std::string &path)
{
    struct stat buffer;

    // stat関数を使用してファイルの情報を取得
    if (stat(path.c_str(), &buffer) != 0)
        return false;

    // S_IXUSRは所有者の実行権限があるかをチェック
    if ((buffer.st_mode & S_IXUSR) == 0)
        return false;

    return true;
}

void ExecCgi::executeCommonCgiScript(Request &req, Response &res, const std::string &path)
{
    // パイプのセットアップ
    int pipefd[2];
    pipe(pipefd);

    // タイムアウト設定
    const double timeout = 2;
    clock_t start_time = Timer::startTimer();

    // CGIスクリプト実行のための子プロセスを作成
    pid_t pid = fork();
    if (pid < 0)
    {
        res.setStatus("500 Internal Server Error");
        res.setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to fork process.</p></body></html>");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    if (pid == 0)
    { // 子プロセス
        // 環境変数の設定
        std::vector<std::string> envVars = buildEnvVars(req);
        std::vector<char *> envpCGI = convertToEnvp(envVars);

        // 標準出力をパイプにリダイレクト
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // CGIスクリプトの実行
        const char *scriptName = path.c_str();
        // std::cout << scriptName << std::endl;
        if (path.substr(path.size() - 3) == ".py")
        {
            const char *pythonPath = "./venv/bin/python3";
            char *pythonArgv[] = {const_cast<char *>(pythonPath), const_cast<char *>(scriptName), NULL};
            execve(pythonPath, pythonArgv, envpCGI.data());
        }
        else
        {
            char *argv[] = {const_cast<char *>(scriptName), NULL};
            execve(path.c_str(), argv, envpCGI.data());
        }
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        close(pipefd[1]);
        int status;
        std::string output;
        char buf[1024];
        ssize_t len;

        bool timeoutOccurred = false;
        // パイプの読み取り側を非ブロッキングに設定
        fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

        pid_t   ret;
        while (true)
        {
            ret = waitpid(pid, &status, WNOHANG);
            if (Timer::calculateTime(start_time) > timeout)
            {
                timeoutOccurred = true;
                break;
            }
            if (ret == 0)
                continue ;
            if (WIFEXITED(status))
            {
                if (WEXITSTATUS(status) == 500)
                {
                    res.setStatus("500 Internal Server Error");
                    res.setBody("");
                    return;
                }
                break ;
            }
            break ;
        }
        // タイムアウトが発生した場合、子プロセスを終了させる
        if (timeoutOccurred)
        {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            res.setStatus("504 Gateway Timeout");
            res.setBody("<html><body><h1>504 Gateway Timeout</h1><p>CGI script took too long to respond.</p></body></html>");
            return;
        }

        while (true)
        {
            len = read(pipefd[0], buf, sizeof(buf) - 1);
            if (len > 0)
            {
                buf[len] = '\0';
                output += buf;
            }
            // EOFに到達した場合
            else if (len == 0)
                break;
            // エラーの場合（非ブロッキング読み取りの場合、データがない場合に-1を返す）
            else if (len < 0)
                continue;
            // タイムアウトチェック
            if (Timer::calculateTime(start_time) > timeout)
            {
                timeoutOccurred = true;
                break;
            }
        }
        close(pipefd[0]);
        // タイムアウトが発生した場合、子プロセスを終了させる
        if (timeoutOccurred)
        {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            res.setStatus("504 Gateway Timeout");
            res.setBody("<html><body><h1>504 Gateway Timeout</h1><p>CGI script took too long to respond.</p></body></html>");
            return;
        }

        res.setStatus("200 OK");
        res.setHeaders("Content-Type: ", "text/html");
        res.setBody(output);
        res.setHeaders("Content-Length: ", std::to_string(output.size()));
        return;
    }
}

std::vector<std::string> ExecCgi::buildEnvVars(Request &req)
{
    std::vector<std::string> envVars;
    // リクエストメソッドに基づいて環境変数を設定
    if (req.getMethod() == "GET")
    {
        // GETリクエストの場合の環境変数を設定
        envVars.push_back("REQUEST_METHOD=GET");
        // その他のGETに特有の環境変数設定
    }
    else if (req.getMethod() == "POST")
    {
        // POSTリクエストの場合の環境変数を設定
        envVars.push_back("REQUEST_METHOD=POST");
        // その他のPOSTに特有の環境変数設定
    }
    // 共通の環境変数設定
    // std::string erasePath = "autoindex.py";
    std::string autoindexPath = req.getUri();
    // autoindexPath = autoindexPath.erase(autoindexPath.size() - erasePath.size());
    autoindexPath = "AUTOINDEX=" + autoindexPath;
    // std::cout << autoindexPath << std::endl;  
    envVars.push_back(autoindexPath);
    return envVars;
}

std::vector<char *> ExecCgi::convertToEnvp(const std::vector<std::string> &envVars)
{
    std::vector<char *> envp;
    for (std::vector<std::string>::const_iterator it = envVars.begin(); it != envVars.end(); ++it)
        envp.push_back(const_cast<char *>(it->c_str()));
    envp.push_back(NULL); // 環境変数リストの終端
    return envp;
}
