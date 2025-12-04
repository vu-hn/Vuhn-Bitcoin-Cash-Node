// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <node/blockstorage.h>
#include <util/defer.h>
#include <validation.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(blockstorage_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(readblock_hash_mismatch) {
    LOCK(cs_main);
    CBlockIndex *pindex = ::ChainActive().Tip();
    BOOST_REQUIRE(pindex != nullptr);
    const BlockHash badHash{g_insecure_rand_ctx.rand256()};
    const BlockHash *const origBlockHash = pindex->phashBlock;
    BOOST_REQUIRE(origBlockHash && *origBlockHash != badHash);
    Defer d([&]{ pindex->phashBlock = origBlockHash; }); // restore pindex's proper hash pointer on scope end
    const auto consensusParams = ::Params().GetConsensus();

    CBlock block;
    // First, ensure it works as intended when hashes match
    BOOST_CHECK(ReadBlockFromDisk(block, pindex, consensusParams));
    BOOST_CHECK(ReadBlockFromDisk(block, pindex->GetBlockPos(), consensusParams, *pindex->phashBlock));
    // Next, ensure it fails when hashes do not match expected
    BOOST_CHECK(!ReadBlockFromDisk(block, pindex->GetBlockPos(), consensusParams, badHash));
    pindex->phashBlock = &badHash; // Fudge block hash to be different (will be restored on scope end)
    BOOST_CHECK(!ReadBlockFromDisk(block, pindex, consensusParams));
}

BOOST_AUTO_TEST_SUITE_END()
