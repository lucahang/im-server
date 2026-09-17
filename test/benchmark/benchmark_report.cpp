#include <fstream>
#include <iostream>
#include <chrono>

struct BenchmarkReport {
    int connections;
    int messages;
    double qps;
    double avg_latency_ms;
    double p99_latency_ms;
};

void GenerateReport(const BenchmarkReport& report)
{
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    std::string filename =
        "benchmark_report_" + std::to_string(timestamp) + ".json";

    std::ofstream out(filename);

    out << "{\n";
    out << "  \"connections\": " << report.connections << ",\n";
    out << "  \"messages\": " << report.messages << ",\n";
    out << "  \"qps\": " << report.qps << ",\n";
    out << "  \"avg_latency_ms\": " << report.avg_latency_ms << ",\n";
    out << "  \"p99_latency_ms\": " << report.p99_latency_ms << "\n";
    out << "}\n";

    std::cout << "generated: " << filename << std::endl;
}

int main()
{
    BenchmarkReport report{
        1000,
        100000,
        20000,
        1.8,
        8.5
    };

    GenerateReport(report);
}
