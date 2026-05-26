#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

namespace {

std::uint64_t run_workload(const std::vector<int>& values)
{
    std::uint64_t checksum = 0;
    for (int value : values) {
        const auto x = static_cast<std::uint64_t>(value);
        checksum += (x * x + 3U * x + 7U) % 1009U;
    }
    return checksum;
}

} // namespace

int main()
{
    std::vector<int> values(4096);
    std::iota(values.begin(), values.end(), 1);
    if (values.empty()) {
        std::cerr << "performance dataset must not be empty\n";
        return 1;
    }

    const std::uint64_t warmup_checksum = run_workload(values);
    if (warmup_checksum == 0U) {
        std::cerr << "warmup checksum must be non-zero\n";
        return 1;
    }

    std::vector<std::chrono::nanoseconds> samples;
    samples.reserve(9);
    std::uint64_t checksum = 0;
    for (int repetition = 0; repetition < 9; ++repetition) {
        const auto start = std::chrono::steady_clock::now();
        checksum ^= run_workload(values);
        const auto finish = std::chrono::steady_clock::now();
        samples.push_back(finish - start);
    }

    if (samples.empty() || checksum == 0U) {
        std::cerr << "performance repetitions must produce samples and checksum\n";
        return 1;
    }

    std::sort(samples.begin(), samples.end());
    const auto min_sample = samples.front();
    const auto median_sample = samples[samples.size() / 2U];
    const auto max_sample = samples.back();
    if (median_sample < min_sample || max_sample < median_sample) {
        std::cerr << "performance sample ordering is invalid\n";
        return 1;
    }

    std::cout << "P1.07 performance smoke samples=" << samples.size()
              << " checksum=" << checksum
              << " min_ns=" << min_sample.count()
              << " median_ns=" << median_sample.count()
              << " max_ns=" << max_sample.count() << '\n';
    return 0;
}
