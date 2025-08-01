// Copyright (c) 2023 The Bitcoin Core developers
// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data.h>

#include <chain.h>
#include <chainparams.h>
#include <clientversion.h>
#include <flatfile.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>

#include <cassert>

static CBlock GetTestBlock() {
    CBlock block;
    VectorReader(SER_NETWORK, CLIENT_VERSION, benchmark::data::Get_block413567(), 0) >> block;
    assert(!block.vtx.empty());
    return block;
}

static void WriteBlockBench(benchmark::State &state) {
    const CBlock block{GetTestBlock()};
    LOCK(cs_main); // needed by SaveBlockToDisk
    BENCHMARK_LOOP {
        const FlatFilePos pos = SaveBlockToDisk(block, 413'567, ::Params(), nullptr);
        assert(!pos.IsNull());
    }
}

static void ReadBlockBench(benchmark::State &state) {
    // Setup: ensure we have the block on disk
    const FlatFilePos pos = WITH_LOCK(cs_main, return SaveBlockToDisk(GetTestBlock(), 413'567, ::Params(), nullptr));
    assert(!pos.IsNull());
    BENCHMARK_LOOP {
        CBlock block;
        const bool success = ReadBlockFromDisk(block, pos, ::Params().GetConsensus());
        assert(success);
    }
}

static void ReadRawBlockBench(benchmark::State &state) {
    // Setup 1: ensure we have the block on disk
    const CBlock block = GetTestBlock();
    const FlatFilePos pos = WITH_LOCK(cs_main, return SaveBlockToDisk(block, 413'567, ::Params(), nullptr));
    assert(!pos.IsNull());
    const BlockHash hashBlock = block.GetHash();
    // Setup 2: make a fake index (needed by ReadRawBlockFromDisk())
    CBlockIndex fakeIndex;
    fakeIndex.phashBlock = &hashBlock;
    fakeIndex.nHeight = 413'567;
    fakeIndex.nFile = pos.nFile;
    fakeIndex.nDataPos = pos.nPos;
    fakeIndex.nStatus = BlockStatus{}.withData(true);
    BENCHMARK_LOOP {
        std::vector<uint8_t> rawBlock;
        const bool success = ReadRawBlockFromDisk(rawBlock, &fakeIndex, ::Params(), SER_DISK, CLIENT_VERSION);
        assert(success);
        assert(!rawBlock.empty());
    }
}

BENCHMARK(WriteBlockBench, 50);
BENCHMARK(ReadBlockBench, 50);
BENCHMARK(ReadRawBlockBench, 50);
