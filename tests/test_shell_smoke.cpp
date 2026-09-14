#include "myshell/parser.hpp"
#include "myshell/executor.hpp"
#include "myshell/jobs.hpp"
#include "myshell/types.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main()
{
    namespace fs = std::filesystem;

    const fs::path out = fs::path("redirection_output.txt");
    const fs::path input = fs::path("redirection_input.txt");
    const fs::path pipeline_out = fs::path("redirection_pipeline_output.txt");

    try
    {
        fs::remove(out);
        fs::remove(input);
        fs::remove(pipeline_out);

        std::ofstream input_writer(input);
        input_writer << "first\n";
        input_writer.close();

        myshell::Parser parser;
        myshell::Executor executor;

        auto output_pipeline = parser.parse("echo hello > redirection_output.txt");
        if (executor.execute(output_pipeline) != 0)
        {
            std::cerr << "output redirection failed\n";
            return 1;
        }

        std::ifstream output_reader(out);
        std::string first_line;
        std::getline(output_reader, first_line);
        if (first_line != "hello")
        {
            std::cerr << "output redirection mismatch\n";
            return 1;
        }

        auto append_pipeline = parser.parse("echo second >> redirection_output.txt");
        if (executor.execute(append_pipeline) != 0)
        {
            std::cerr << "append redirection failed\n";
            return 1;
        }

        auto input_pipeline = parser.parse("cat < redirection_input.txt");
        if (executor.execute(input_pipeline) != 0)
        {
            std::cerr << "input redirection failed\n";
            return 1;
        }

        auto pipe_pipeline = parser.parse("echo hello | cat > redirection_pipeline_output.txt");
        if (executor.execute(pipe_pipeline) != 0)
        {
            std::cerr << "pipeline execution failed\n";
            return 1;
        }

        std::ifstream pipeline_reader(pipeline_out);
        std::string pipeline_line;
        std::getline(pipeline_reader, pipeline_line);
        if (pipeline_line != "hello")
        {
            std::cerr << "pipeline output mismatch\n";
            return 1;
        }

        auto env_pipeline = parser.parse("echo $HOME");
        if (env_pipeline.commands.size() != 1 ||
            env_pipeline.commands[0].program != "echo" ||
            env_pipeline.commands[0].arguments.empty() ||
            env_pipeline.commands[0].arguments[0] != std::getenv("HOME"))
        {
            std::cerr << "environment expansion failed\n";
            return 1;
        }

        auto quoted_pipeline = parser.parse("echo \"hello world\"");
        if (quoted_pipeline.commands.size() != 1 ||
            quoted_pipeline.commands[0].program != "echo" ||
            quoted_pipeline.commands[0].arguments.size() != 1 ||
            quoted_pipeline.commands[0].arguments[0] != "hello world")
        {
            std::cerr << "quoted tokenization failed\n";
            return 1;
        }

        auto escaped_pipeline = parser.parse("echo hello\\ world");
        if (escaped_pipeline.commands.size() != 1 ||
            escaped_pipeline.commands[0].program != "echo" ||
            escaped_pipeline.commands[0].arguments.size() != 1 ||
            escaped_pipeline.commands[0].arguments[0] != "hello world")
        {
            std::cerr << "escaped tokenization failed\n";
            return 1;
        }

        const auto before_jobs = myshell::JobManager::instance().jobs().size();
        auto background_pipeline = parser.parse("sleep 0.05 &");
        if (executor.execute(background_pipeline) != 0)
        {
            std::cerr << "background command failed\n";
            return 1;
        }

        const auto after_jobs = myshell::JobManager::instance().jobs().size();
        if (after_jobs <= before_jobs)
        {
            std::cerr << "background job was not registered\n";
            return 1;
        }

        fs::remove(out);
        fs::remove(input);
        fs::remove(pipeline_out);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    return 0;
}
