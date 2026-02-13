# Vuhn Bitcoin Cash Node — Software Architecture Reference

> **Source:** Forked from Bitcoin Cash Node (BCHN) v29.0.0
>
> **Repository:** `vu-hn/Vuhn-Bitcoin-Cash-Node`
>
> **Local Path:** `F:\ClaudeProjects\cryptocurrency\vuhn-bitcoin-cash-node\`
>
> **Upstream:** `https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node.git`
>
> **Language:** C++20 | **Build System:** CMake | **License:** MIT

---

## Table of Contents

1. [High-Level Architecture](#1-high-level-architecture)
2. [Repository Layout](#2-repository-layout)
3. [Executables & Entry Points](#3-executables--entry-points)
4. [Internal Libraries (Build Targets)](#4-internal-libraries-build-targets)
5. [Bundled Third-Party Libraries](#5-bundled-third-party-libraries)
6. [External Dependencies](#6-external-dependencies)
7. [Core Subsystems & Data Flow](#7-core-subsystems--data-flow)
8. [Key Classes & Relationships](#8-key-classes--relationships)
9. [Threading Model](#9-threading-model)
10. [Storage Layer](#10-storage-layer)
11. [Network Protocol](#11-network-protocol)
12. [Script Engine & VM](#12-script-engine--vm)
13. [Signal / Notification System](#13-signal--notification-system)
14. [Configuration System](#14-configuration-system)
15. [Logging System](#15-logging-system)
16. [Test Infrastructure](#16-test-infrastructure)
17. [Build System Details](#17-build-system-details)
18. [Windows Build Notes](#18-windows-build-notes)
19. [Linting & Code Style Infrastructure](#19-linting--code-style-infrastructure)
20. [Source Code Size Analysis](#20-source-code-size-analysis)

---

## 1. High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                          APPLICATION LAYER                          │
│  bitcoind (daemon)  bitcoin-qt (GUI)  bitcoin-cli  bitcoin-tx/wallet│
└──────────┬──────────────────┬──────────────────┬────────────────────┘
           │                  │                  │
┌──────────▼──────────────────▼──────────────────▼────────────────────┐
│                           INTERFACE LAYER                           │
│   JSON-RPC Server  │  REST API  │  ZeroMQ Publisher  │  Qt GUI      │
└──────────┬──────────────────┬──────────────────┬────────────────────┘
           │                  │                  │
┌──────────▼──────────────────▼──────────────────▼────────────────────┐
│                      VALIDATION / POLICY LAYER                      │
│  CChainState  │  CTxMemPool  │  Policy Rules  │  Fee Estimation     │
└──────────┬──────────────────┬──────────────────┬────────────────────┘
           │                  │                  │
┌──────────▼──────────────────▼──────────────────▼────────────────────┐
│                           CONSENSUS LAYER                           │
│  Block Validation  │  Tx Verification  │  Script Interpreter        │
│  PoW Checking      │  UTXO Management  │  Signature Validation      │
└──────────┬──────────────────┬──────────────────┬────────────────────┘
           │                  │                  │
┌──────────▼──────────┐ ┌─────▼─────────────┐ ┌──▼────────────────────┐
│   NETWORK LAYER     │ │  STORAGE LAYER    │ │  CRYPTO LAYER         │
│ CConnman / CNode    │ │ LevelDB (UTXO)    │ │ secp256k1             │
│ P2P Protocol        │ │ FlatFile (blocks) │ │ SHA-256 (HW accel)    │
│ Peer Management     │ │ BerkeleyDB (wal)  │ │ RIPEMD-160, AES       │
└─────────────────────┘ └───────────────────┘ └───────────────────────┘
```

The codebase is a **monolithic C++ application** descended from Bitcoin Core through Bitcoin ABC. All layers live in a single `src/` tree. Components communicate through direct function calls and an observer-pattern signal system (`CValidationInterface`).

---

## 2. Repository Layout

```
vuhn-bitcoin-cash-node/
├── CMakeLists.txt              # Root build configuration (C++20)
├── PackageOptions.cmake        # Packaging/install options
├── COPYING                     # MIT License
├── README.md
├── CONTRIBUTING.md
├── DISCLOSURE_POLICY.md
├── INSTALL.md
├── crowdin.yml                 # Translation config
├── Dockerfile-doxygen          # Docs generation
│
├── src/                        # ===== ALL SOURCE CODE =====
│   ├── bitcoind.cpp            # Daemon entry point
│   ├── bitcoin-cli.cpp         # CLI client entry point
│   ├── bitcoin-tx.cpp          # Offline tx tool entry point
│   ├── bitcoin-wallet.cpp      # Offline wallet tool entry point
│   │
│   ├── init.cpp/h              # Application initialization (138KB)
│   ├── validation.cpp/h        # Core blockchain validation (253KB - largest file)
│   ├── net.cpp/h               # P2P networking (109KB)
│   ├── net_processing.cpp/h    # P2P message processing (224KB)
│   ├── txmempool.cpp/h         # Transaction memory pool
│   ├── chain.cpp/h             # Blockchain state (CChain, CBlockIndex)
│   ├── coins.cpp/h             # UTXO set management
│   ├── pow.cpp/h               # Proof-of-work validation
│   ├── miner.cpp/h             # Block template creation
│   ├── chainparams.cpp/h       # Network parameters (mainnet/testnet/regtest)
│   │
│   ├── consensus/              # Consensus rules
│   │   ├── params.h            #   Consensus parameters struct
│   │   ├── validation.h        #   Consensus validation interfaces
│   │   ├── tx_check.cpp/h      #   Transaction structure checks
│   │   ├── tx_verify.cpp/h     #   Transaction input verification
│   │   ├── merkle.cpp/h        #   Merkle tree computation
│   │   ├── activation.cpp/h    #   Protocol upgrade activation
│   │   ├── abla.cpp/h          #   Adaptive Block Limit Algorithm
│   │   └── tokens.cpp/h        #   CashToken consensus rules
│   │
│   ├── primitives/             # Core data structures
│   │   ├── block.cpp/h         #   CBlock, CBlockHeader
│   │   ├── transaction.cpp/h   #   CTransaction, CTxIn, CTxOut
│   │   ├── token.cpp/h         #   Token data structures
│   │   ├── blockhash.h         #   BlockHash type
│   │   └── txid.h              #   TxId type
│   │
│   ├── script/                 # Script engine (543KB)
│   │   ├── interpreter.cpp/h   #   EvalScript, VerifyScript (147KB)
│   │   ├── script.cpp/h        #   CScript, opcodes
│   │   ├── bigint.cpp/h        #   Arbitrary precision integers
│   │   ├── bitfield.cpp/h      #   Bitfield operations
│   │   ├── standard.cpp/h      #   Standard script patterns
│   │   ├── sign.cpp/h          #   Transaction signing
│   │   ├── sigcache.cpp/h      #   Signature validation cache
│   │   ├── scriptcache.cpp/h   #   Script execution cache
│   │   ├── bitcoinconsensus.cpp/h  # Shared consensus library API
│   │   ├── vm_limits.h         #   VM resource limits
│   │   └── descriptor.cpp/h    #   Output descriptors
│   │
│   ├── crypto/                 # Cryptographic primitives (360KB)
│   │   ├── sha256.cpp/h        #   SHA-256 (+ AVX2, SSE4, SHANI variants)
│   │   ├── sha512.cpp/h        #   SHA-512
│   │   ├── sha3.cpp/h          #   SHA-3
│   │   ├── sha1.cpp/h          #   SHA-1
│   │   ├── ripemd160.cpp/h     #   RIPEMD-160
│   │   ├── hmac_sha256.cpp/h   #   HMAC-SHA256
│   │   ├── hmac_sha512.cpp/h   #   HMAC-SHA512
│   │   ├── aes.cpp/h           #   AES encryption
│   │   ├── chacha20.cpp/h      #   ChaCha20 stream cipher
│   │   ├── siphash.cpp/h       #   SipHash (bloom filters)
│   │   ├── muhash.cpp/h        #   MuHash (UTXO commitments)
│   │   └── ctaes/              #   Constant-time AES implementation
│   │
│   ├── wallet/                 # Wallet subsystem (774KB)
│   │   ├── wallet.cpp/h        #   CWallet (183KB)
│   │   ├── walletdb.cpp/h      #   Wallet database
│   │   ├── coinselection.cpp/h #   Coin selection algorithms
│   │   ├── crypter.cpp/h       #   Wallet encryption
│   │   ├── rpcwallet.cpp/h     #   Wallet RPC commands (209KB)
│   │   ├── rpcdump.cpp/h       #   Import/export RPC
│   │   ├── fees.cpp/h          #   Fee estimation
│   │   └── init.cpp            #   Wallet initialization
│   │
│   ├── rpc/                    # JSON-RPC server (585KB)
│   │   ├── server.cpp/h        #   RPC server infrastructure
│   │   ├── blockchain.cpp      #   Blockchain RPCs (145KB)
│   │   ├── mining.cpp          #   Mining RPCs (69KB)
│   │   ├── rawtransaction.cpp  #   Raw transaction RPCs (100KB)
│   │   ├── net.cpp             #   Network RPCs
│   │   ├── misc.cpp            #   Miscellaneous RPCs
│   │   ├── command.cpp/h       #   RPC command base class
│   │   └── register.h          #   Command registration
│   │
│   ├── qt/                     # Qt GUI application (1158KB)
│   │   ├── bitcoin.cpp/h       #   Main application class
│   │   ├── bitcoingui.cpp/h    #   Main window (58KB)
│   │   ├── clientmodel.cpp/h   #   Client data model
│   │   ├── walletmodel.cpp/h   #   Wallet data model
│   │   ├── forms/              #   Qt .ui form files
│   │   ├── res/                #   Icons, images, resources
│   │   └── locale/             #   Translations
│   │
│   ├── interfaces/             # Abstract API boundaries
│   │   ├── chain.cpp/h         #   Chain interface (node ↔ wallet)
│   │   ├── node.cpp/h          #   Node interface (node ↔ GUI)
│   │   ├── wallet.cpp/h        #   Wallet interface
│   │   └── handler.cpp/h       #   Event handler interface
│   │
│   ├── node/                   # Node management
│   │   ├── blockstorage.cpp/h  #   Block file management
│   │   ├── context.h           #   Node context struct
│   │   └── transaction.cpp/h   #   Transaction broadcasting
│   │
│   ├── index/                  # Optional blockchain indexes
│   │   ├── base.cpp/h          #   Base index class
│   │   ├── txindex.cpp/h       #   Transaction index (-txindex)
│   │   └── coinstatsindex.cpp/h#   Coin statistics index
│   │
│   ├── policy/                 # Transaction relay policy
│   │   ├── policy.cpp/h        #   Standard policy rules
│   │   ├── fees.cpp/h          #   Fee estimation
│   │   └── mempool.h           #   Mempool acceptance policy
│   │
│   ├── dsproof/                # Double-Spend Proof (BCHN feature, 116KB)
│   │   ├── dsproof.cpp/h       #   Core DSP class
│   │   ├── dsproof_create.cpp  #   DSP creation
│   │   ├── dsproof_validate.cpp#   DSP validation
│   │   └── storage.cpp/h       #   DSP storage
│   │
│   ├── zmq/                    # ZeroMQ notifications (92KB)
│   │   ├── zmqnotificationinterface.cpp/h
│   │   ├── zmqpublishnotifier.cpp/h
│   │   └── zmqrpc.cpp/h
│   │
│   ├── util/                   # Utilities (271KB)
│   │   ├── system.cpp/h        #   ArgsManager, system utilities (49KB)
│   │   ├── strencodings.cpp/h  #   String encoding
│   │   ├── time.cpp/h          #   Time utilities
│   │   ├── thread.cpp/h        #   Threading utilities
│   │   └── moneystr.cpp/h      #   Currency formatting
│   │
│   ├── compat/                 # Platform compatibility (94KB)
│   ├── support/                # Memory allocators, secure cleanse (85KB)
│   ├── algorithm/              # Container utilities
│   │
│   ├── secp256k1/              # ===== BUNDLED: Elliptic curve crypto =====
│   ├── leveldb/                # ===== BUNDLED: Key-value store =====
│   ├── univalue/               # ===== BUNDLED: JSON library =====
│   │
│   ├── seeder/                 # DNS seed crawler utility
│   ├── test/                   # C++ unit tests
│   ├── bench/                  # Performance benchmarks
│   └── config/                 # Build-time generated headers
│
├── test/                       # ===== TEST SUITES =====
│   ├── functional/             #   Python integration tests (~200 test files)
│   │   ├── test_framework/     #     Test framework library
│   │   ├── test_runner.py      #     Main test runner
│   │   ├── abc_*.py            #     Protocol feature tests
│   │   ├── bchn_*.py           #     BCHN-specific tests
│   │   ├── feature_*.py        #     Core feature tests
│   │   ├── interface_*.py      #     RPC/REST/ZMQ tests
│   │   ├── mempool_*.py        #     Mempool behavior tests
│   │   ├── mining_*.py         #     Mining tests
│   │   ├── p2p_*.py            #     P2P protocol tests
│   │   ├── rpc_*.py            #     RPC command tests
│   │   └── wallet_*.py         #     Wallet tests
│   ├── fuzz/                   #   Fuzzing harnesses
│   ├── lint/                   #   Code linting scripts
│   └── sanitizer_suppressions/ #   ASan/TSan/UBSan configs
│
├── cmake/modules/              # ===== CMAKE MODULES =====
│   ├── AddCompilerFlags.cmake  #   Compiler flag management
│   ├── FindBerkeleyDB.cmake    #   BDB detection
│   ├── FindEvent.cmake         #   libevent detection
│   ├── FindGMP.cmake           #   GMP detection
│   ├── FindZeroMQ.cmake        #   ZMQ detection
│   ├── Sanitizers.cmake        #   ASan/TSan/UBSan setup
│   ├── TestSuite.cmake         #   Test infrastructure
│   └── NSIS.template.in        #   Windows installer template
│
├── depends/                    # ===== CROSS-COMPILATION DEPENDENCIES =====
│   ├── Makefile                #   Dependency build orchestration
│   ├── packages/               #   32 package definitions (.mk files)
│   ├── patches/                #   Dependency patches
│   ├── builders/               #   Build system helpers
│   └── hosts/                  #   Platform-specific configs
│
├── doc/                        # ===== DOCUMENTATION =====
│   ├── developer-notes.md      #   Core development guide (37KB)
│   ├── build-windows.md        #   Windows build instructions
│   ├── build-unix.md           #   Unix build instructions
│   ├── build-osx.md            #   macOS build instructions
│   ├── bch-upgrades.md         #   BCH upgrade history
│   ├── bips.md                 #   BIP implementation status
│   ├── REST-interface.md       #   REST API docs
│   ├── zmq.md                  #   ZMQ integration
│   └── release-notes/          #   Per-version release notes
│
├── contrib/                    # ===== TOOLS & UTILITIES =====
│   ├── devtools/               #   Developer scripts
│   ├── gitian-descriptors/     #   Reproducible build configs
│   ├── linearize/              #   Blockchain linearization
│   ├── seeds/                  #   Network seed generation
│   └── init/                   #   System service files
│
└── share/                      # ===== STATIC DATA =====
    ├── examples/               #   Example config files
    ├── pixmaps/                #   Application icons
    └── rpcauth/                #   RPC auth helper
```

---

## 3. Executables & Entry Points

| Executable         | Entry Point              | Purpose                                                                |
|--------------------|--------------------------|------------------------------------------------------------------------|
| **bitcoind**       | `src/bitcoind.cpp`       | Full node daemon — validates blocks, relays transactions, serves RPC   |
| **bitcoin-qt**     | `src/qt/bitcoin.cpp`     | GUI application (Qt5) — same as bitcoind + graphical interface         |
| **bitcoin-cli**    | `src/bitcoin-cli.cpp`    | RPC client — sends JSON-RPC commands to a running node                 |
| **bitcoin-tx**     | `src/bitcoin-tx.cpp`     | Offline transaction tool — create, sign, decode transactions           |
| **bitcoin-wallet** | `src/bitcoin-wallet.cpp` | Offline wallet tool — create/inspect wallet files without a running node |
| **bitcoin-seeder** | `src/seeder/main.cpp`    | DNS seed crawler — discovers and serves peer addresses                 |

### Startup Sequence (bitcoind)

```
main()
  └→ AppInit()
       ├→ SetupServerArgs()           # Define all CLI arguments
       ├→ ParseParameters()           # Parse command line + bitcoin.conf
       ├→ AppInitBasicSetup()         # Signal handlers, logging init
       ├→ AppInitParameterInteraction()# Validate arg combinations
       ├→ AppInitSanityChecks()       # ECC init, glibc sanity
       ├→ AppInitLockDataDirectory()  # Lock datadir (prevent dual instances)
       └→ AppInitMain()               # === MAIN INITIALIZATION ===
            ├→ Initialize CScheduler + start scheduler thread
            ├→ Initialize CConnman (network manager)
            ├→ Initialize BanMan (peer ban tracking)
            ├→ Load block index from LevelDB
            ├→ Load UTXO set (chainstate)
            ├→ Start block loading thread
            ├→ Start wallet(s)
            ├→ Start RPC server + HTTP server
            ├→ Start P2P networking (listen + connect)
            └→ Enter main event loop
```

---

## 4. Internal Libraries (Build Targets)

These are static libraries built by CMake from `src/CMakeLists.txt`:

| Library Target       | Source Files                                                                                                               | Purpose                                                        |
|----------------------|----------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------|
| **server**           | validation, net, net_processing, init, miner, txmempool, rpc/*, httpserver, rest, index/*, policy/*, dsproof/*, zmq/*      | Full node server functionality                                 |
| **common**           | chainparams, coins, key, pubkey, key_io, cashaddr, base58, core_read, core_write, protocol, netaddress, netbase            | Shared code used by all executables                            |
| **util**             | util/system, util/strencodings, util/time, logging, random, fs, sync, compat/*, support/*                                  | Low-level utilities and platform abstractions                  |
| **bitcoinconsensus** | script/interpreter, script/script, script/bitcoinconsensus, primitives/*, consensus/*, crypto/*                            | **Shared library** (.dll/.so) — consensus rules only, embeddable |
| **wallet**           | wallet/*.cpp                                                                                                               | Wallet functionality (optional, can be disabled)               |
| **seeder**           | seeder/bitcoin, seeder/db, seeder/dns, seeder/util                                                                        | DNS seed crawler library                                       |

### Library Dependency Graph

```
bitcoind ──→ server ──→ common ──→ util
                │          │
                │          └──→ bitcoinconsensus ──→ secp256k1
                │                                    └──→ libgmp
                └──→ wallet (optional)
                └──→ leveldb
                └──→ univalue

bitcoin-cli ──→ common ──→ util
bitcoin-tx  ──→ common ──→ util ──→ bitcoinconsensus
bitcoin-qt  ──→ server + wallet + Qt5
```

---

## 5. Bundled Third-Party Libraries

These live inside the source tree and are built as part of the project:

| Library      | Location           | Version            | Purpose                                                          |
|--------------|--------------------|--------------------|------------------------------------------------------------------|
| **secp256k1** | `src/secp256k1/`  | Custom (BCHN fork) | Elliptic curve crypto (ECDSA, Schnorr, key recovery, multiset)   |
| **LevelDB**  | `src/leveldb/`     | Custom             | Key-value store for block index and UTXO set (chainstate)        |
| **UniValue** | `src/univalue/`    | Custom             | JSON parsing and generation for RPC                              |
| **ctaes**    | `src/crypto/ctaes/` | —                 | Constant-time AES implementation for wallet encryption           |

---

## 6. External Dependencies

### Core (Required)

| Library      | Version  | Purpose                                              | CMake Module      |
|--------------|----------|------------------------------------------------------|-------------------|
| **Boost**    | 1.73+    | Filesystem, chrono, test framework                   | System            |
| **OpenSSL**  | 1.0.1k+  | TLS for RPC, random number generation                | System            |
| **libevent** | 2.0.22+  | HTTP server, async I/O for RPC                       | `FindEvent.cmake` |
| **libgmp**   | 6.3.0    | Arbitrary precision arithmetic (BigInt script operations) | `FindGMP.cmake`   |
| **zlib**     | —        | Compression (test data)                              | System            |

### Optional

| Library        | Version        | Purpose                               | Build Flag                    |
|----------------|----------------|---------------------------------------|-------------------------------|
| **BerkeleyDB** | 5.3.28         | Wallet database storage               | `BUILD_BITCOIN_WALLET`        |
| **Qt**         | 5.15.16        | GUI application                       | `BUILD_BITCOIN_QT`            |
| **ZeroMQ**     | 4.3.4          | Real-time notifications (blocks, txs) | `BUILD_BITCOIN_ZMQ`           |
| **MiniUPnPc**  | 2.0.20180203   | UPnP port mapping                     | `ENABLE_UPNP`                 |
| **libnatpmp**  | —              | NAT-PMP port mapping                  | `ENABLE_NATPMP`               |
| **QREncode**   | 3.4.4          | QR code generation (GUI)              | With Qt                       |
| **jemalloc**   | 5.2.1          | High-performance memory allocator     | `USE_JEMALLOC`                |
| **RapidCheck** | —              | Property-based testing                | `ENABLE_PROPERTY_BASED_TESTS` |

### Depends System

The `depends/` directory provides a cross-compilation system that builds all external dependencies from source with deterministic settings. Package definitions are in `depends/packages/*.mk` (32 packages total). Build with:

```bash
cd depends && make HOST=x86_64-w64-mingw32  # Windows cross-compile
cd depends && make                           # Native build
```

---

## 7. Core Subsystems & Data Flow

### Block Processing Pipeline

```
Peer sends BLOCK message
  │
  ▼
CConnman::SocketHandler (reads from socket into vRecv buffer)
  │
  ▼
PeerLogicValidation::ProcessMessages (deserializes CBlock)
  │
  ▼
AcceptBlockHeader()
  ├→ Validate PoW (proof-of-work)
  ├→ Check timestamp, difficulty
  ├→ Verify not in failed block set
  └→ Add to mapBlockIndex
  │
  ▼
AcceptBlock()
  ├→ Check tx count, size, merkle root
  └→ Write block to disk (blk?????.dat)
  │
  ▼
ActivateBestChain()
  │
  ▼
ConnectBlock()
  ├→ For each transaction:
  │   ├→ Validate inputs exist in UTXO set
  │   ├→ Check amounts (inputs >= outputs)
  │   ├→ Verify scripts (parallel via CheckQueue workers)
  │   └→ Update UTXO set (spend inputs, create outputs)
  ├→ Update chain work, chain tx count
  └→ Write undo data (rev?????.dat)
  │
  ▼
CValidationInterface signals:
  ├→ BlockConnected      → Wallet, TxIndex, UI
  ├→ UpdatedBlockTip     → Peers, miners
  └→ NewPoWValidBlock    → Compact block relay
```

### Transaction Flow

```
A) FROM NETWORK:                    B) FROM RPC:
   Peer sends TX message               sendrawtransaction / sendtoaddress
        │                                      │
        ▼                                      ▼
   ProcessTxMessage              ───→ AcceptToMemoryPool() ←───
        │                              │
        ▼                              ▼
   Validation checks:            ┌──────────────────────┐
   • Syntax (well-formed)        │  CTxMemPool          │
   • Script verification         │  .addUnchecked()     │
   • UTXO availability           │                      │
   • Fee checks (meets minimum)  │  Indexes:            │
   • Size/sigcheck limits        │  • By hash (txid)    │
        │                        │  • By fee rate       │
        ▼                        │  • By insertion time │
   g_mempool.addUnchecked()      └──────────────────────┘
        │
        ▼
   CValidationInterface::TransactionAddedToMempool
        │
        ▼
   Relay to peers via INV message
```

### Reorg (Chain Reorganization)

```
Current chain:  ... → A → B → C (tip)
New chain:      ... → A → B' → C' → D' (more work)

ActivateBestChain() detects D' has more chain work:
  1. DisconnectBlock(C)  → restore spent UTXOs from rev file
  2. DisconnectBlock(B)  → restore spent UTXOs from rev file
  3. ConnectBlock(B')    → validate and apply
  4. ConnectBlock(C')    → validate and apply
  5. ConnectBlock(D')    → validate and apply → new tip

Disconnected txs (from B, C) re-enter mempool if still valid
```

---

## 8. Key Classes & Relationships

### Data Structures

| Class                  | File                       | Purpose                                                                    |
|------------------------|----------------------------|----------------------------------------------------------------------------|
| `CBlockHeader`         | `primitives/block.h`       | 80-byte block header (version, prevHash, merkleRoot, time, bits, nonce)    |
| `CBlock`               | `primitives/block.h`       | Full block = header + vector of transactions                               |
| `CBlockIndex`          | `chain.h`                  | Block header metadata in memory (height, chain work, disk position, validity) |
| `CChain`               | `chain.h`                  | Active best chain — vector of CBlockIndex* indexed by height               |
| `CTransaction`         | `primitives/transaction.h` | Immutable transaction (inputs, outputs, locktime). Cached hash.            |
| `CMutableTransaction`  | `primitives/transaction.h` | Mutable version for building transactions                                  |
| `CTxIn`                | `primitives/transaction.h` | Transaction input (prevout reference + scriptSig + sequence)               |
| `CTxOut`               | `primitives/transaction.h` | Transaction output (amount + scriptPubKey + optional token data)           |
| `COutPoint`            | `primitives/transaction.h` | Reference to a specific output: (txid, index) pair                         |
| `Coin`                 | `coins.h`                  | Single UTXO: the output + height + coinbase flag                           |
| `CScript`              | `script/script.h`          | Bitcoin script bytecode (vector<uint8_t> with push/pop helpers)            |
| `uint256`              | `uint256.h`                | 256-bit unsigned integer (used for hashes)                                 |
| `Amount`               | `amount.h`                 | Monetary value in satoshis                                                 |

### Managers & State

| Class                   | File               | Purpose                                                                                  |
|-------------------------|--------------------|-------------------------------------------------------------------------------------------------|
| `CChainState`           | `validation.cpp`   | Central blockchain state machine. Owns CChain, manages UTXO set, validates blocks.              |
| `CTxMemPool`            | `txmempool.h`      | Unconfirmed transaction pool. Boost multi_index with hash/fee/time indexes.                      |
| `CConnman`              | `net.h`            | Network connection manager. Owns all CNode instances.                                            |
| `CNode`                 | `net.h`            | Single peer connection. Socket, message queues, inventory tracking.                              |
| `PeerLogicValidation`   | `net_processing.h` | P2P message dispatcher. Implements NetEventsInterface + CValidationInterface.                    |
| `BanMan`                | `banman.h`         | Peer ban/discourage tracking.                                                                    |
| `CWallet`               | `wallet/wallet.h`  | Key management, balance tracking, transaction building.                                          |
| `CScheduler`            | `scheduler.h`      | Background task scheduler with periodic/delayed execution.                                       |
| `ArgsManager`           | `util/system.h`    | Command-line and config file argument parser. Global `gArgs` instance.                           |

### Class Relationship Diagram

```
                              ┌──────────────┐
                              │  bitcoind    │
                              │  main()      │
                              └──────┬───────┘
                                     │
                  ┌──────────────────┼──────────────────┐
                  │                  │                  │
          ┌───────▼───────┐  ┌───────▼───────┐  ┌───────▼───────┐
          │  CConnman     │  │  CChainState  │  │  CWallet      │
          │  (network)    │  │  (consensus)  │  │  (keys/funds) │
          └───────┬───────┘  └───────┬───────┘  └───────────────┘
                  │                  │
       ┌──────────▼──────┐           │
       │  CNode (peer)   │           │
       └──────┬──────────┘           │
              │                      │
      ┌───────▼────────────┐         │
      │PeerLogicValidation │         │
      │(message dispatch)  │         │
      └───────┬────────────┘         │
              │                      │
              └────────┬─────────────┘
                       │
              ┌────────▼────────┐
              │  CTxMemPool     │
              │  (unconfirmed)  │
              └────────┬────────┘
                       │
              ┌────────▼────────┐
              │  CCoinsView     │
              │  Cache → DB     │
              │  (UTXO set)     │
              └────────┬────────┘
                       │
              ┌────────▼────────┐
              │  CScript +      │
              │  Interpreter    │
              │  (validation)   │
              └─────────────────┘

    ┌───────────────────────────────┐
    │  CValidationInterface         │
    │  (observer pattern signals)   │
    │  - BlockConnected             │
    │  - TransactionAddedToMempool  │
    │  - UpdatedBlockTip            │
    └───────────────────────────────┘
```

---

## 9. Threading Model

| Thread            | Created In       | Purpose                                                             |
|-------------------|------------------|---------------------------------------------------------------------|
| **main**          | `bitcoind.cpp`   | Initialization, then waits for shutdown signal                      |
| **scheduler**     | `init.cpp`       | Runs periodic tasks (mempool expiry, peer mgmt, DB flush)           |
| **loadblk**       | `init.cpp`       | Loads blockchain from disk during startup                           |
| **msghand**       | `CConnman`       | Processes incoming P2P messages (calls ProcessMessages/SendMessages) |
| **net**           | `CConnman`       | Socket I/O — reads/writes to peer sockets via select()              |
| **opencon**       | `CConnman`       | Opens new outbound connections                                      |
| **addcon**        | `CConnman`       | Connects to manually added peers (-addnode)                         |
| **dnsseed**       | `CConnman`       | DNS seed resolution for peer discovery                              |
| **http**          | `httpserver.cpp` | libevent HTTP server for RPC                                        |
| **httpworker[N]** | `httpserver.cpp` | RPC request processing worker threads                               |
| **scriptch[N]**   | `init.cpp`       | Script verification workers (0–255, configurable via `-par`)        |
| **torcontrol**    | `torcontrol.cpp` | Tor SOCKS proxy management                                         |

### Key Synchronization Primitives

| Mutex                | Protects                               | Scope                    |
|----------------------|----------------------------------------|--------------------------|
| `cs_main`            | CChainState, mapBlockIndex, UTXO views | Global — the "big lock"  |
| `CTxMemPool::cs`     | Mempool state                          | Per-mempool              |
| `CNode::cs_vSend`    | Outgoing message queue                 | Per-peer                 |
| `CNode::cs_vRecv`    | Incoming message buffer                | Per-peer                 |
| `CWallet::cs_wallet` | Wallet state                           | Per-wallet               |
| `g_best_block_mutex` | Best block notification                | Global                   |

---

## 10. Storage Layer

### On-Disk Layout

```
<datadir>/                           # Default: ~/.bitcoin/ (Linux), %APPDATA%\Bitcoin (Windows)
├── bitcoin.conf                     # Configuration file
├── .cookie                          # RPC auth cookie
├── peers.dat                        # Known peer addresses
├── banlist.dat                      # Banned peer list
├── fee_estimates.dat                # Fee estimation data
├── mempool.dat                      # Persisted mempool (-persistmempool)
│
├── blocks/
│   ├── blk00000.dat                 # Raw block data (sequential, ~128MB each)
│   ├── blk00001.dat
│   ├── ...
│   ├── rev00000.dat                 # Undo data (for reorgs, parallel to blk files)
│   ├── rev00001.dat
│   └── index/                       # LevelDB: CBlockIndex → disk position mapping
│       ├── 000003.log
│       ├── CURRENT
│       └── MANIFEST-000002
│
├── chainstate/                      # LevelDB: UTXO set (all unspent outputs)
│   ├── 000003.log
│   ├── CURRENT
│   └── MANIFEST-000002
│
└── indexes/                         # Optional indexes
    └── txindex/                     # LevelDB: txid → (block, position)
```

### Storage Classes

| Class            | File               | Storage                      | Purpose                                    |
|------------------|--------------------|------------------------------|--------------------------------------------|
| `FlatFileSeq`    | `flatfile.h`       | blk?????.dat, rev?????.dat   | Sequential block and undo data             |
| `CBlockTreeDB`   | `txdb.h`           | blocks/index/ (LevelDB)     | Block header index                         |
| `CCoinsViewDB`   | `txdb.h`           | chainstate/ (LevelDB)       | UTXO set on disk                           |
| `CCoinsViewCache` | `coins.h`         | In-memory                    | Cached UTXO layer over CCoinsViewDB       |
| `CDBWrapper`     | `dbwrapper.h`      | LevelDB                     | Generic LevelDB wrapper with obfuscation   |
| `CWalletDB`      | `wallet/walletdb.h` | wallet.dat (BerkeleyDB)     | Wallet key/transaction storage             |

### Flush Schedule

| Operation            | Interval      | Controlled By              |
|----------------------|---------------|----------------------------|
| UTXO cache → disk    | Every 1 hour  | `DATABASE_WRITE_INTERVAL`  |
| Full database flush  | Every 24 hours | `DATABASE_FLUSH_INTERVAL` |
| Mempool persist      | On shutdown   | `-persistmempool` flag     |

---

## 11. Network Protocol

### Message Format

```
┌──────────────┬──────────────┬──────────────┬──────────────┐
│  Magic (4B)  │ Command (12B)│  Size (4B)   │ Checksum (4B)│
│  0xe3e1f3e8  │ "version\0\0"│  payload len │ sha256d[:4]  │
├──────────────┴──────────────┴──────────────┴──────────────┤
│                      Payload (variable)                   │
└───────────────────────────────────────────────────────────┘
```

### Key Message Types

| Category            | Messages                                       | Purpose                          |
|---------------------|------------------------------------------------|----------------------------------|
| **Handshake**       | VERSION, VERACK                                | Establish connection, negotiate features |
| **Address**         | ADDR, ADDRV2, SENDADDRV2, GETADDR              | Peer discovery and gossip        |
| **Inventory**       | INV, GETDATA                                   | Announce/request blocks and transactions |
| **Blocks**          | BLOCK, HEADERS, GETBLOCKS, GETHEADERS          | Block distribution               |
| **Compact Blocks**  | CMPCTBLOCK, BLOCKTXN, GETBLOCKTXN              | BIP152 efficient block relay     |
| **Transactions**    | TX, MEMPOOL                                    | Transaction distribution         |
| **Fees**            | FEEFILTER                                      | Minimum fee rate for relay       |
| **Keepalive**       | PING, PONG                                     | Connection liveness              |
| **Filtering**       | FILTERLOAD, FILTERADD, FILTERCLEAR, MERKLEBLOCK | SPV bloom filters               |

### Connection Limits

| Parameter                    | Default | Purpose                        |
|------------------------------|---------|--------------------------------|
| `MAX_PEER_CONNECTIONS`       | 125     | Total peer connections         |
| `MAX_OUTBOUND_CONNECTIONS`   | 11      | Outbound connections           |
| `MAX_PROTOCOL_MESSAGE_LENGTH` | 2 MB   | Max message payload            |
| `MAX_INV_SZ`                 | 50,000  | Max inventory items per message |
| `TIMEOUT_INTERVAL`           | 20 min  | Disconnect unresponsive peers  |
| `DEFAULT_BANSCORE_THRESHOLD` | 100     | Misbehavior score to trigger ban |

---

## 12. Script Engine & VM

### Opcode Categories

| Category           | Examples                                          | Notes                          |
|--------------------|---------------------------------------------------|--------------------------------|
| **Constants**      | OP_0, OP_1..OP_16, OP_PUSHDATA1/2/4              | Push data onto stack           |
| **Flow Control**   | OP_IF, OP_ELSE, OP_ENDIF, OP_VERIFY              | Conditional execution          |
| **Stack**          | OP_DUP, OP_DROP, OP_SWAP, OP_PICK, OP_ROLL       | Stack manipulation             |
| **Arithmetic**     | OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD           | BigInt arithmetic              |
| **Bitwise**        | OP_AND, OP_OR, OP_XOR, OP_INVERT                 | Bitwise operations             |
| **Comparison**     | OP_EQUAL, OP_LESSTHAN, OP_GREATERTHAN            | Value comparison               |
| **Crypto**         | OP_HASH160, OP_HASH256, OP_CHECKSIG, OP_CHECKMULTISIG | Hash and signature ops    |
| **Introspection**  | OP_TXINPUTCOUNT, OP_UTXOVALUE, etc.              | Native introspection (BCH)     |
| **Loops**          | OP_BEGIN, OP_UNTIL                                | Loops (May 2026 upgrade)       |
| **Functions**      | OP_DEFINE, OP_INVOKE                              | Functions (May 2026 upgrade)   |

### Script Execution Flow

```
VerifyScript(scriptSig, scriptPubKey, flags, checker)
  │
  ├→ EvalScript(scriptSig)              # Execute unlocking script
  │     └→ Builds stack
  │
  ├→ EvalScript(scriptPubKey, stack)     # Execute locking script with stack from above
  │     └→ Must leave TRUE on stack
  │
  └→ If P2SH:
       ├→ Deserialize redeemScript from scriptSig
       └→ EvalScript(redeemScript, stack) # Execute redeem script
```

### Key Script Classes

| Class                           | File                     | Purpose                              |
|---------------------------------|--------------------------|--------------------------------------|
| `CScript`                       | `script/script.h`        | Script bytecode container            |
| `BaseSignatureChecker`          | `script/interpreter.h`   | Abstract signature verification      |
| `TransactionSignatureChecker`   | `script/interpreter.h`   | Verifies sigs against transaction    |
| `PrecomputedTransactionData`    | `script/interpreter.h`   | Cached sighash data for efficiency   |
| `ScriptExecutionMetrics`        | `script/script_metrics.h` | Tracks opcount, stack usage         |

---

## 13. Signal / Notification System

`CValidationInterface` (defined in `validationinterface.h`) implements an observer pattern for validation events. Components register as subscribers and receive callbacks when state changes:

| Signal                           | When Fired                      | Subscribers                        |
|----------------------------------|---------------------------------|------------------------------------|
| `UpdatedBlockTip`                | Active chain tip changed        | Miners, peer logic, UI             |
| `BlockConnected`                 | Block added to active chain     | Wallet, TxIndex, CoinStatsIndex, UI |
| `BlockDisconnected`              | Block removed (reorg)           | Wallet, TxIndex                    |
| `TransactionAddedToMempool`      | Tx entered mempool              | Wallet, UI                         |
| `TransactionRemovedFromMempool`  | Tx evicted/mined                | Wallet                             |
| `TransactionDoubleSpent`         | Double-spend proof received     | Wallet, relay                      |
| `NewPoWValidBlock`               | New valid block for miners      | Compact block relay                |

All callbacks are delivered via `SingleThreadedSchedulerClient` to ensure sequential, thread-safe processing.

---

## 14. Configuration System

### ArgsManager (`util/system.h`)

Global instance: `gArgs`

```cpp
gArgs.GetArg("-maxconnections", DEFAULT_MAX_PEER_CONNECTIONS);
gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX);
gArgs.IsArgSet("-blocksonly");
```

### Configuration Sources (priority order)
1. Command-line arguments (highest)
2. `bitcoin.conf` file
3. Compiled defaults (lowest)

### Network Selection

| Flag         | Network  | Default Port | Magic Bytes    |
|--------------|----------|--------------|----------------|
| (default)    | mainnet  | 8333         | `0xe3e1f3e8`   |
| `-testnet`   | testnet3 | 18333        | `0xf4e5f3f4`   |
| `-testnet4`  | testnet4 | 28333        | —              |
| `-scalenet`  | scalenet | 38333        | —              |
| `-chipnet`   | chipnet  | 48333        | —              |
| `-regtest`   | regtest  | 18444        | `0xdab5bffa`   |

---

## 15. Logging System

### Logger (`logging.h`)

Categories (enabled via `-debug=<category>`):

| Category   | What It Logs                |
|------------|-----------------------------|
| `net`      | Network connections, messages |
| `tor`      | Tor proxy operations        |
| `mempool`  | Mempool additions/removals  |
| `http`     | HTTP server requests        |
| `bench`    | Performance timing          |
| `zmq`      | ZeroMQ notifications        |
| `db`       | Database operations         |
| `rpc`      | RPC calls                   |
| `addrman`  | Address manager             |
| `coindb`   | UTXO database               |
| `leveldb`  | LevelDB internals           |
| `dsproof`  | Double-spend proofs         |
| `abla`     | Adaptive block limit        |

### Log Rate Limiting

- Per source location budget: **1MB per hour** (`RATELIMIT_MAX_BYTES`)
- Prevents log flooding from hot paths
- `LogPrintf()` is rate-limited; `LogPrintfNoRateLimit()` bypasses

### File Management

- Output: `<datadir>/debug.log`
- Auto-shrink at **11MB** (keeps only recent entries)
- Optional: timestamps, microseconds, thread names

---

## 16. Test Infrastructure

### Test Layers

| Layer                | Location           | Framework        | What It Tests                            |
|----------------------|--------------------|------------------|------------------------------------------|
| **Unit tests**       | `src/test/`        | Boost.Test       | Individual classes and functions          |
| **Functional tests** | `test/functional/` | Python           | Full-node integration (RPC, P2P, wallet) |
| **Fuzz tests**       | `test/fuzz/`       | libFuzzer/AFL    | Input parsing, deserialization            |
| **Benchmarks**       | `src/bench/`       | Custom framework | Performance measurement                  |
| **Property tests**   | `src/test/`        | RapidCheck       | Property-based testing (optional)        |

### Running Tests

```bash
# Unit tests (native Linux build)
ninja check-bitcoin              # All unit tests
./src/test/test_bitcoin --run_test=blockchain_tests  # Specific suite

# Functional tests
python test/functional/test_runner.py          # All functional tests
python test/functional/rpc_blockchain.py       # Specific test

# Benchmarks
./src/bench/bench_bitcoin
```

### Running Unit Tests for Windows Cross-Compiled Builds

When cross-compiling for Windows from WSL2, `ninja check` will not work
because it tries to execute .exe files on Linux. Instead, build the test
executable and run it on Windows:

```bash
# In WSL2 build directory:
ninja test_bitcoin.exe

# Copy to Windows (requires libbitcoinconsensus-29.dll alongside it):
cp src/test/test_bitcoin.exe /mnt/f/.../vbchn-build-output/
cp src/libbitcoinconsensus-29.dll /mnt/f/.../vbchn-build-output/
```

```bash
# On Windows:
test_bitcoin.exe --log_level=test_suite     # All tests, suite-level output
test_bitcoin.exe --run_test=net_tests       # Specific suite
```

**Status:** 679 test cases, all passing (verified 2026-02-13).

### Key Test Files

| Test File                          | Size  | Tests                            |
|------------------------------------|-------|----------------------------------|
| `script_tests.cpp`                 | 165KB | Script interpreter correctness   |
| `bigint_script_property_tests.cpp` | 165KB | BigInt property-based tests      |
| `token_tests.cpp`                  | 121KB | CashToken validation             |
| `transaction_tests.cpp`            | 55KB  | Transaction validation           |
| `vmlimits_tests.cpp`               | 56KB  | VM resource limit enforcement    |

---

## 17. Build System Details

### CMake Configuration

The root `CMakeLists.txt` sets:
- **C++ Standard:** C++20
- **Minimum CMake:** 3.16
- **Hardening:** `-fstack-protector-all`, `-D_FORTIFY_SOURCE=2`, PIE, RELRO

### Key Build Options

| Option                      | Default | Purpose                                    |
|-----------------------------|---------|--------------------------------------------|
| `BUILD_BITCOIN_DAEMON`      | ON      | Build bitcoind                             |
| `BUILD_BITCOIN_QT`          | ON      | Build bitcoin-qt (requires Qt5)            |
| `BUILD_BITCOIN_CLI`         | ON      | Build bitcoin-cli                          |
| `BUILD_BITCOIN_TX`          | ON      | Build bitcoin-tx                           |
| `BUILD_BITCOIN_WALLET`      | ON      | Build wallet support (requires BDB)        |
| `BUILD_BITCOIN_ZMQ`         | ON      | Build ZeroMQ support                       |
| `BUILD_BITCOIN_SEEDER`      | ON      | Build DNS seeder                           |
| `ENABLE_UPNP`               | ON      | UPnP port mapping                          |
| `ENABLE_NATPMP`             | OFF     | NAT-PMP port mapping                       |
| `USE_JEMALLOC_EXPERIMENTAL` | OFF     | jemalloc allocator (experimental)          |
| `ENABLE_HARDENING`          | ON      | Compiler hardening flags                   |
| `ENABLE_SANITIZERS`         | —       | ASan, TSan, UBSan                          |
| `ENABLE_TEST`               | ON      | Build test suite                           |

### Build Commands (typical)

```bash
mkdir build && cd build
cmake -GNinja .. -DBUILD_BITCOIN_QT=OFF  # Configure without GUI
ninja                                      # Build
ninja check                                # Run tests
```

---

## 18. Windows Build Notes

VBCHN is cross-compiled for Windows using MinGW-w64 from a WSL2 Ubuntu 24.04 host. The `doc/build-windows.md` documents the general approach. Key points:

### Cross-Compilation (Recommended)
```bash
# On Ubuntu/Debian:
sudo apt install g++-mingw-w64-x86-64
cd depends && make HOST=x86_64-w64-mingw32
cd .. && mkdir build && cd build
cmake -GNinja .. -DCMAKE_TOOLCHAIN_FILE=../cmake/platforms/Win64.cmake
ninja
```

### Native Windows Build (MSYS2)
```bash
# Install MSYS2, then in MSYS2 MinGW64 shell:
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake ninja
pacman -S mingw-w64-x86_64-boost mingw-w64-x86_64-openssl mingw-w64-x86_64-libevent
pacman -S mingw-w64-x86_64-gmp mingw-w64-x86_64-zeromq mingw-w64-x86_64-qt5
mkdir build && cd build
cmake -GNinja ..
ninja
```

### Windows-Specific Code

The codebase has Windows-specific handling in:
- `src/compat/` — POSIX compatibility shims
- `cmake/modules/FindSHLWAPI.cmake` — Windows shell API
- `cmake/modules/NSIS.template.in` — Windows installer (NSIS)
- Root `CMakeLists.txt` lines 99+ — Windows-specific compiler flags (`-mwindows`, etc.)

### CRLF Line Ending Issues

When syncing source files from Windows (`/mnt/f/...`) to the WSL2 build tree,
files may have CRLF line endings. This breaks CMake's `configure_file()` —
particularly the version variables in `src/config/CMakeLists.txt` — causing
build errors like `integer overflow` or `expected primary-expression before ';'`.

**Fix:** Strip CRLF after every copy:
```bash
sed -i 's/\r$//' <file>
```

The `rebuild-vbchn.sh` script handles this automatically for all tracked files.

### Rebuild Script

`~/rebuild-vbchn.sh` (WSL2) automates incremental rebuilds:
1. Checks out clean source
2. Copies modified files from Windows and strips CRLF
3. Runs cmake configure + ninja build

Invoke from Windows: `MSYS_NO_PATHCONV=1 wsl -d Ubuntu-24.04 /home/phil/rebuild-vbchn.sh`

### Windows Runtime Considerations
- Data directory: `F:\Bitcoin\Bitcoin-Cash-Node-Data-Directory` (custom via `-datadir`)
- Uses Windows Sockets (Winsock2)
- MinGW runtime links against msvcrt.dll
- NSIS installer available for distribution

---

### Build Output Location

Built executables are copied to `F:\ClaudeProjects\cryptocurrency\vbchn-build-output\`:
- `bitcoind.exe` — Full node daemon
- `bitcoin-qt.exe` — GUI application
- `bitcoin-cli.exe` — RPC client
- `bitcoin-tx.exe` — Transaction tool
- `bitcoin-wallet.exe` — Wallet tool
- `test_bitcoin.exe` — Unit test executable
- `libbitcoinconsensus-29.dll` — Required by test_bitcoin.exe

---

## 19. Linting & Code Style Infrastructure

VBCHN inherits a comprehensive linting system from BCHN covering style and correctness,
but has no structural enforcement (function/file/class length limits).

### Existing Tools

| Tool / Config               | Target   | What It Checks                                                       |
|-----------------------------|----------|----------------------------------------------------------------------|
| `src/.clang-format`         | C++      | Formatting: 120-char columns, 4-space indent, brace style            |
| `src/.clang-tidy`           | C++      | `boost-use-to-string`, `readability-braces-around-statements`        |
| `.arclint`                  | All      | 20+ linters via Arcanist (clang-format, cppcheck, shellcheck, etc.)  |
| `.arcconfig`                | All      | Phabricator/Arcanist project config                                  |
| `.gitlab-ci.yml`            | CI       | Runs `arc lint` and `ninja check-lint`                               |
| `test/lint/` (31+ scripts)  | All      | Circular deps, include guards, format strings, unicode, etc.         |
| `flake8` + `mypy`           | Python   | Style, type checking (config in `.static-checks-python-requirements.txt`) |
| `shellcheck`                | Shell    | Shell script analysis                                                |
| `autopep8`                  | Python   | Auto-formatting (max line length 132)                                |

### Key Lint Scripts (`test/lint/`)

| Script                                     | Purpose                                    |
|--------------------------------------------|--------------------------------------------|
| `lint-cpp.sh`                              | Pattern-based C++ checks                   |
| `lint-assertions.sh`                       | Assertion validation                       |
| `lint-circular-dependencies.sh`            | Circular dependency detection              |
| `lint-format-strings.sh`                   | printf/sprintf format string validation    |
| `lint-include-guards.sh`                   | Header include guard validation            |
| `lint-includes.sh`                         | Include relationship validation            |
| `lint-python.sh`                           | Python linting (flake8 + mypy)             |
| `lint-python-mutable-default-parameters.sh`| Mutable default argument detection         |
| `lint-shell-shebang.sh`                    | Bash shebang validation                    |
| `lint-shell-locale.sh`                     | Locale-dependent code detection            |
| `check-doc.py`                             | Command-line option documentation check    |
| `check-rpc-mappings.py`                    | RPC method mapping validation              |
| `lint-unicode.sh`                          | Unicode handling validation                |

### Third-Party Exclusions

All linters exclude these embedded third-party directories:

- `src/secp256k1/` — ECDSA & Schnorr signatures (MIT)
- `src/leveldb/` — Key-value database (BSD)
- `src/univalue/` — JSON support (MIT)
- `src/crypto/ctaes/` — Constant-time AES (MIT)

### What Does NOT Exist (gap)

- No maximum function length enforcement
- No maximum file length enforcement
- No maximum class size limits
- No cyclomatic complexity checks

---

## 20. Source Code Size Analysis

Project-owned files only (excludes secp256k1, leveldb, univalue, ctaes).

### Summary

| Metric       | Value    |
|--------------|----------|
| Total files  | 749      |
| Total lines  | 184,915  |

### File Size Distribution

| Range             | Count | %     |
|-------------------|-------|-------|
| >5000 lines       | 1     | 0.1%  |
| 3001–5000 lines   | 4     | 0.5%  |
| 2001–3000 lines   | 6     | 0.8%  |
| 1001–2000 lines   | 18    | 2.4%  |
| 501–1000 lines    | 62    | 8.3%  |
| 201–500 lines     | 146   | 19.5% |
| <=200 lines       | 512   | 68.4% |

### Top 30 Largest Files

| Lines   | File                                   | Category                |
|---------|----------------------------------------|-------------------------|
| 5,253   | `validation.cpp`                       | Core consensus          |
| 4,606   | `net_processing.cpp`                   | Network message handling |
| 4,303   | `wallet/wallet.cpp`                    | Wallet logic            |
| 4,013   | `wallet/rpcwallet.cpp`                 | Wallet RPC commands     |
| 3,085   | `test/script_tests.cpp`                | Test data/cases         |
| 2,942   | `test/bigint_script_property_tests.cpp`| Test data/cases         |
| 2,803   | `rpc/blockchain.cpp`                   | Blockchain RPC commands |
| 2,696   | `init.cpp`                             | Node startup/shutdown   |
| 2,648   | `net.cpp`                              | Network connections     |
| 2,531   | `script/interpreter.cpp`               | Script VM execution     |
| 2,150   | `test/util_tests.cpp`                  | Test cases              |
| 1,885   | `rpc/rawtransaction.cpp`               | Raw TX RPC commands     |
| 1,863   | `test/token_tests.cpp`                 | Test cases              |
| 1,781   | `test/bigint_tests.cpp`                | Test cases              |
| 1,441   | `crypto/sha256_sse4.cpp`               | SSE4 assembly (special) |
| 1,389   | `qt/bitcoingui.cpp`                    | GUI main window         |
| 1,375   | `wallet/rpcdump.cpp`                   | Wallet import/export    |
| 1,322   | `wallet/wallet.h`                      | Wallet header (classes) |
| 1,305   | `rpc/mining.cpp`                       | Mining RPC commands     |
| 1,303   | `util/system.cpp`                      | System utilities        |
| 1,286   | `qt/rpcconsole.cpp`                    | GUI RPC console         |
| 1,209   | `test/scriptnum_tests.cpp`             | Test cases              |
| 1,142   | `txmempool.cpp`                        | Transaction mempool     |
| 1,077   | `script/script.h`                      | Script types/classes    |
| 1,075   | `chainparams.cpp`                      | Chain parameters        |
| 1,065   | `serialize.h`                          | Serialization framework |
| 1,058   | `test/bloom_tests.cpp`                 | Test cases              |
| 1,007   | `tinyformat.h`                         | Third-party header      |
| 1,006   | `test/transaction_tests.cpp`           | Test cases              |
| 991     | `netaddress.cpp`                       | Network address handling |

### Headers With Most Class/Struct Definitions

| File                         | Lines | Types | Key Classes                                              |
|------------------------------|-------|-------|----------------------------------------------------------|
| `wallet/wallet.h`           | 1,322 | 12    | CWallet, CWalletTx, CMerkleTx, CKeyPool, COutput, ...   |
| `txmempool.h`               | 776   | 12    | CTxMemPool, CTxMemPoolEntry, CCoinsViewMemPool, ...      |
| `script/script.h`           | 1,077 | 10    | CScript, CScriptNum, ScriptInt, ScriptBigInt, ...        |
| `net.h`                     | 858   | 9     | CConnman, CNode, CNetMessage, CNodeStats, ...            |
| `streams.h`                 | 724   | 8     | CDataStream, CAutoFile, CBufferedFile, BufferedReader, ...|
| `primitives/transaction.h`  | —     | 6     | COutPoint, CTxIn, CTxOut, CTransaction, ...              |
| `serialize.h`               | 1,065 | 5     | CSizeComputer, CompactSizeFormatter, DefaultFormatter, ...|
| `chain.h`                   | —     | 5     | CBlockIndex, CDiskBlockIndex, CChain, BlockHasher, ...   |
| `rpc/server.h`              | —     | 5     | RPCServer, RPCTimerBase, CRPCTable, ...                  |
| `validation.h`              | 727   | 5     | CheckInputsLimiter, CScriptCheck, CVerifyDB, ...        |

### Refactoring Prioritization

**Phase 1 — Easy Wins (no logic changes):**
RPC files (`rpc/blockchain.cpp`, `rpc/rawtransaction.cpp`, `rpc/mining.cpp`,
`wallet/rpcwallet.cpp`) contain self-contained command functions that can be
split into separate files per command group with zero risk. Large test files
can similarly be split by test suite.

**Phase 2 — Header Decomposition:**
Split oversized headers: `wallet/wallet.h` (12 types) into `wallet/keypool.h`,
`wallet/wallettx.h`, `wallet/output.h`, etc. Split `script/script.h` (10 types)
into `script/scriptnum.h`, `script/bignum.h`. Split `net.h` (9 types) to
extract `CNode` into `net_node.h`.

**Phase 3 — Core File Decomposition (careful refactoring):**
`validation.cpp` (5,253 lines) — split block validation, UTXO management, chain state.
`net_processing.cpp` (4,606 lines) — split message handlers by message type.
`wallet/wallet.cpp` (4,303 lines) — split CWallet methods by concern (coin selection,
signing, sync). `init.cpp` (2,696 lines) — split startup steps into modules.

---

*Document generated from VBCHN v29.0.1 source analysis — February 2026*
