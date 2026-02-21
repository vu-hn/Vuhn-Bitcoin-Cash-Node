# validation.cpp — Code Topology Map & Unused Code Audit

> **File:** `src/validation.cpp`
> **Codebase:** Vuhn Bitcoin Cash Node (VBCHN), forked from BCHN v29.0.0
> **Date:** 2026-02-21
> **Purpose:** Pre-refactoring analysis — no code changes

---

## Table of Contents

1. [Structural Overview Diagram](#1-structural-overview-diagram)
2. [File Overview & Statistics](#2-file-overview--statistics)
3. [Classes Defined in validation.cpp](#3-classes-defined-in-validationcpp)
4. [Complete Function Catalog](#4-complete-function-catalog)
   - 4.1 [CChainState Methods (26)](#41-cchainstate-methods-26)
   - 4.2 [Static (File-Local) Functions (21)](#42-static-file-local-functions-21)
   - 4.3 [Exported Standalone Functions (61)](#43-exported-standalone-functions-61)
   - 4.4 [Oversized Functions (12+)](#44-oversized-functions-12)
5. [Global & Static Variables](#5-global--static-variables)
6. [Proposed Refactoring Domains](#6-proposed-refactoring-domains)
7. [Dependency & Coupling Analysis](#7-dependency--coupling-analysis)
8. [Unused Code Audit](#8-unused-code-audit)
9. [Test Coverage Map](#9-test-coverage-map)
   - 9.1 [Unit Tests (src/test/)](#91-unit-tests-srctest)
   - 9.2 [Functional Tests (test/functional/)](#92-functional-tests-testfunctional)
   - 9.3 [Coverage by Refactoring Domain](#93-coverage-by-refactoring-domain)
   - 9.4 [Coverage Gaps](#94-coverage-gaps)

---

## 1. Structural Overview Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        validation.cpp  (5,987 lines)                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  INCLUDES & FORWARD DECLS .............. lines 1-73     (73 lines)      │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  50 project headers, 11 C++ stdlib headers                       │   │
│  │  Forward decl: ConnectTrace, FlushStateToDisk,                   │   │
│  │    FindFilesToPruneManual, FindFilesToPrune, GetNextBlockScript..│   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  CLASS: CChainState .................... lines 89-243   (155 lines)     │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  Central blockchain state machine. Single instance: g_chainstate │   │
│  │  18 public methods, 8 private methods, 5 private fields          │   │
│  │  Manages: block index candidates, chain tip, failed blocks       │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  GLOBAL STATE .......................... lines 245-331  (87 lines)      │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  cs_main, mapBlockIndex, g_mempool, pcoinsTip, pblocktree, etc.  │   │
│  │  ~23 extern-visible globals + anonymous namespace references     │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  MEMPOOL ADMISSION ..................... lines 345-896  (552 lines)     │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  TestLockPointValidity, CheckSequenceLocks, FormatStateMessage   │   │
│  │  CheckInputsFromMempoolAndCache (static)                         │   │
│  │  AcceptToMemoryPoolWorker (static, 376 lines)                    │   │
│  │  AcceptToMemoryPoolWithTime, AcceptToMemoryPool                  │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  TRANSACTION LOOKUP .................... lines 898-1014 (117 lines)     │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  FindTransactionInBlock (2 overloads), GetTransaction            │   │
│  │  GetBlockSubsidy, IsInitialBlockDownload                         │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  FORK WARNING & INVALID CHAIN .......... lines 1045-1188 (144 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  AlertNotify, CheckForkWarningConditions,                        │   │
│  │  CheckForkWarningConditionsOnNewFork, InvalidChainFound,         │   │
│  │  CChainState::InvalidBlockFound                                  │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  UTXO / COINS OPERATIONS ............... lines 1190-1398 (209 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  SpendCoins, UpdateCoins (×2), CScriptCheck::operator()          │   │
│  │  GetSpendHeight, CheckInputs (112 lines), UndoCoinSpend          │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  BLOCK DISCONNECT & UNDO ............... lines 1400-1489 (90 lines)     │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  CChainState::DisconnectBlock, ApplyBlockUndo                    │   │
│  │  Script check queue, worker thread start/stop                    │   │
│  │  ComputeBlockVersion                                             │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  SCRIPT FLAGS & ABLA ................... lines 1493-1627 (135 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  GetNextBlockScriptFlags (static), GetMemPoolScriptFlags         │   │
│  │  MaintainAblaState (static) — ABLA block size tracking           │   │
│  │  Benchmarking timer variables                                    │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  BLOCK CONNECTION ...................... lines 1635-2009 (375 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  CChainState::ConnectBlock — THE LARGEST FUNCTION                │   │
│  │  UTXO validation, script checking, BIP30/BIP68, coinbase reward  │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  STORAGE & PERSISTENCE ................. lines 2020-2219 (200 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  FlushStateToDisk (static, 145 lines)                            │   │
│  │  FlushStateToDisk (public), PruneAndFlush, TipChanged            │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  CHAIN ACTIVATION ...................... lines 2222-3098 (877 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  UpdateTip, DisconnectTip, ConnectTip, FindMostWorkChain         │   │
│  │  PerBlockConnectTrace, ConnectTrace                              │   │
│  │  FinalizeBlockInternal, FindBlockToFinalize                      │   │
│  │  PruneBlockIndexCandidates, ActivateBestChainStep                │   │
│  │  NotifyHeaderTip, ActivateBestChain (133 lines)                  │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  BLOCK VALIDITY MANIPULATION ........... lines 3100-3444 (345 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  PreciousBlock, UnwindBlock (145 lines), FinalizeBlockAndInvalid │   │
│  │  InvalidateBlock, ParkBlock, UpdateFlagsForBlock, UpdateFlags    │   │
│  │  ResetBlockFailureFlags, UnparkBlockImpl                         │   │
│  │  GetFinalizedBlock, IsBlockFinalized                             │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  BLOCK INDEX MANAGEMENT ................ lines 3446-3542 (97 lines)     │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  AddToBlockIndex, ReceivedBlockTransactions                      │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  BLOCK VALIDATION ...................... lines 3552-4289 (738 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  CheckBlockHeader (static), CheckBlock, CheckBlockSize           │   │
│  │  ContextualCheckBlockHeader (static), ContextualCheckBlock (st.) │   │
│  │  ContextualCheckTransactionForCurrentBlock                       │   │
│  │  AcceptBlockHeader, ProcessNewBlockHeaders                       │   │
│  │  AcceptBlock (152 lines), ProcessNewBlock, TestBlockValidity     │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  PRUNING ............................... lines 4291-4456 (166 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  PruneOneBlockFile, FindFilesToPruneManual (static)              │   │
│  │  PruneBlockFilesManual, FindFilesToPrune (static)                │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  LOADING & INITIALIZATION .............. lines 4458-5146 (689 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  InsertBlockIndex, LoadBlockIndex (CChainState + standalone)     │   │
│  │  LoadBlockIndexDB (static), VerifyAblaStateForChain (static)     │   │
│  │  LoadChainTip, CVerifyDB (RAII + VerifyDB 160 lines)             │   │
│  │  RollforwardBlock, ReplayBlocks, UnloadBlockIndex                │   │
│  │  LoadGenesisBlock, LoadExternalBlockFile (159 lines)             │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  DEBUG INVARIANT CHECKER ............... lines 5308-5616 (309 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  CChainState::CheckBlockIndex — deep tree walk + assertions      │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  MEMPOOL & DSPROOF PERSISTENCE ......... lines 5618-5880 (263 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  LoadMempool, DumpMempool, DumpDSProofs, LoadDSProofs            │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  UTILITY & MISC ........................ lines 5882-5987 (106 lines)    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  GuessVerificationProgress                                       │   │
│  │  g_upgrade12_block_tracker, g_upgrade2027_block_tracker          │   │
│  │  ActivationBlockTracker::GetActivationBlock                      │   │
│  │  GetNextBlockSizeLimit, IsBIP30Repeat, IsBIP30Unspendable        │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### High-Level Call Graph (Simplified)

```
ProcessNewBlock ──► CheckBlock ──► CheckBlockHeader
       │                           CheckCoinbase / CheckRegularTransaction
       │
       ├──► AcceptBlock ──► AcceptBlockHeader ──► ContextualCheckBlockHeader
       │         │                                  CheckBlockHeader
       │         │                                  AddToBlockIndex
       │         ├──► CheckBlock
       │         ├──► ContextualCheckBlock
       │         ├──► ReceivedBlockTransactions
       │         └──► MaintainAblaState
       │
       └──► ActivateBestChain ──► FindMostWorkChain
                    │               ActivateBestChainStep
                    │                   ├──► DisconnectTip ──► DisconnectBlock
                    │                   │                       ApplyBlockUndo
                    │                   │                       UndoCoinSpend
                    │                   │
                    │                   └──► ConnectTip ──► ConnectBlock
                    │                            │           ├──► CheckBlock
                    │                            │           ├──► CheckInputs
                    │                            │           ├──► SpendCoins
                    │                            │           ├──► AddCoins
                    │                            │           └──► MaintainAblaState
                    │                            │
                    │                            └──► FlushStateToDisk
                    │
                    └──► CheckBlockIndex (regtest debug)

AcceptToMemoryPool ──► AcceptToMemoryPoolWithTime ──► AcceptToMemoryPoolWorker
                                                          ├──► CheckRegularTransaction
                                                          ├──► GetMemPoolScriptFlags
                                                          ├──► IsStandardTx
                                                          ├──► CheckSequenceLocks
                                                          ├──► CheckInputs
                                                          └──► CheckInputsFromMempoolAndCache
```

---

## 2. File Overview & Statistics

| Metric | Value |
|--------|-------|
| **Total lines** | 5,987 |
| **Project `#include`s** | 50 |
| **C++ stdlib `#include`s** | 11 (`<algorithm>` through `<utility>`) |
| **Classes defined in file** | 3 (`CChainState`, `ConnectTrace`, `PerBlockConnectTrace`) |
| **Classes implemented from header** | 3 (`CVerifyDB`, `CScriptCheck`, `ActivationBlockTracker`) |
| **CChainState methods** | 26 (18 public + 8 private, per declaration) |
| **Static (file-local) functions** | 21 |
| **Exported standalone functions** | ~61 (incl. wrapper/overload variants) |
| **Global/extern variables** | ~23 (visible outside TU) |
| **File-scope static variables** | ~24 (benchmarking timers, constants, state) |
| **`#ifdef` conditional compilation** | None — all fork activations are runtime |
| **Oversized functions (>100 lines)** | 12 |

---

## 3. Classes Defined in validation.cpp

### 3.1 CChainState (lines 89–243, declaration: 155 lines)

The central blockchain state machine. A single global instance `g_chainstate` (line 243) exists, with public aliases `ChainActive()` → `g_chainstate.m_chain` and `mapBlockIndex` → `g_chainstate.mapBlockIndex`.

**Private fields:**

| Field | Type | Description |
|-------|------|-------------|
| `setBlockIndexCandidates` | `set<CBlockIndex*, CBlockIndexWorkComparator>` | Blocks with valid TX data at least as good as tip |
| `m_cs_chainstate` | `RecursiveMutex` | Mutex for ActivateBestChain exclusion |
| `nBlockSequenceId` | `atomic<int32_t>` | Monotonic ID for received blocks |
| `nBlockReverseSequenceId` | `int32_t` | Decreasing counter for preciousblock |
| `nLastPreciousChainwork` | `arith_uint256` | Chainwork at last preciousblock call |
| `m_failed_blocks` | `set<CBlockIndex*>` | Blocks found invalid since restart |

**Public fields:**

| Field | Type | Description |
|-------|------|-------------|
| `m_chain` | `CChain` | The active best chain |
| `mapBlockIndex` | `BlockMap` | All known block headers |
| `mapBlocksUnlinked` | `multimap<CBlockIndex*, CBlockIndex*>` | Blocks missing parent data |
| `pindexBestInvalid` | `CBlockIndex*` | Highest-work invalid block |
| `pindexBestParked` | `CBlockIndex*` | Highest-work parked block |
| `pindexFinalized` | `CBlockIndex const*` | Finalization point (cannot reorg past) |

**Public methods (18):** `LoadBlockIndex`, `ActivateBestChain`, `AcceptBlockHeader`, `AcceptBlock`, `DisconnectBlock`, `ConnectBlock`, `DisconnectTip`, `PreciousBlock`, `UnwindBlock`, `ResetBlockFailureFlags`, `UpdateFlagsForBlock`, `UpdateFlags`, `UnparkBlockImpl`, `ReplayBlocks`, `LoadGenesisBlock`, `PruneBlockIndexCandidates`, `UnloadBlockIndex`

**Private methods (8):** `ActivateBestChainStep`, `ConnectTip`, `AddToBlockIndex`, `InsertBlockIndex`, `CheckBlockIndex`, `InvalidBlockFound`, `FindMostWorkChain`, `ReceivedBlockTransactions`, `RollforwardBlock`

### 3.2 PerBlockConnectTrace (lines 2345–2351, 7 lines)

Simple data struct holding a `CBlockIndex*`, a `shared_ptr<const CBlock>`, and a `shared_ptr<vector<CTransactionRef>>` for conflicted transactions. Used by `ConnectTrace`.

### 3.3 ConnectTrace (lines 2369–2412, 44 lines)

Tracks blocks connected during `ActivateBestChainStep`. Connects to `CTxMemPool::NotifyEntryRemoved` signal to capture transactions removed due to `CONFLICT` reason. Provides `GetBlocksConnected()` to retrieve the vector of `PerBlockConnectTrace` entries. Single-use class.

---

## 4. Complete Function Catalog

### 4.1 CChainState Methods (26)

| # | Method | Lines | Count | Category | Internal Calls |
|---|--------|-------|-------|----------|----------------|
| 1 | `ActivateBestChain` | 2961–3093 | 133 | Chain Activation | `FindMostWorkChain`, `ActivateBestChainStep`, `CheckBlockIndex`, `FlushStateToDisk` |
| 2 | `ActivateBestChainStep` | 2817–2928 | 112 | Chain Activation | `DisconnectTip`, `ConnectTip`, `PruneBlockIndexCandidates`, `CheckForkWarningConditions`, `CheckForkWarningConditionsOnNewFork`, `InvalidChainFound` |
| 3 | `ConnectTip` | 2506–2632 | 127 | Chain Activation | `ConnectBlock`, `FlushStateToDisk`, `UpdateTip`, `FindBlockToFinalize`, `FinalizeBlockInternal`, `GetNextBlockScriptFlags` |
| 4 | `DisconnectTip` | 2264–2337 | 74 | Chain Activation | `DisconnectBlock`, `FlushStateToDisk`, `UpdateTip`, `GetNextBlockScriptFlags` |
| 5 | `FindMostWorkChain` | 2638–2792 | 155 | Chain Activation | `InvalidChainFound`, `UnparkBlock`, `CheckForkWarningConditionsOnNewFork` |
| 6 | `PruneBlockIndexCandidates` | 2798–2810 | 13 | Chain Activation | (none internal) |
| 7 | `ConnectBlock` | 1635–2009 | 375 | Block Validation | `CheckBlock`, `CheckBlockSize`, `GetNextBlockScriptFlags`, `CheckInputs`, `SpendCoins`, `AddCoins`, `MaintainAblaState`, `GetNextBlockSizeLimit` |
| 8 | `DisconnectBlock` | 1404–1414 | 11 | UTXO/Coins | `ApplyBlockUndo` |
| 9 | `AcceptBlockHeader` | 3883–3989 | 107 | Block Validation | `CheckBlockHeader`, `ContextualCheckBlockHeader`, `AddToBlockIndex`, `CheckBlockIndex` |
| 10 | `AcceptBlock` | 4051–4202 | 152 | Block Validation | `AcceptBlockHeader`, `CheckBlock`, `ContextualCheckBlock`, `ReceivedBlockTransactions`, `MaintainAblaState`, `FlushStateToDisk`, `CheckBlockIndex` |
| 11 | `AddToBlockIndex` | 3446–3483 | 38 | Block Index | (none internal) |
| 12 | `InsertBlockIndex` | 4458–4478 | 21 | Block Index | (none internal) |
| 13 | `ReceivedBlockTransactions` | 3489–3542 | 54 | Block Index | (none internal) |
| 14 | `InvalidBlockFound` | 1180–1188 | 9 | Fork Management | `InvalidChainFound` |
| 15 | `PreciousBlock` | 3100–3137 | 38 | Block Validity | `UnparkBlock`, `PruneBlockIndexCandidates`, `ActivateBestChain` |
| 16 | `UnwindBlock` | 3144–3288 | 145 | Block Validity | `DisconnectTip`, `InvalidChainFound` |
| 17 | `ResetBlockFailureFlags` | 3383–3403 | 21 | Block Validity | `UpdateFlags` |
| 18 | `UpdateFlagsForBlock` | 3330–3352 | 23 | Block Validity | (none internal) |
| 19 | `UpdateFlags` | 3354–3381 | 28 | Block Validity | `UpdateFlagsForBlock` |
| 20 | `UnparkBlockImpl` | 3409–3425 | 17 | Block Validity | `UpdateFlags` |
| 21 | `LoadBlockIndex` | 4480–4563 | 84 | Storage/Init | `InsertBlockIndex` |
| 22 | `LoadGenesisBlock` | 5116–5142 | 27 | Storage/Init | `AddToBlockIndex`, `ReceivedBlockTransactions`, `MaintainAblaState` |
| 23 | `ReplayBlocks` | 4953–5046 | 94 | Storage/Init | `DisconnectBlock`, `RollforwardBlock` |
| 24 | `RollforwardBlock` | 4925–4951 | 27 | Storage/Init | (uses `AddCoins`, `SpendCoin` from coins.h) |
| 25 | `UnloadBlockIndex` | 5054–5058 | 5 | Storage/Init | (none internal) |
| 26 | `CheckBlockIndex` | 5308–5616 | 309 | Debug | (assertions only) |

### 4.2 Static (File-Local) Functions (21)

| # | Function | Lines | Count | Category | Called By |
|---|----------|-------|-------|----------|----------|
| 1 | `CheckInputsFromMempoolAndCache` | 445–482 | 38 | Mempool | `AcceptToMemoryPoolWorker` |
| 2 | `AcceptToMemoryPoolWorker` | 484–859 | 376 | Mempool | `AcceptToMemoryPoolWithTime` |
| 3 | `AlertNotify` | 1048–1066 | 19 | Fork Warning | `CheckForkWarningConditions` |
| 4 | `CheckForkWarningConditions` | 1068–1117 | 50 | Fork Warning | `CheckForkWarningConditionsOnNewFork`, `ActivateBestChainStep` |
| 5 | `CheckForkWarningConditionsOnNewFork` | 1119–1144 | 26 | Fork Warning | `FindMostWorkChain`, `ActivateBestChainStep` |
| 6 | `InvalidChainFound` | 1147–1176 | 30 | Fork Warning | `InvalidBlockFound`, `FindMostWorkChain`, `UnwindBlock` |
| 7 | `GetNextBlockScriptFlags` | 1493–1571 | 79 | Script Flags | `ConnectBlock`, `DisconnectTip`, `ConnectTip`, `GetMemPoolScriptFlags` |
| 8 | `MaintainAblaState` | 1589–1618 | 30 | ABLA | `ConnectBlock`, `AcceptBlock`, `LoadGenesisBlock` |
| 9 | `FlushStateToDisk` | 2020–2164 | 145 | Storage/Flush | `AcceptToMemoryPoolWithTime`, `DisconnectTip`, `ConnectTip`, `ActivateBestChain`, `AcceptBlock`, `PruneBlockFilesManual`, `FlushStateToDisk()`, `PruneAndFlush`, `VerifyAblaStateForChain` |
| 10 | `TipChanged` | 2186–2219 | 34 | Chain State | `UpdateTip`, `LoadChainTip`, `UnloadBlockIndex` |
| 11 | `UpdateTip` | 2222–2251 | 30 | Chain State | `DisconnectTip`, `ConnectTip` |
| 12 | `FinalizeBlockInternal` | 2414–2447 | 34 | Finalization | `ConnectTip`, `FinalizeBlockAndInvalidate` |
| 13 | `FindBlockToFinalize` | 2449–2496 | 48 | Finalization | `ConnectTip` |
| 14 | `NotifyHeaderTip` | 2930–2950 | 21 | Notification | `ProcessNewBlockHeaders`, `ProcessNewBlock`, `LoadExternalBlockFile` |
| 15 | `CheckBlockHeader` | 3552–3563 | 12 | Block Validation | `CheckBlock`, `AcceptBlockHeader` |
| 16 | `ContextualCheckBlockHeader` | 3667–3763 | 97 | Block Validation | `AcceptBlockHeader`, `TestBlockValidity` |
| 17 | `ContextualCheckBlock` | 3810–3876 | 67 | Block Validation | `AcceptBlock`, `TestBlockValidity` |
| 18 | `FindFilesToPruneManual` | 4332–4358 | 27 | Pruning | `FlushStateToDisk` |
| 19 | `FindFilesToPrune` | 4392–4456 | 65 | Pruning | `FlushStateToDisk` |
| 20 | `LoadBlockIndexDB` | 4565–4625 | 61 | Storage/Init | `LoadBlockIndex` |
| 21 | `VerifyAblaStateForChain` | 4632–4714 | 83 | ABLA | `LoadChainTip` |

**All 21 static functions are internally called. No orphaned static functions detected.**

### 4.3 Exported Standalone Functions (61)

| # | Function | Lines | Count | Category |
|---|----------|-------|-------|----------|
| 1 | `ChainActive` | 245–247 | 3 | Utility/Query |
| 2 | `FindForkInGlobalIndex` | 309–326 | 18 | Utility/Query |
| 3 | `TestLockPointValidity` | 345–362 | 18 | Mempool |
| 4 | `CheckSequenceLocks` | 364–433 | 70 | Mempool |
| 5 | `FormatStateMessage` | 436–441 | 6 | Utility |
| 6 | `AcceptToMemoryPoolWithTime` | 864–887 | 24 | Mempool |
| 7 | `AcceptToMemoryPool` | 889–896 | 8 | Mempool |
| 8 | `internal::FindTransactionInBlock` | 907–928 | 22 | Transaction Lookup |
| 9 | `FindTransactionInBlock` | 934–939 | 6 | Transaction Lookup |
| 10 | `GetTransaction` | 946–1001 | 56 | Transaction Lookup |
| 11 | `GetBlockSubsidy` | 1003–1014 | 12 | Utility/Query |
| 12 | `IsInitialBlockDownload` | 1016–1043 | 28 | Utility/Query |
| 13 | `SpendCoins` | 1190–1203 | 14 | UTXO/Coins |
| 14 | `UpdateCoins` (with undo) | 1205–1209 | 5 | UTXO/Coins |
| 15 | `UpdateCoins` (no undo) | 1211–1222 | 12 | UTXO/Coins |
| 16 | `GetSpendHeight` | 1246–1250 | 5 | UTXO/Coins |
| 17 | `CheckInputs` | 1252–1363 | 112 | Script Checking |
| 18 | `UndoCoinSpend` | 1366–1398 | 33 | UTXO/Coins |
| 19 | `ApplyBlockUndo` | 1416–1474 | 59 | UTXO/Coins |
| 20 | `StartScriptCheckWorkerThreads` | 1478–1480 | 3 | Script Checking |
| 21 | `StopScriptCheckWorkerThreads` | 1482–1484 | 3 | Script Checking |
| 22 | `ComputeBlockVersion` | 1486–1489 | 4 | Utility/Query |
| 23 | `GetMemPoolScriptFlags` | 1573–1582 | 10 | Script Flags |
| 24 | `FlushStateToDisk` (public) | 2166–2173 | 8 | Storage/Flush |
| 25 | `PruneAndFlush` | 2175–2183 | 9 | Storage/Flush |
| 26 | `ActivateBestChain` (wrapper) | 3095–3098 | 4 | Chain Activation |
| 27 | `PreciousBlock` (wrapper) | 3139–3142 | 4 | Block Validity |
| 28 | `FinalizeBlockAndInvalidate` | 3290–3318 | 29 | Block Validity |
| 29 | `InvalidateBlock` | 3320–3323 | 4 | Block Validity |
| 30 | `ParkBlock` | 3325–3328 | 4 | Block Validity |
| 31 | `ResetBlockFailureFlags` (wrapper) | 3405–3407 | 3 | Block Validity |
| 32 | `UnparkBlockAndChildren` | 3427–3429 | 3 | Block Validity |
| 33 | `UnparkBlock` | 3431–3433 | 3 | Block Validity |
| 34 | `GetFinalizedBlock` | 3435–3438 | 4 | Utility/Query |
| 35 | `IsBlockFinalized` | 3440–3444 | 5 | Utility/Query |
| 36 | `CheckBlock` | 3565–3639 | 75 | Block Validation |
| 37 | `CheckBlockSize` | 3641–3655 | 15 | Block Validation |
| 38 | `ContextualCheckTransactionForCurrentBlock` | 3765–3801 | 37 | Block Validation |
| 39 | `ProcessNewBlockHeaders` | 3992–4038 | 47 | Block Validation |
| 40 | `ProcessNewBlock` | 4204–4250 | 47 | Block Validation |
| 41 | `TestBlockValidity` | 4252–4289 | 38 | Block Validation |
| 42 | `PruneOneBlockFile` | 4298–4326 | 29 | Pruning |
| 43 | `PruneBlockFilesManual` | 4361–4369 | 9 | Pruning |
| 44 | `LoadChainTip` | 4716–4750 | 35 | Storage/Init |
| 45 | `CVerifyDB::CVerifyDB` | 4752–4753 | 2 | Debug |
| 46 | `CVerifyDB::~CVerifyDB` | 4756–4758 | 3 | Debug |
| 47 | `CVerifyDB::VerifyDB` | 4760–4919 | 160 | Debug |
| 48 | `ReplayBlocks` (wrapper) | 5048–5050 | 3 | Storage/Init |
| 49 | `UnloadBlockIndex` | 5063–5090 | 28 | Storage/Init |
| 50 | `LoadBlockIndex` | 5092–5114 | 23 | Storage/Init |
| 51 | `LoadGenesisBlock` (wrapper) | 5144–5146 | 3 | Storage/Init |
| 52 | `LoadExternalBlockFile` | 5148–5306 | 159 | Storage/Init |
| 53 | `LoadMempool` | 5620–5705 | 86 | Mempool Persist |
| 54 | `DumpMempool` | 5707–5761 | 55 | Mempool Persist |
| 55 | `DumpDSProofs` | 5765–5808 | 44 | DSProof Persist |
| 56 | `LoadDSProofs` | 5810–5880 | 71 | DSProof Persist |
| 57 | `GuessVerificationProgress` | 5886–5901 | 16 | Utility/Query |
| 58 | `ActivationBlockTracker::GetActivationBlock` | 5908–5964 | 57 | Fork Management |
| 59 | `GetNextBlockSizeLimit` | 5966–5976 | 11 | Utility/Query |
| 60 | `IsBIP30Repeat` | 5978–5981 | 4 | Utility/Query |
| 61 | `IsBIP30Unspendable` | 5983–5986 | 4 | Utility/Query |
| — | `CScriptCheck::operator()` | 1224–1244 | 21 | Script Checking |

### 4.4 Oversized Functions (>100 lines)

These are prime candidates for decomposition:

| # | Function | Lines | Count | Responsibility |
|---|----------|-------|-------|----------------|
| 1 | `AcceptToMemoryPoolWorker` | 484–859 | **376** | Mempool admission: tx checks, UTXO lookup, fee policy, script verification, DSProof creation |
| 2 | `ConnectBlock` | 1635–2009 | **375** | Block connection: BIP30, script flags, UTXO mutation, sig check limits, coinbase reward, ABLA |
| 3 | `CheckBlockIndex` | 5308–5616 | **309** | Debug-only tree walk: validates every invariant of the block index |
| 4 | `CVerifyDB::VerifyDB` | 4760–4919 | **160** | Startup verification: reads blocks, checks undo data, reconnects |
| 5 | `LoadExternalBlockFile` | 5148–5306 | **159** | Reindex: reads raw block files, handles out-of-order blocks |
| 6 | `FindMostWorkChain` | 2638–2792 | **155** | Chain selection: iterates candidates, handles parked/invalid/missing |
| 7 | `AcceptBlock` | 4051–4202 | **152** | Block storage: validates, checks deep reorg, writes to disk |
| 8 | `FlushStateToDisk` (static) | 2020–2164 | **145** | Persistence: prune check, block index write, UTXO flush |
| 9 | `UnwindBlock` | 3144–3288 | **145** | Invalidate/park: disconnect chain, mark status, update candidates |
| 10 | `ActivateBestChain` | 2961–3093 | **133** | Main activation loop: find best chain, step through connections |
| 11 | `ConnectTip` | 2506–2632 | **127** | Single block connect: read, validate, flush, update mempool |
| 12 | `CheckInputs` | 1252–1363 | **112** | Script verification: cache check, parallel checking, error diagnosis |
| 13 | `ActivateBestChainStep` | 2817–2928 | **112** | One activation step: disconnect old, connect new, update mempool |
| 14 | `AcceptBlockHeader` | 3883–3989 | **107** | Header validation: duplicate check, context check, failed ancestor check |

---

## 5. Global & Static Variables

### 5.1 Extern-Visible Globals (declared in validation.h)

| Variable | Type | Line | Purpose |
|----------|------|------|---------|
| `cs_main` | `RecursiveMutex` | 261 | Master lock for chainstate access |
| `mapBlockIndex` | `BlockMap&` | 263 | Reference to `g_chainstate.mapBlockIndex` |
| `pindexBestHeader` | `CBlockIndex*` | 264 | Best header seen (for getheaders) |
| `g_best_block_mutex` | `Mutex` | 265 | Protects `g_best_block` |
| `g_best_block_cv` | `condition_variable` | 266 | Notifies on new best block |
| `g_best_block` | `uint256` | 267 | Hash of current best block |
| `fIsBareMultisigStd` | `bool` | 268 | Permit bare multisig as standard |
| `fRequireStandard` | `bool` | 269 | Enforce standardness rules |
| `fCheckBlockIndex` | `bool` | 270 | Run CheckBlockIndex (regtest) |
| `fCheckpointsEnabled` | `bool` | 271 | Enable checkpoint verification |
| `nCoinCacheUsage` | `size_t` | 272 | UTXO cache size limit |
| `nMaxTipAge` | `int64_t` | 273 | Max tip age for IBD detection |
| `hashAssumeValid` | `BlockHash` | 275 | Skip script validation to this hash |
| `nMinimumChainWork` | `arith_uint256` | 276 | Minimum trusted chain work |
| `minRelayTxFee` | `CFeeRate` | 278 | Minimum relay fee rate |
| `maxTxFee` | `Amount` | 279 | Maximum transaction fee |
| `g_mempool` | `CTxMemPool` | 281 | The global transaction mempool |
| `COINBASE_FLAGS` | `CScript` | 284 | Coinbase script flags |
| `strMessageMagic` | `const string` | 286 | Message signing magic string |
| `pcoinsdbview` | `unique_ptr<CCoinsViewDB>` | 328 | Coins database view |
| `pcoinsTip` | `unique_ptr<CCoinsViewCache>` | 329 | Active UTXO cache |
| `pblocktree` | `unique_ptr<CBlockTreeDB>` | 330 | Block tree database |
| `g_upgrade12_block_tracker` | `ActivationBlockTracker` | 5904 | Tracks Upgrade 12 activation |
| `g_upgrade2027_block_tracker` | `ActivationBlockTracker` | 5906 | Tracks Upgrade 2027 activation |

### 5.2 Anonymous Namespace References (lines 289–305)

| Variable | Type | Purpose |
|----------|------|---------|
| `pindexBestInvalid` | `CBlockIndex*&` | Ref to `g_chainstate.pindexBestInvalid` |
| `pindexBestParked` | `CBlockIndex*&` | Ref to `g_chainstate.pindexBestParked` |
| `pindexFinalized` | `CBlockIndex const*&` | Ref to `g_chainstate.pindexFinalized` |
| `mapBlocksUnlinked` | `multimap&` | Ref to `g_chainstate.mapBlocksUnlinked` |

### 5.3 File-Scope State Variables

| Variable | Type | Line | Purpose |
|----------|------|------|---------|
| `g_chainstate` | `CChainState` | 243 | The singleton chain state |
| `pindexBestForkTip` | `CBlockIndex const*` | 1045 | Fork warning: best fork tip |
| `pindexBestForkBase` | `CBlockIndex const*` | 1046 | Fork warning: fork base |
| `scriptcheckqueue` | `CCheckQueue<CScriptCheck>` | 1476 | Parallel script check queue |
| `FlushStateMode` | `enum class` | 332 | NONE, IF_NEEDED, PERIODIC, ALWAYS |
| `MEMPOOL_DUMP_VERSION` | `uint64_t` | 5618 | Mempool serialization version (1) |
| `DSPROOF_DUMP_VERSION` | `uint64_t` | 5763 | DSProof serialization version (1) |

### 5.4 Benchmarking Timer Variables (14 total)

| Variable | Line | Scope |
|----------|------|-------|
| `nTimeCheck` | 1620 | ConnectBlock timing |
| `nTimeForks` | 1621 | Fork check timing |
| `nTimeVerify` | 1622 | Script verify timing |
| `nTimeConnect` | 1623 | Transaction connect timing |
| `nTimeIndex` | 1624 | Index writing timing |
| `nTimeCallbacks` | 1625 | Callback timing |
| `nTimeTotal` | 1626 | Total block time |
| `nBlocksTotal` | 1627 | Total blocks processed |
| `nTimeReadFromDisk` | 2339 | ConnectTip disk read timing |
| `nTimeConnectTotal` | 2340 | ConnectTip total timing |
| `nTimeFlush` | 2341 | ConnectTip flush timing |
| `nTimeChainState` | 2342 | ConnectTip chainstate timing |
| `nTimePostConnect` | 2343 | ConnectTip post-connect timing |

---

## 6. Proposed Refactoring Domains

Based on the call graph and functional groupings, the file splits naturally into **5 domains** (extending the 4 proposed in Architecture doc Section 20):

### Domain 1: Chain State & Activation (~1,200 lines)

**Core responsibility:** Managing the active chain tip, selecting the best chain, connecting/disconnecting blocks.

**Functions:**
- `CChainState` class (declaration + all chain activation methods)
- `ChainActive()`, `ActivateBestChain`, `ActivateBestChainStep`, `ConnectTip`, `DisconnectTip`
- `FindMostWorkChain`, `PruneBlockIndexCandidates`
- `UpdateTip`, `TipChanged`, `NotifyHeaderTip`
- `ConnectTrace`, `PerBlockConnectTrace`
- `FinalizeBlockInternal`, `FindBlockToFinalize`, `FinalizeBlockAndInvalidate`
- `PreciousBlock`, `UnwindBlock`, `InvalidateBlock`, `ParkBlock`
- `UpdateFlagsForBlock`, `UpdateFlags`, `ResetBlockFailureFlags`, `UnparkBlockImpl`, `UnparkBlock`, `UnparkBlockAndChildren`
- `GetFinalizedBlock`, `IsBlockFinalized`
- Fork warning functions: `AlertNotify`, `CheckForkWarningConditions`, `CheckForkWarningConditionsOnNewFork`, `InvalidChainFound`, `InvalidBlockFound`

**Proposed file:** `chainstate.cpp`

### Domain 2: Block Validation (~900 lines)

**Core responsibility:** Validating blocks and headers against consensus rules.

**Functions:**
- `CheckBlockHeader`, `CheckBlock`, `CheckBlockSize`
- `ContextualCheckBlockHeader`, `ContextualCheckBlock`
- `ContextualCheckTransactionForCurrentBlock`
- `AcceptBlockHeader`, `AcceptBlock`
- `ProcessNewBlock`, `ProcessNewBlockHeaders`
- `TestBlockValidity`
- `ConnectBlock` (the 375-line monster — touches UTXO too, see coupling analysis)
- `GetNextBlockScriptFlags`, `GetMemPoolScriptFlags`, `ComputeBlockVersion`
- `MaintainAblaState`

**Proposed file:** `blockvalidation.cpp`

### Domain 3: UTXO & Coins (~350 lines)

**Core responsibility:** UTXO set manipulation, script checking, coin spending/restoration.

**Functions:**
- `SpendCoins`, `UpdateCoins` (×2), `AddCoins` (from coins.h, called heavily here)
- `UndoCoinSpend`, `ApplyBlockUndo`, `DisconnectBlock`
- `CheckInputs`, `CScriptCheck::operator()`
- `GetSpendHeight`
- `scriptcheckqueue`, `StartScriptCheckWorkerThreads`, `StopScriptCheckWorkerThreads`

**Proposed file:** `utxo.cpp` or keep in existing `coins.cpp` extended

### Domain 4: Mempool Admission (~600 lines)

**Core responsibility:** Transaction validation for mempool entry.

**Functions:**
- `AcceptToMemoryPool`, `AcceptToMemoryPoolWithTime`, `AcceptToMemoryPoolWorker`
- `CheckInputsFromMempoolAndCache`
- `TestLockPointValidity`, `CheckSequenceLocks`
- `FormatStateMessage`

**Proposed file:** `mempool_accept.cpp` (Bitcoin Core already did this split)

### Domain 5: Storage, Persistence & Initialization (~1,500 lines)

**Core responsibility:** Loading/saving chain state, mempool, block files.

**Functions:**
- `FlushStateToDisk` (both overloads), `PruneAndFlush`
- `PruneOneBlockFile`, `PruneBlockFilesManual`, `FindFilesToPruneManual`, `FindFilesToPrune`
- `LoadBlockIndex`, `LoadBlockIndexDB`, `LoadChainTip`, `LoadGenesisBlock`, `LoadExternalBlockFile`
- `InsertBlockIndex`, `AddToBlockIndex`, `ReceivedBlockTransactions`
- `VerifyAblaStateForChain`, `CVerifyDB`, `VerifyDB`
- `ReplayBlocks`, `RollforwardBlock`
- `UnloadBlockIndex`
- `LoadMempool`, `DumpMempool`, `DumpDSProofs`, `LoadDSProofs`
- `CheckBlockIndex` (debug invariant checker)

**Proposed file:** `validation_storage.cpp` + `mempool_persist.cpp`

### Remaining Utility Functions

These small utility functions could stay in a slimmed-down `validation.cpp` or move to `validation_util.cpp`:

- `ChainActive`, `FindForkInGlobalIndex`, `GetBlockSubsidy`, `IsInitialBlockDownload`
- `GuessVerificationProgress`, `GetNextBlockSizeLimit`, `IsBIP30Repeat`, `IsBIP30Unspendable`
- `GetTransaction`, `FindTransactionInBlock`
- `ActivationBlockTracker::GetActivationBlock`
- Global variable definitions

---

## 7. Dependency & Coupling Analysis

### 7.1 Tightest Couplings (Splitting Hazards)

#### `ConnectBlock` — The Hub Function (375 lines)

`ConnectBlock` is the single most coupled function in the file. It touches **all four proposed domains**:

| Domain | What ConnectBlock Does |
|--------|----------------------|
| Block Validation | Calls `CheckBlock`, `CheckBlockSize`, `GetNextBlockScriptFlags` |
| UTXO/Coins | Calls `AddCoins`, `SpendCoins`, `CheckInputs`, manages `CBlockUndo` |
| Chain State | Writes `setDirtyBlockIndex`, calls `MaintainAblaState` |
| Storage | Calls `WriteUndoDataForBlock`, sets `view.SetBestBlock()` |

**Recommendation:** `ConnectBlock` must live in whichever file owns the "block connection" responsibility. The UTXO functions it calls (`SpendCoins`, `AddCoins`, `CheckInputs`) should be accessible via header includes.

#### `FlushStateToDisk` — Called from 9 Sites

This static function is called from mempool admission, chain activation, tip connection/disconnection, block acceptance, manual pruning, and ABLA verification. It couples storage to every other domain.

**Recommendation:** Extract to a standalone function in `validation_storage.cpp` and expose via header.

#### `g_chainstate` — The God Object

The `CChainState` singleton holds `mapBlockIndex`, `m_chain`, `setBlockIndexCandidates`, `pindexFinalized`, and all the methods that mutate them. Every domain accesses it.

**Recommendation:** This is the hardest coupling to break. Phase 1 should keep `CChainState` intact but move method implementations to domain-specific files while keeping the class declaration in one header.

### 7.2 Global State Coupling Matrix

```
                    cs_main  mapBlockIndex  pcoinsTip  g_mempool  g_chainstate
                    -------  -------------  ---------  ---------  ------------
Chain Activation      ✓           ✓             ✓          ✓          ✓
Block Validation      ✓           ✓             ✓                     ✓
UTXO/Coins            ✓                         ✓
Mempool Admission     ✓           (via chain)   ✓          ✓
Storage/Init          ✓           ✓             ✓          ✓          ✓
```

### 7.3 Cross-Domain Function Calls

| Caller Domain | Callee Domain | Functions Called |
|---------------|---------------|-----------------|
| Chain Activation → Block Validation | `ConnectBlock`, `CheckBlock` |
| Chain Activation → UTXO/Coins | via `ConnectBlock` → `SpendCoins`, `CheckInputs` |
| Chain Activation → Storage | `FlushStateToDisk`, `WriteUndoDataForBlock` |
| Block Validation → UTXO/Coins | `CheckInputs`, `SpendCoins`, `AddCoins` |
| Block Validation → Storage | `MaintainAblaState` (writes `setDirtyBlockIndex`) |
| Mempool → UTXO/Coins | `CheckInputs`, `CheckInputsFromMempoolAndCache` |
| Mempool → Storage | `FlushStateToDisk` (periodic) |

### 7.4 Recommended Split Order

1. **Mempool admission** (easiest — mostly self-contained, calls into UTXO via header)
2. **Mempool/DSProof persistence** (standalone file I/O)
3. **UTXO/Coins operations** (well-defined interface)
4. **Block validation** (moderate coupling to ConnectBlock)
5. **Chain activation + storage** (hardest — deeply intertwined)

---

## 8. Unused Code Audit

### 8.1 Confirmed Unused / Dead Code

| Item | Location | Evidence | Safe to Remove? |
|------|----------|----------|-----------------|
| **`NodeCRef`** type alias | `net.h:131` | Grep for `NodeCRef` finds only its declaration. Self-documented comment says "unused". Zero references anywhere in codebase. | **Yes** |
| **`OBJ_USER_KEYS`** code path | `rpc/util.h:59`, `rpc/util.cpp:151,373,434,457` | The `OBJ_USER_KEYS` enum case at line 434 contains `assert(false)` — explicitly dead code. The enum value exists in the header and is handled in 4 switch statements, but no `RPCArg` is ever constructed with `Type::OBJ_USER_KEYS` anywhere in the codebase. All switch cases are dead. | **Yes** (remove enum value + all switch cases) |
| **`BlockValidity::UNKNOWN`** | `blockvalidity.h:10-13` | Commented "Unused" in source. Used as zero-value sentinel in `BlockStatus` default initialization. Also explicitly referenced in `test/blockstatus_tests.cpp:38,130` and `test/blockindex_tests.cpp:44,268` for test coverage of the enum range. | **Low priority** — zero-sentinel with test coverage |

### 8.2 Stale TODOs / Legacy Code Flagged for Removal

| Item | Location | Notes |
|------|----------|-------|
| **`NODE_BITCOIN_CASH`** service bit | `protocol.h:314-316`, `init.cpp:1992` | Defined at `protocol.h:316` with TODO to remove. Set unconditionally via `nLocalServices |= NODE_BITCOIN_CASH` in `init.cpp:1992`. Also referenced in: `qt/guiutil.cpp:898` (GUI display), `seeder/bitcoin.cpp:98,314` (seeder handshake), `seeder/test/p2p_messaging_tests.cpp:85,152` (test). The UAHF hard fork is permanent — this bit no longer differentiates BCH from BTC. Removing requires coordinated network protocol deprecation. |
| **`SIGHASH_FORKID` TODO** | `core_write.cpp:175` | Comment: `// TODO: Remove SIGHASH_FORKID from flag definitions when we do not need to test against it anymore`. The hard fork is permanent; the flag is always set. The TODO is stale but the code path is not dead — it's used for signature serialization display. |
| **`GetDataDir()` side-effect call** | `bitcoind.cpp:~137` | A call to `GetDataDir()` exists purely for its side-effect of creating the data directory. Comment in code flags this for cleanup. |
| **`tableRPC.execute()` shim** | `rpc/server.cpp:~97` | Backward-compatibility wrapper around the newer `CRPCCommand::execute()` interface. Flagged as a shim in comments. |
| **`-bytespersigop` deprecated alias** | `init.cpp:1086-1088` (definition), `init.cpp:1965-1973` (handling) | Legacy name for `-bytespersigcheck`. Both names are handled as aliases with mutual exclusion check (`"may not both be specified"`). Deprecated annotation in help text. |
| **`getinfo` tombstone** | `rpc/misc.cpp:529-541` (handler), line 618 (registration as "hidden") | `getinfo_deprecated()` throws `RPC_METHOD_NOT_FOUND` with a helpful migration message. Registered in the "hidden" category so it doesn't appear in `help` output. |

### 8.3 Platform-Specific Notes (Windows Target)

| Item | Location | Notes |
|------|----------|-------|
| **`ShellEscape()`** | `util/system.cpp:~1326` | Unix-only function for shell argument escaping. On Windows, the `%w` wallet notification pattern is silently skipped. This is a documented limitation, not dead code. |
| **Signal handlers** | `init.cpp` | Unix uses `SIGTERM`/`SIGHUP` handlers; Windows uses `consoleCtrlHandler`. Both paths are active on their respective platforms. Correct `#ifdef WIN32` branching. |
| **`bitcoin-seeder`** | `src/seeder/` | Entire program is Unix-only (uses raw POSIX socket headers like `<arpa/inet.h>`). Excluded from Windows CMake build via `if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")`. Not dead — just not buildable on Windows. |
| **`FlushBlockFile` forward decl** | `validation.cpp:307` | Forward-declares `FlushBlockFile()` which is defined in `node/blockstorage.cpp`. This is an architectural smell (validation.cpp calling directly into blockstorage without going through a header), but it is functional and used by `FlushStateToDisk`. |

### 8.4 validation.cpp Internal Audit

**All 21 static functions are called internally.** No orphaned or dead functions found within validation.cpp.

**All 26 CChainState methods are called.** The class has no unused methods.

**All 61 exported standalone functions are referenced** from other translation units (verified via header declarations in `validation.h` and grep across `src/`).

### 8.5 Additional Observations

| Item | Location | Notes |
|------|----------|-------|
| **`#define MICRO` / `#define MILLI`** | `validation.cpp:71-72` | Old-style `#define` constants. Could be `constexpr double` but not dead code — used throughout benchmarking `LogPrint` calls. |
| **`FlushStateMode` enum** | `validation.cpp:332` | File-local enum defined between global variables and forward declarations. Should move closer to `FlushStateToDisk`. |
| **`MEMPOOL_DUMP_VERSION` / `DSPROOF_DUMP_VERSION`** | Lines 5618, 5763 | Both are version 1 with no version migration code. If the format ever changes, the load functions just return false on version mismatch. |
| **`pindexBestForkTip` / `pindexBestForkBase`** | Lines 1045-1046 | File-scope `const` pointers modified via `const_cast`-like patterns (assigned in `CheckForkWarningConditionsOnNewFork`). Should be non-const or moved into CChainState. |

---

## 9. Test Coverage Map

This section maps which unit tests and functional tests exercise each validation.cpp function and code path. When splitting validation.cpp into smaller files during refactoring, use this map to identify exactly which tests must pass to verify each domain.

### 9.1 Unit Tests (src/test/)

21 test files directly test validation.cpp functions:

| Test File | Lines | Functions Tested | Domain |
|-----------|-------|------------------|--------|
| `validation_tests.cpp` | 132 | `GetBlockSubsidy`, `LoadExternalBlockFile` | Utility, Storage |
| `validation_block_tests.cpp` | 208 | `ProcessNewBlock`, `ProcessNewBlockHeaders` | Block Validation |
| `blockcheck_tests.cpp` | 263 | `CheckBlock`, `ReadRawBlockFromDisk`, `ReadBlockSizeFromDisk` | Block Validation |
| `txvalidation_tests.cpp` | 115 | `AcceptToMemoryPool` | Mempool |
| `txvalidationcache_tests.cpp` | 673 | `AcceptToMemoryPool`, `CheckInputs` | Mempool, Script |
| `mempool_tests.cpp` | 914 | Mempool ops (indirect via `AcceptToMemoryPool`) | Mempool |
| `miner_tests.cpp` | 747 | `CreateNewBlock`, `CheckSequenceLocks` | Mempool, Block Validation |
| `blockchain_tests.cpp` | 74 | `GetDifficulty` (uses chain data) | Utility |
| `coins_tests.cpp` | 915 | `UpdateCoins`, coin cache | UTXO/Coins |
| `undo_tests.cpp` | 98 | `UpdateCoins`, `ApplyBlockUndo` | UTXO/Coins |
| `checkpoints_tests.cpp` | 246 | `ProcessNewBlockHeaders`, checkpoint validation | Block Validation |
| `denialofservice_tests.cpp` | 583 | Block/tx validation in DoS context | Chain Activation |
| `finalization_tests.cpp` | 96 | `GetFinalizedBlock`, block finalization | Chain Activation |
| `finalization_header_tests.cpp` | 281 | `ProcessNewBlockHeaders`, finalization rules | Chain Activation |
| `blockstorage_tests.cpp` | 36 | `ReadBlockFromDisk` | Storage |
| `dsproof_tests.cpp` | 842 | `AcceptToMemoryPool`, DSProof creation | Mempool |
| `blockstatus_tests.cpp` | 136 | `BlockStatus` validity states | Block Validation |
| `token_tests.cpp` | 2,035 | Token tx validation, `CreateNewBlock` | Block Validation |
| `activation_tests.cpp` | 521 | Consensus activation by height/MTP | Block Validation |
| `transaction_tests.cpp` | 1,140 | Transaction verification, script execution | Script/UTXO |
| `txindex_tests.cpp` | 86 | `ProcessNewBlock` (indirect via indexing) | Storage |

**Total: ~10,200 lines of unit test code**

### 9.2 Functional Tests (test/functional/)

~39 functional test files exercise validation.cpp code paths, grouped by category:

#### Block Validation & Chain Reorg (6 tests)

| Test File | Lines | Functions Exercised |
|-----------|-------|---------------------|
| `feature_block.py` | 1,163 | Comprehensive block consensus rules |
| `abc-invalid-chains.py` | 158 | `InvalidateBlock`, chain reorg |
| `abc-parkedchain.py` | 253 | `ParkBlock`/`UnparkBlock` |
| `rpc_invalidateblock.py` | 76 | `InvalidateBlock`, reorg |
| `rpc_preciousblock.py` | 127 | `PreciousBlock` |
| `abc-finalize-block.py` | 332 | `FinalizeBlockAndInvalidate`, `IsBlockFinalized` |

#### Mempool & Tx Acceptance (6 tests)

| Test File | Lines | Functions Exercised |
|-----------|-------|---------------------|
| `mempool_accept.py` | 356 | `AcceptToMemoryPool`, `CheckInputs` |
| `mempool_reorg.py` | 125 | Mempool resurrection on reorg |
| `mempool_packages.py` | 259 | Package validation |
| `mempool_resurrect.py` | 75 | Tx resurrection after disconnect |
| `mempool_spend_coinbase.py` | 59 | Coinbase maturity |
| `mempool_expiry.py` | 114 | Tx eviction |

#### P2P Invalid Block/Tx (4 tests)

| Test File | Lines | Functions Exercised |
|-----------|-------|---------------------|
| `p2p_invalid_block.py` | 124 | `CheckBlock`, `ProcessNewBlock` |
| `p2p_invalid_tx.py` | 250 | Tx validation errors |
| `p2p_invalid_messages.py` | 307 | Malformed messages |
| `p2p_invalid_locator.py` | 53 | Block locator validation |

#### Consensus Activation & Script (8 tests)

| Test File | Lines | Functions Exercised |
|-----------|-------|---------------------|
| `feature_csv_activation.py` | 678 | CSV/BIP68/112/113 |
| `feature_cltv.py` | 223 | CHECKLOCKTIMEVERIFY |
| `feature_bip68_sequence.py` | 493 | `CheckSequenceLocks` |
| `feature_dersig.py` | 223 | DER signature enforcement |
| `feature_block_sigchecks.py` | 300 | Sigcheck limits |
| `feature_abla.py` | 180 | ABLA block size |
| `feature_native_introspection.py` | 1,382 | Introspection opcodes |
| `feature_int64_cscriptnum.py` | 678 | 64-bit script numbers |

#### Block Processing & Chain Download (6 tests)

| Test File | Lines | Functions Exercised |
|-----------|-------|---------------------|
| `feature_pruning.py` | 528 | `PruneOneBlockFile`, `PruneBlockFilesManual` |
| `feature_assumevalid.py` | 209 | `CheckInputs` skip, `IsInitialBlockDownload` |
| `feature_minchainwork.py` | 101 | IBD with minimum chainwork |
| `p2p_compactblocks.py` | 1,062 | Compact block validation |
| `abc-p2p-compactblocks.py` | 361 | Compact blocks + tx ordering |
| `abc-p2p-fullblocktest.py` | 268 | Full block consensus |

#### Mining & Block Templates (3 tests)

| Test File | Lines | Functions Exercised |
|-----------|-------|---------------------|
| `mining_basic.py` | 316 | Block submission |
| `mining_prioritisetransaction.py` | 219 | Tx prioritization |
| `rpc_validateblocktemplate.py` | 386 | `TestBlockValidity` |

#### Database & Recovery (2 tests)

| Test File | Lines | Functions Exercised |
|-----------|-------|---------------------|
| `feature_dbcrash.py` | 325 | Crash recovery, `FlushStateToDisk`, `ReplayBlocks` |
| `feature_reindex.py` | 41 | `LoadBlockIndex`, reindex |

#### Additional (4 tests)

| Test File | Lines | Functions Exercised |
|-----------|-------|---------------------|
| `abc-mempool-coherence-on-activations.py` | 371 | Activation boundary mempool coherence |
| `abc-schnorr.py` | 252 | Schnorr signatures |
| `abc-replay-protection.py` | 304 | Replay protection |
| `vbchn-feature-checkblockreads.py` | — | Block reading |

**Total: ~15,000+ lines of functional test code**

### 9.3 Coverage by Refactoring Domain

When a domain is split to its own file, run the tests listed here to verify correctness:

| Domain | Unit Tests | Functional Tests |
|--------|-----------|-----------------|
| **Chain State & Activation** | `finalization_tests`, `finalization_header_tests`, `denialofservice_tests` | `abc-finalize-block`, `abc-parkedchain`, `rpc_invalidateblock`, `rpc_preciousblock`, `abc-invalid-chains` |
| **Block Validation** | `blockcheck_tests`, `validation_block_tests`, `checkpoints_tests`, `blockstatus_tests`, `activation_tests`, `token_tests` | `feature_block`, `abc-p2p-fullblocktest`, `feature_block_sigchecks`, `feature_abla`, `p2p_invalid_block`, `feature_csv_activation`, `feature_cltv`, `feature_dersig` |
| **UTXO & Coins** | `coins_tests`, `undo_tests`, `txvalidationcache_tests`, `transaction_tests` | `feature_assumevalid` |
| **Mempool Admission** | `txvalidation_tests`, `txvalidationcache_tests`, `mempool_tests`, `miner_tests`, `dsproof_tests` | `mempool_accept`, `mempool_reorg`, `mempool_packages`, `mempool_resurrect`, `mempool_spend_coinbase`, `abc-mempool-coherence-on-activations` |
| **Storage & Persistence** | `validation_tests`, `blockstorage_tests`, `txindex_tests` | `feature_pruning`, `feature_dbcrash`, `feature_reindex` |

### 9.4 Coverage Gaps

23 functions with NO direct test references — all are static/internal or rarely-invoked utility functions.

#### Static functions (15) — only exercised indirectly through higher-level code paths

| Function | Why No Direct Test |
|----------|--------------------|
| `FlushStateToDisk` | Called implicitly during block connection/disconnection |
| `TipChanged` | Called by `UpdateTip` and `LoadChainTip` |
| `UpdateTip` | Called by `DisconnectTip` and `ConnectTip` |
| `FinalizeBlockInternal` | Called by `ConnectTip` and `FinalizeBlockAndInvalidate` |
| `FindBlockToFinalize` | Called by `ConnectTip` |
| `NotifyHeaderTip` | Called by `ProcessNewBlockHeaders`, `ProcessNewBlock`, `LoadExternalBlockFile` |
| `FindFilesToPruneManual` | Called by `FlushStateToDisk` |
| `FindFilesToPrune` | Called by `FlushStateToDisk` |
| `LoadBlockIndexDB` | Called by `LoadBlockIndex` |
| `VerifyAblaStateForChain` | Called by `LoadChainTip` |
| `AlertNotify` | Called by `CheckForkWarningConditions` |
| `CheckForkWarningConditions` | Called by `ActivateBestChainStep` |
| `CheckForkWarningConditionsOnNewFork` | Called by `FindMostWorkChain`, `ActivateBestChainStep` |
| `InvalidChainFound` | Called by `InvalidBlockFound`, `FindMostWorkChain`, `UnwindBlock` |
| `MaintainAblaState` | Called by `ConnectBlock`, `AcceptBlock`, `LoadGenesisBlock` |

#### Exported functions (8) — no dedicated tests

| Function | Notes |
|----------|-------|
| `GuessVerificationProgress` | Estimation utility, only used in UI/RPC display |
| `ComputeBlockVersion` | Simple version computation, called by miner |
| `IsBIP30Repeat` | Historical BTC quirk — returns `false` for all BCH blocks |
| `IsBIP30Unspendable` | Historical BTC quirk — returns `false` for all BCH blocks |
| `DumpMempool` | Serialization to disk on shutdown |
| `LoadMempool` | Deserialization from disk on startup |
| `DumpDSProofs` | DSProof serialization to disk on shutdown |
| `LoadDSProofs` | DSProof deserialization from disk on startup |

#### Utility-only references (2) — used in test setup/teardown but not directly tested

| Function | Notes |
|----------|-------|
| `FormatStateMessage` | Only used in `setup_common.cpp` error handling |
| `UnloadBlockIndex` | Only used in test cleanup |

#### Debug invariant checker

`CheckBlockIndex` (309-line debug invariant checker) is only enabled via `fCheckBlockIndex = true` in test setup — it runs implicitly during every regtest block connection but has no dedicated correctness test of its own.

---

## Appendix: Include Dependency List

### Project Headers (50)

```
validation.h, arith_uint256.h, blockindexworkcomparator.h, blockvalidity.h,
chainparams.h, checkpoints.h, checkqueue.h, config.h,
consensus/activation.h, consensus/consensus.h, consensus/merkle.h,
consensus/tokens.h, consensus/tx_check.h, consensus/tx_verify.h,
consensus/validation.h, dsproof/dsproof.h, dsproof/storage.h,
flatfile.h, fs.h, hash.h, index/txindex.h, node/blockstorage.h,
policy/fees.h, policy/mempool.h, policy/policy.h, pow.h,
primitives/block.h, primitives/transaction.h, random.h,
reverse_iterator.h, script/script.h, script/scriptcache.h,
script/sigcache.h, script/standard.h, shutdown.h, span.h,
timedata.h, tinyformat.h, txdb.h, txmempool.h, ui_interface.h,
undo.h, util/defer.h, util/moneystr.h, util/strencodings.h,
util/string.h, util/system.h, util/time.h, validationinterface.h,
warnings.h
```

### C++ Standard Library Headers (11)

```
<algorithm>, <atomic>, <ctime>, <deque>, <iterator>,
<limits>, <list>, <optional>, <string>, <thread>, <utility>
```
