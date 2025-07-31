// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data.h>

#include <chainparams.h>
#include <consensus/activation.h>
#include <config.h>
#include <streams.h>
#include <validation.h>

#include <cassert>

static void BenchFindTransactionInBlock(benchmark::State &state, const size_t testTxPos, const bool useCTOR) {
    // We keep the block around as a static to avoid having to redo the costly setup each time this function is called
    static const CBlock block = []{
        // We need a block that is using CTOR
        const auto &data = benchmark::data::Get_block877227();
        constexpr int32_t blockHeight = 877227; // must match above

        SelectParams(CBaseChainParams::MAIN);
        // Ensure sanity of IsMagneticAnomalyEnabled()
        assert(IsMagneticAnomalyEnabled(GetConfig().GetChainParams().GetConsensus(), blockHeight - 1));

        // Read the block from benchmark data
        CBlock ret;
        VectorReader(SER_NETWORK, PROTOCOL_VERSION, data, 0) >> ret;

        // Ensure it's sorted
        for (size_t i = 2; i < ret.vtx.size(); i++) {
            assert(ret.vtx[i]->GetId() > ret.vtx[i-1]->GetId());
        }

        return ret;
    }();

    // Verify that it is really using CTOR and has enough transactions for our tests
    assert(block.vtx.size() > 1000 && testTxPos < block.vtx.size());

    // Look up txid by index, then verify looking up index by txid
    const TxId txid = block.vtx[testTxPos]->GetId();
    std::optional<size_t> txPos = internal::FindTransactionInBlock(block, txid, useCTOR);
    assert(txPos.has_value() && testTxPos == *txPos);

    // Benchmark looking up by txid
    BENCHMARK_LOOP {
        std::optional<size_t> res;
        benchmark::NoOptimize(res = internal::FindTransactionInBlock(block, txid, useCTOR));
        assert(res && *res == testTxPos);
    }
}

static void FindTransactionInBlock_0000_yCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 0, true);
}
static void FindTransactionInBlock_0000_nCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 0, false);
}
static void FindTransactionInBlock_0001_yCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 1, true);
}
static void FindTransactionInBlock_0001_nCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 1, false);
}
static void FindTransactionInBlock_0005_yCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 5, true);
}
static void FindTransactionInBlock_0005_nCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 5, false);
}
static void FindTransactionInBlock_0015_yCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 15, true);
}
static void FindTransactionInBlock_0015_nCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 15, false);
}
static void FindTransactionInBlock_0100_yCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 100, true);
}
static void FindTransactionInBlock_0100_nCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 100, false);
}
static void FindTransactionInBlock_1000_yCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 1000, true);
}
static void FindTransactionInBlock_1000_nCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 1000, false);
}
static void FindTransactionInBlock_7000_yCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 7000, true);
}
static void FindTransactionInBlock_7000_nCTOR(benchmark::State &state) {
    BenchFindTransactionInBlock(state, 7000, false);
}

BENCHMARK(FindTransactionInBlock_0000_yCTOR, 100000);
BENCHMARK(FindTransactionInBlock_0000_nCTOR, 100000);
BENCHMARK(FindTransactionInBlock_0001_yCTOR, 100000);
BENCHMARK(FindTransactionInBlock_0001_nCTOR, 100000);
BENCHMARK(FindTransactionInBlock_0005_yCTOR, 100000);
BENCHMARK(FindTransactionInBlock_0005_nCTOR, 100000);
BENCHMARK(FindTransactionInBlock_0015_yCTOR, 100000);
BENCHMARK(FindTransactionInBlock_0015_nCTOR, 100000);
BENCHMARK(FindTransactionInBlock_0100_yCTOR, 100000);
BENCHMARK(FindTransactionInBlock_0100_nCTOR, 100000);
BENCHMARK(FindTransactionInBlock_1000_yCTOR, 100000);
BENCHMARK(FindTransactionInBlock_1000_nCTOR, 100000);
BENCHMARK(FindTransactionInBlock_7000_yCTOR, 10000);
BENCHMARK(FindTransactionInBlock_7000_nCTOR, 10000);
