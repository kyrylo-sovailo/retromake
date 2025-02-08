#include "../include/util.h"

#include <dirent.h>
#include <limits.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cctype>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

bool rm::begins_with(const std::string &string, const char *pattern) noexcept
{
    const size_t pattern_size = std::strlen(pattern);
    if (string.size() < pattern_size) return false;
    else return std::memcmp(string.c_str(), pattern, pattern_size) == 0;
}

bool rm::begins_with(const char *string, const char *pattern) noexcept
{
    const size_t string_size = std::strlen(string);
    const size_t pattern_size = std::strlen(pattern);
    if (string_size < pattern_size) return false;
    else return std::memcmp(string, pattern, pattern_size) == 0;
}

bool rm::ends_with(const std::string &string, const char *pattern) noexcept
{
    const size_t pattern_size = std::strlen(pattern);
    if (string.size() < pattern_size) return false;
    else return std::memcmp(string.c_str() + string.size() - pattern_size, pattern, pattern_size) == 0;
}

bool rm::ends_with(const char *string, const char *pattern) noexcept
{
    const size_t string_size = std::strlen(string);
    const size_t pattern_size = std::strlen(pattern);
    if (string_size < pattern_size) return false;
    else return std::memcmp(string + string_size - pattern_size, pattern, pattern_size) == 0;
}

std::string rm::trim(const std::string &string)
{
    std::string debug = string;
    size_t begin = 0;
    while (begin < string.size() && (string[begin] == ' ' || string[begin] == '\t' || string[begin] == '\r' || string[begin] == '\n')) begin++;
    size_t end = string.size();
    while (end > begin && (string[end-1] == ' ' || string[end-1] == '\t' || string[end-1] == '\r' || string[end-1] == '\n')) end--;
    return string.substr(begin, end - begin);
}

std::string rm::lower(const std::string &string)
{
    std::string lower(string.size(), '\0');
    for (size_t i = 0; i < string.size(); i++) lower[i] = std::tolower(string[i]);
    return lower;
}

std::vector<std::string> rm::parse(const std::string &string, char delimiter)
{
    std::vector<std::string> result;
    bool request_new = true;
    for (auto ci = string.cbegin(); ci != string.cend(); ci++)
    {
        const char c = *ci;
        if (c == delimiter || (delimiter == '\0' && !std::isalnum(c)))
        {
            request_new = true;
        }
        else
        {
            if (request_new) { result.push_back(std::string(1, c)); request_new = false; }
            else result.back().push_back(c);
        }
    }
    return result;
}

std::map<std::string, std::string> rm::parse_config(const std::string &path)
{
    enum class State
    {
        wait_name_begin,
        wait_name_end,
        wait_equal,
        wait_value_begin,
        wait_value_end,
        wait_end_line
    };
    std::map<std::string, std::string> result;
    std::string name, value;
    State state = State::wait_name_begin;

    std::ifstream file(path, std::ios::binary);
    while (true)
    {
        char c;
        const bool good = static_cast<bool>(file.read(&c, 1));
        switch (state)
        {
        case State::wait_name_begin:
            if (!good || c == '\0') return result;
            else if (c == '#') state = State::wait_end_line;
            else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {}
            else if (c == '_' || std::isalpha(c)) { name.push_back(c); state = State::wait_name_end; }
            else throw std::runtime_error("Unexpected symbol in entry name in " + path);
            break;
        case State::wait_name_end:
            if (!good || c == '\0' || c == '\n' || c == '\r') throw std::runtime_error("Unexpected end of entry name in " + path);
            else if (c == ' ' || c == '\t') state = State::wait_equal;
            else if (c == '_' || std::isalnum(c)) name.push_back(c);
            else if (c == '=') state = State::wait_value_begin;
            else throw std::runtime_error("Unexpected symbol in entry name in " + path);
            break;
        case State::wait_equal:
            if (!good || c == '\0' || c == '\n' || c == '\r') throw std::runtime_error("Unexpected end of entry name in " + path);
            else if (c == ' ' || c == '\t') {}
            else if (c == '=') state = State::wait_value_begin;
            else throw std::runtime_error("Unexpected symbol in entry name in " + path);
            break;
        case State::wait_value_begin:
            if (!good || c == '\0' || c == '\n' || c == '\r') throw std::runtime_error("Unexpected end of entry value in " + path);
            else if (c == ' ' || c == '\t') {}
            else { value.push_back(c); state = State::wait_value_end; }
            break;
        case State::wait_value_end:
            if (!good || c == '\0' || c == '\n' || c == '\r')
            {
                result[name] = value;
                if (!good || c == '\0') return result;
                name.resize(0);
                value.resize(0);
                state = State::wait_name_begin;
            }
            else value.push_back(c);
            break;
        case State::wait_end_line:
            if (!good || c == '\0') return result;
            else if (c == '\n' || c == '\r') state = State::wait_name_begin;
            break;
        }
    }
}

struct SearchImplementation
{
    DIR *directory;
    struct dirent *entry;
};

rm::Search::Search(const std::string &directory)
{
    static_assert(sizeof(SearchImplementation) <= sizeof(_data));
    std::memset(_data, 0, sizeof(_data));
    SearchImplementation *data = reinterpret_cast<SearchImplementation*>(_data);
    data->directory = opendir(directory.c_str());
    if (data->directory == nullptr) throw std::runtime_error("Failed to open directory " + directory);
}

const char *rm::Search::get(bool *directory)
{
    static_assert(sizeof(SearchImplementation) <= sizeof(_data));
    SearchImplementation *data = reinterpret_cast<SearchImplementation*>(_data);
    while (true)
    {
        data->entry = readdir(data->directory);
        if (data->entry == nullptr) return nullptr;
        if (data->entry->d_type != DT_DIR && data->entry->d_type != DT_REG) continue;
        if (directory != nullptr) *directory = data->entry->d_type == DT_DIR;
        return data->entry->d_name;
    }
}

rm::Search::~Search()
{
    static_assert(sizeof(SearchImplementation) <= sizeof(_data));
    SearchImplementation *data = reinterpret_cast<SearchImplementation*>(_data);
    if (data->directory != nullptr) closedir(data->directory);
}

std::string rm::path_executable()
{
    std::string executable(256, '\0');
    while (true)
    {
        int read = readlink("/proc/self/exe", &executable[0], executable.size());
        if (read < 0) throw std::runtime_error("readlink(\"/proc/self/exe\") failed");
        else if (read == executable.size()) executable.resize(executable.size() * 2);
        else { executable.resize(read); break; }
    }
    return executable;
}

std::string rm::path_working_directory()
{
    std::string directory;
    directory.resize(256);
    while (true)
    {
        char *cwd = getcwd(&directory[0], directory.size());
        if (cwd == nullptr) directory.resize(directory.size() * 2);
        else { directory.resize(std::strlen(directory.c_str())); break; }
    }
    if (directory.back() != '/') directory.push_back('/');
    return directory;
}

std::string rm::path_user_directory()
{
    std::string directory;
    struct passwd *user = getpwuid(getuid());
    if (user == nullptr) throw std::runtime_error("getpwuid() failed");
    directory = user->pw_dir;
    if (directory.back() != '/') directory.push_back('/');
    return directory;
}

std::string rm::path_relative(const std::string &target, const std::string &base, size_t *depth)
{
    size_t i = 0;
    while (i < base.size() && i < target.size() && base[i] == target[i]) i++;
    std::string relative;
    size_t _depth = 0;
    for (size_t i2 = i; i2 < base.size(); i2++)
    {
        if (base[i2] == '/') { relative += "../"; _depth++; }
    }
    if (depth != nullptr) *depth = _depth;
    relative.append(target.cbegin() + i, target.cend());
    return relative;
}

std::string rm::path_base(const std::string &path)
{
    if (path.empty()) throw std::runtime_error("Could not find base name of " + path);
    auto slash = path.rfind('/', path.size() - 1);
    if (slash == std::string::npos) slash = 0;
    else slash++;
    auto dot = path.find('.', slash);
    if (dot == std::string::npos) dot = path.size();
    return path.substr(slash, dot - slash);
}

void rm::path_parent(std::string *path)
{
    if (path->empty()) throw std::runtime_error("Could not step back from " + *path);
    auto slash = path->rfind('/', path->size() - 1);
    if (slash == std::string::npos) throw std::runtime_error("Could not step back from " + *path);
    path->resize(slash + 1);
}

void rm::path_append(std::string *path, const std::string &path2, bool is_directory)
{
    if (path2.empty()) {}
    else if (path2.front() == '/') *path = path2; //Unix Specific
    else
    {
        size_t i = 0;
        while (i < path2.size())
        {
            auto slash = path2.find('/', i + 1);
            if (slash == std::string::npos) slash = path2.size();
            if (i + 1 == slash) {}
            else if (i + 2 == slash && path2[i+1] == '.') {}
            else if (i + 3 == slash && path2[i+1] == '.' && path2[i+2] == '.') path_parent(path);
            else path->insert(path->end(), path2.cbegin() + i, path2.cbegin() + slash);
            i = slash;
        }
    }
    if (is_directory && path->back() != '/') path->push_back('/');
    else if (!is_directory && path->back() == '/') path->pop_back();
}

bool rm::path_exists(const std::string &path, bool is_directory, std::string *absolute_path)
{
    struct stat status;
    if (stat(path.c_str(), &status) != 0) return false;
    if (is_directory && !S_ISDIR(status.st_mode)) return false;
    else if (!is_directory && !S_ISREG(status.st_mode)) return false;
    if (absolute_path != nullptr)
    {
        char buffer[PATH_MAX+1];
        char *absolute_path_p = ::realpath(path.c_str(), buffer);
        if (absolute_path_p == nullptr) return false;
        *absolute_path = absolute_path_p;
        if (is_directory && absolute_path->back() != '/') absolute_path->push_back('/');
        else if (!is_directory && absolute_path->back() == '/') absolute_path->pop_back();
    }
    return true;
}

void rm::path_ensure(const std::string &path, const bool is_directory, const size_t depth)
{
    std::string path2 = path; char *path2c = &path2[0];
    size_t depth2 = depth;
    size_t slash = path2.size();
    while (depth2 > 0)
    {
        struct stat status;
        if (stat(path2c, &status) == 0)
        {
            const bool should_be_directory = is_directory || depth != depth2;
            if (should_be_directory && !S_ISDIR(status.st_mode)) throw std::runtime_error("Failed to create directory " + std::string(path2c));
            else if (!should_be_directory && !S_ISREG(status.st_mode)) throw std::runtime_error("Failed to create file " + std::string(path2c));
            break;
        }
        depth2--;
        while (path2c[slash] != '/')
        {
            if (slash == 0) throw std::runtime_error("Could not step back from " + std::string(path2c));
            slash--;
        }
        path2c[slash] = '\0';
    }
    while (depth2 < depth)
    {
        depth2++;
        slash += std::strlen(path2c + slash);
        path2c[slash] = '/';
        const bool should_be_directory = is_directory || depth != depth2;
        if (should_be_directory && mkdir(path2c, 0700) != 0) throw std::runtime_error("Failed to create directory " + std::string(path2c));
    }
}

int rm::call_wait(std::vector<std::string> &arguments, std::map<std::string, std::string> *environment)
{
    const pid_t pid = fork();
    if (pid < 0) throw std::runtime_error("fork() failed");
    else if (pid == 0)
    {
        //I am child
        call_no_return(arguments, environment);
        return 1;
    }
    else
    {
        //I am parent
        int status;
        if (wait(&status) < 0) throw std::runtime_error("wait() failed");
        if (!WIFEXITED(status)) throw std::runtime_error("wait() did not wait till child termination");
        return WEXITSTATUS(status);
    }
}

void rm::call_no_return(std::vector<std::string> &arguments, std::map<std::string, std::string> *environment)
{
    std::vector<char*> argv;
    for (auto argument = arguments.begin(); argument != arguments.end(); argument++)
        argv.push_back(&(*argument)[0]);
    argv.push_back(nullptr);

    if (environment == nullptr)
    {
        execvp(argv[0], argv.data());
        throw std::runtime_error("execvp() failed");
    }
    else
    {
        std::vector<std::string> envp_mem; envp_mem.reserve(environment->size() + 1);
        std::vector<char*> envp; envp.reserve(environment->size());
        for (auto entry = environment->cbegin(); entry != environment->cend(); entry++)
        {
            envp_mem.push_back(entry->first + "=" + entry->second);
            envp.push_back(&envp_mem.back()[0]);
        }
        envp.push_back(nullptr);

        execvpe(argv[0], argv.data(), envp.data());
        throw std::runtime_error("execvpe() failed");
    }
}

void rm::remove_cmake_define(std::vector<std::string> *arguments, const char *pattern)
{
    for (auto argument = arguments->begin(); argument != arguments->end(); /*argument++*/)
    {
        if (*argument == "-D")
        {
            auto current = argument;
            current++;
            if (current != arguments->end() && begins_with(*current, pattern))
            {
                argument = arguments->erase(argument);
                argument = arguments->erase(argument);
            }
            else
            {
                argument++;
                argument++;
            }
        }
        else if (begins_with(*argument, "-D") && begins_with(argument->c_str() + 2, pattern))
        {
            argument = arguments->erase(argument);
        }
        else argument++;
    }
}