#include "myshell/parser.hpp"
#include "myshell/executor.hpp"
#include "myshell/types.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>

int main()
{
    namespace fs = std::filesystem;
    using namespace std::chrono;

    const fs::path out = fs::path("redirection_output.txt");
    const fs::path input = fs::path("redirection_input.txt");

    try
    {
        fs::remove(out);
        fs::remove(input);

        std::ofstream input_writer(input);
        input_writer << "first\n";
        input_writer.close();

        myshell::Parser parser;
        myshell::Executor executor;

        auto pipeline_output = parser.parse("echo hello > redirection_output.txt");
        if (executor.execute(pipeline_output) != 0)
        {
            std::cerr << "output redirection test failed\n";
            return 1;
        }

        std::ifstream output_reader(out);
        std::string first_line;
        std::getline(output_reader, first_line);
        if (first_line != "hello")
        {
            std::cerr << "output redirection content mismatch: '" << first_line << "'\n";
            return 1;
        }

        auto pipeline_append = parser.parse("echo second >> redirection_output.txt");
        if (executor.execute(pipeline_append) != 0)
        {
            std::cerr << "append redirection test failed\n";
            return 1;
        }

        std::ifstream output_reader_after_append(out);
        std::string all_output;
        std::string second_line;
        std::getline(output_reader_after_append, first_line);
        std::getline(output_reader_after_append, second_line);
        if (second_line != "second")
        {
            std::cerr << "append redirection behavior mismatch: '" << second_line << "'\n";
            return 1;
        }

        auto pipeline_input = parser.parse("cat < redirection_input.txt");
        if (executor.execute(pipeline_input) != 0)
        {
            std::cerr << "input redirection test failed\n";
            return 1;
        }

        const fs::path pipeline_out = fs::path("redirection_pipeline_output.txt");
        fs::remove(pipeline_out);

        auto pipeline_chain = parser.parse("echo hello | cat > redirection_pipeline_output.txt");
        if (executor.execute(pipeline_chain) != 0)
        {
            std::cerr << "pipeline execution test failed\n";
            return 1;
        }

        std::ifstream pipeline_reader(pipeline_out);
        std::string pipeline_line;
        std::getline(pipeline_reader, pipeline_line);
        if (pipeline_line != "hello")
        {
            std::cerr << "pipeline output content mismatch: '" << pipeline_line << "'\n";
            return 1;
        }

        const auto background_start = steady_clock::now();
        auto background_pipeline = parser.parse("sleep 0.2 &");
        if (executor.execute(background_pipeline) != 0)
        {
            std::cerr << "background execution test failed\n";
            return 1;
        }

        const auto duration_ms = duration_cast<milliseconds>(steady_clock::now() - background_start);
        if (duration_ms.count() > 200)
        {
            std::cerr << "background execution should return immediately\n";
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
