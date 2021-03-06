#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string_view>
#include <set>

/*
    How to use copy_touched_files.exe:
    - Download ProcessMonitor.zip from e.g.: 
        https://docs.microsoft.com/en-us/sysinternals/downloads/procmon
    - extract the zip and execute Procmon.exe
    - Configure Path | begins with | C:\texlive | then | Include
    - Apply
    - OK
    - Execute the build of a document
    - File | Save
    - Events displayed using current filter
    - NOT profile events
    - Comma-Separated Values (CSV)
    - Supply a path "D:\procts\comp\procmon\Logfile.CSV"
    - Run copy touched files with arguements:
        C:/texlive/ 
        D:/procts/procmon/Touched_Files/
        D:/procts/procmon/Logfile.CSV
*/

int main(int argc, char* argv[])
{
    using paths_t = std::set<std::filesystem::path>;
    paths_t paths;
    if (argc < 4)
    {
        std::cerr << "Expected 3 parameters: source directory, target directory, logfile (CSV)" << std::endl;
        return -1;
    }
    std::string argv1(argv[1]);
    while ((argv1.back() == '/') || (argv1.back() == '\\'))
    {
        argv1.erase(argv1.size() - 1);
    }
    std::filesystem::path root_path(argv1);
    ptrdiff_t root_len = std::distance(root_path.begin(), root_path.end());
    std::string argv2(argv[2]);
    while ((argv2.back() == '/') || (argv2.back() == '\\'))
    {
        argv2.erase(argv2.size() - 1);
    }
    std::filesystem::path target_path(argv2);
    std::filesystem::path process_monitor_log_path(argv[3]);
    std::ifstream process_monitor_log(process_monitor_log_path);
    if (!process_monitor_log.is_open())
    {
        std::cerr << "Cannot open file '" << process_monitor_log_path << "'" << std::endl;
        return -1;
    }
    int row = 0;
    std::string line;
    while (std::getline(process_monitor_log, line))
    {
        std::istringstream line_stream(line);
        std::string value;
        int column = 0;
        while (std::getline(line_stream, value, ','))
        {
            if (column++ != 4)
            {
                continue; // found value is not from the path column
            }
            if (row == 0)
            {
                if (value != "\"Path\"")
                {
                    std::cerr << "Expected '\"Path\"' as fifth parameter in first line" << std::endl;
                    return -1;
                }
                continue;
            }
            size_t len = value.length();
            if (len <= 2)
            {
                continue; // found path is empty string
            }
            std::filesystem::path path(value.substr(1, len - 2));
            ptrdiff_t path_len = std::distance(path.begin(), path.end()) - 1;
            if (root_len > path_len)
            {
                continue; // found path does not have root_path
            }
            if (!std::equal(root_path.begin(), root_path.end(), path.begin()))
            {
                continue; // found path does not have root_path
            }
			try
			{
				if (!std::filesystem::is_regular_file(path))
				{
					continue; // found path is not a regular file
				}
			}
			catch (...)
			{
				continue; // std::filesystem::is_regular_file(path) can't deal with regular expression??
			}
			for (std::filesystem::directory_entry const& p : std::filesystem::directory_iterator(std::filesystem::path(path).remove_filename()))
            {
                if (std::filesystem::equivalent(p, path))
                {
                    path = p;
                    break;
                }
            }
            paths.insert(path);
        }
        ++row;
    }
    for (std::filesystem::path const& path_from : paths)
    {
        std::filesystem::path::const_iterator it = path_from.begin();
        for (int i = 0; i < root_len; ++i)
        {
            ++it; // skip root
        }
        std::filesystem::path path_to = target_path;
        while (it != path_from.end())
        {
            path_to /= *it++;
        }
        std::filesystem::create_directories(std::filesystem::path(path_to).remove_filename());
		try
		{
			std::filesystem::copy(path_from, path_to);
		}
		catch (...)
		{
			std::cout << "Failed to copy from '" << path_from << "' to '" << path_to << "'" << std::endl;
		}
    }
    std::cout << "Ready" << std::endl;
    return 0;
}

