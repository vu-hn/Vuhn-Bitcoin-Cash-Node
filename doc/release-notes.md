# Release Notes for Bitcoin Cash Node version 29.0.0

Bitcoin Cash Node version 29.0.0 is now available from:

  <https://bitcoincashnode.org>

## Overview

This is a major release of Bitcoin Cash Node (BCHN) that implements the May 15, 2026 Network Upgrade. The upgrade
implements the following four consensus-level CHIP upgrades:

- [CHIP-2024-12 P2S: Pay to Script](https://github.com/bitjson/bch-p2s)
- [CHIP-2021-05-loops: Bounded Looping Operations](https://github.com/bitjson/bch-loops)
- [CHIP-2025-05 Functions: Function Definition and Invocation Operations](https://github.com/bitjson/bch-functions)
- [CHIP-2025-05 Bitwise: Re-Enable Bitwise Operations](https://github.com/bitjson/bch-bitwise)

Additionally, this version contains various other performance fixes, corrections, and improvements.

Users who are running any of our previous releases (v28.x.x or before) are urged to upgrade to v29.0.0 ahead of
May 15, 2026.

## Usage recommendations

Users who are running v28.0.1 or older are urged to upgrade to v29.0.0 in order to maintain compatibility with the
network.

## Network changes

- It's now possible to limit total bandwidth usage of the peer-to-peer network, on a per-peer basis. The CLI & conf
  argument `-peerratelimit` has been added for this purpose. See the built-in help (`bitcoind --help`) for more information.
- Optimizations were made to aspects of the network layer, reducing CPU utilization in high-txn rate scenarios.

## Added functionality

- A new "coin stats" index has been added, which may be enabled with the command-line option `-coinstatsindex`
  (default: disabled). If enabled, this index keeps track of various UTXO-set releated statistics for each block, which
  can be queried with the `gettxoutsetinfo` RPC.
- A new `patterns` flag has been added to the `getblock` and `getrawtransaction` RPC methods.
  - This is usable with all verbose outputs (>=2 for `getblock`, >=1 for `getrawtransaction`), and if set then all input
    and output scripts contain an additional key, `byteCodePattern` which is a JSON object that describes the script's
    pattern and "fingerprint". This is useful for categorizing the script in question. For more information on what is
    inside this key's object, see the RPC help for `getblock` and `getrawtransaction`, and [this discussion on Bitcoin Cash Research](https://bitcoincashresearch.org/t/smart-contract-fingerprinting-a-method-for-pattern-recognition-and-analysis-in-bitcoin-cash/1441).
  - If used with verbosity levels that include prevouts (`getblock` verbosity>=3 and `getrawtransaction` verbosity==2)
    and if the input is spending a P2SH prevout, then additionally there will a `redeemScript` object having its own `byteCodePattern`
    and additionally having a `p2shType` key.
  - A REST endpoint `/block/withpatterns/<HASH>` has been added which corresponds to `getblock` RPC with `verbosity=3 patterns=true`.
  - A REST endpoint `/tx/withpatterns/<HASH>` has been added which corresponds to `/tx/` with patterns added.
- Two new keys have been added to the results dictionary of the `getmempoolinfo` RPC method:
  - `permitbaremultisig` (true or false) indicates the node's `-permitbaremultisig` app-global setting.
  - `maxdatacarriersize` (numeric) indicates the node's `-datacarriersize=<n>` app-global setting.
- A new CLI argument `-peerratelimit` has been added, in order to optionally limit per-peer data transfer usage. This
  facility is off by default. See the built-in help for more information (`bitcoind --help`).

## Deprecated functionality

None

## Modified functionality

- The `gettxoutsetinfo` RPC has been modified and expanded greatly. Please see the RPC help for `gettxoutsetinfo` for
  full details but below is a summary of changes:
  - By default it now uses a new hashing algorithm, `hash_serialized_3` which is a better and more platform-neutral
    mechanism for hashing the current UTXO set.
  - Support for EC Multiset Hash (ECMH) as well as Core-compatbile MuHash3072 has been added to the RPC as well.
  - This RPC can now interoperate with the `coinstatsindex`. If enabled, the index allows `gettxoutsetinfo` to query
    UTXO set statistics for any block in the block chain.
- The `getblockchaininfo` RPC now has a new key `upgrade_status` which contains details about the next scheduled BCH
  network upgrade and its activation. See the RPC help for `getblockchaininfo` for more details.
- Non-debug logging to disk (but not to console) is now rate-limited. Each non-debug log message has a quota of 1MiB per
  hour. (A non-debug message is any log message that would be printed even if `-debug=0` were specified on the command
  line.) If any non-debug log messages have been suppressed due to this mechanism during any 1-hour period, then all
  subsequent log messages during that period will be prefixed with a `[*]` to indicate that log message suppression is
  active and that the debug.log file is potentially incomplete. An exception to this quota rule are the "UpdateTip"
  messages which are seen when new blocks are added to the blockchain, which are never suppressed or rate-limited.
- RPCs that are used to generate transactions that allow for NFT "commitments" now allow for empty strings to represent
  empty NFT commitments. Previously, to express an empty commitment, one needed to omit the "commitment" key altogether
  from the JSON object specifying the NFT (issue #538).

## Removed functionality

- The old hashing algorithm (`hash_serialized`) which was used by the `gettxoutsetinfo` algorithm has been removed
  in favor of a new algorithm `hash_serialized_3` which is used by default instead.
- Support for older macOS versions has been dropped. On Mac, the minimum supported version of macOS is macOS 13.3
  (Ventura) or newer.

## New RPC methods

None

## User interface changes

- By default, the `gettxoutsetinfo` now reports the UTXO set hash under the JSON object key `hash_serialized_3` rather
  than `hash_serialized`.
- For `getrawtransaction` at verbosity level 2, the JSON output now places the `sequence` field as the last
  object in the input, and the `fee` field as the first object after the `vout` array. This aligns with the unified
  transaction serialization used by `getblock`. The data content is unchanged, but clients relying on specific key
  ordering may need updates.
- The `getmempoolinfo` RPC now has two new keys: `permitbaremultisig` (boolean) and `maxdatacarriersize` (numeric).
- The `getblockchaininfo` RPC has a new key: `upgrade_status`, which itself is a dictionary with keys describing the
  upcoming network upgrade and when it will activate, or if it has already activated.

## Regressions

Bitcoin Cash Node 29.0.0 does not introduce any known regressions as compared to 28.0.1.

## Limitations

The following are limitations in this release of which users should be aware:

1. CashToken support is low-level at this stage. The wallet application does
   not yet keep track of the user's tokens.
   Tokens are only manageable via RPC commands currently.
   They only persist through the UTXO database and block database at this
   point.
   There are existing RPC commands to list and filter for tokens in the UTXO set.
   RPC raw transaction handling commands have been extended to allow creation
   (and sending) of token transactions.
   Interested users are advised to consult the functional test in
   `test/functional/bchn-rpc-tokens.py` for examples on token transaction
   construction and listing.
   Future releases will aim to extend the RPC API with more convenient
   ways to create and spend tokens, as well as upgrading the wallet storage
   and indexing subsystems to persistently store data about tokens of interest
   to the user. Later we expect to add GUI wallet management of Cash Tokens.

2. Transactions with SIGHASH_UTXO are not covered by DSProofs at present.

3. P2SH-32 is not used by default in the wallet (regular P2SH-20 remains
   the default wherever P2SH is treated).

4. The markup of Double Spend Proof events in the wallet does not survive
   a restart of the wallet, as the information is not persisted to the
   wallet.

5. The ABLA algorithm for BCH is currently temporarily set to cap the max block
   size at 2GB. This is due to limitations in the p2p protocol (as well as the
   block data file format in BCHN).


## Known Issues

Some issues could not be closed in time for release, but we are tracking all
of them on our GitLab repository.

- The minimum macOS version is 14.5 (Sonoma).
  Earlier macOS versions are no longer supported.

- Windows users are recommended not to run multiple instances of bitcoin-qt
  or bitcoind on the same machine if the wallet feature is enabled.
  There is risk of data corruption if instances are configured to use the same
  wallet folder.

- Some users have encountered unit tests failures when running in WSL
  environments (e.g. WSL/Ubuntu).  At this time, WSL is not considered a
  supported environment for the software. This may change in future.
  It has been reported that using WSL2 improves the issue.

- `doc/dependencies.md` needs revision (Issue #65).

- For users running from sources built with BerkeleyDB releases newer than
  the 5.3 which is used in this release, please take into consideration
  the database format compatibility issues described in Issue #34.
  When building from source it is recommended to use BerkeleyDB 5.3 as this
  avoids wallet database incompatibility issues with the official release.

- The `test_bitcoin-qt` test executable fails on Linux Mint 20
  (see Issue #144). This does not otherwise appear to impact the functioning
  of the BCHN software on that platform.

- With a certain combination of build flags that included disabling
  the QR code library, a build failure was observed where an erroneous
  linking against the QR code library (not present) was attempted (Issue #138).

- Possible out-of-memory error when starting bitcoind with high excessiveblocksize
  value (Issue #156)

- A problem was observed on scalenet where nodes would sometimes hang for
  around 10 minutes, accepting RPC connections but not responding to them
  (see #210).

- Startup and shutdown time of nodes on scalenet can be long (see Issue #313).

- Race condition in one of the `p2p_invalid_messages.py` tests (see Issue #409).

- Occasional failure in bchn-txbroadcastinterval.py (see Issue #403).

- wallet_keypool.py test failure when run as part of suite on certain many-core
  platforms (see Issue #380).

- Spurious 'insufficient funds' failure during p2p_stresstest.py benchmark
  (see Issue #377).

- If compiling from source, secp256k1 now no longer works with latest openssl3.x series.
  There are workarounds (see Issue #364).

- Spurious `AssertionError: Mempool sync timed out` in several tests
  (see Issue #357).

- For some platforms, there may be a need to install additional libraries
  in order to build from source (see Issue #431 and discussion in MR 1523).

- More TorV3 static seeds may be needed to get `-onlynet=onion` working
  (see Issue #429).

- Memory usage can be very high if repeatedly doing RPC `getblock` with
  verbose=2 on a hash of known big blocks (see Issue #466).

- A GUI crash failure was observed when attempting to encrypt a large imported
  wallet (see Issue #490).

- The 'wallet_multiwallet' functional test fails on latest Arch Linux due to
  a change in semantics in a dependency (see Issue #505). This is not
  expected to impact functionality otherwise, only a particular edge case
  of the test.

- The 'p2p_extversion' functional test is sensitive to timing issues when
  run at high load (see Issue #501).

---

## Changes since Bitcoin Cash Node 28.0.1

### New documents

None

### Removed documents

None

### Notable commits grouped by functionality

#### P2P Network

- ea755aa64812290c69684f2c699dcea547903a7a Added per-peer traffic limit rules, -peerratelimit
- 692527a80b3bbf91291039dbfdcbffef3722d117 [qa] Update mainnet static seeds in preparation for v29.0.0 release

#### Security or consensus relevant fixes

- e6a2445e5b3f5f70fdcf57785e0f944e277f6f42 Make CBlockIndex::nChainTx be a 64-bit unsigned int
- 724f9c9d6d39efc4a2a2190a5dfc82620184221d Post-upgrade 11: Add checkpoints to include post-upgrade blocks
- f759c6dc5294b64ed82a88618a109277826260b6 [backport] addrman, refactor: introduce user-defined type for internal nId
- b47c3c37c031dbf1c2c55f9977f1593fec0d5e63 Upgrade 11: Switch to height-based activation
- 7b9ebc2c68031aef346f90bb4fd658c7aa89c56a 2026 CHIPS: Bitops, Loops, Functions, and P2S
- 9c1e34e4828e19c10c90a2332c802984c90a7efd util: Filter control characters out of log messages
- 48c80b45c6d1634780556a153209ac45475a2ea7 log: Add rate limiting to LogPrintf
- 359dda2bdc76efcedec77f985cc4e74f4ace7504 Update checkpoints for v29.0.0 release
- f7db8343ec69ee18d4cfc795de1866fe59f574d8 [qa] Update "assume valid" and "minimum chain work" for v29.0.0 release

#### Interfaces / RPC

- be93a2d5b4251e8ad459ce3c2f331c4164c0d2b8 RPC: add verbosity=4 for `getblock` RPC, which adds script "byteCodePattern" information
- fe384182c160ced664585564eceec33017e4bb68 Add `coinstatsindex`, and use it with `gettxoutsetinfo` RPC; also add `MuHash3072`
- d809a665d62aaa44d28123207ba89a50bf86e183 rpc: Allow explicitly empty NFT commitment
- 71ea195efe246b82052e5d62705b8dfe87561cf1 RPC: add patterns flag for `getblock` and `getrawtransaction` verbose RPCs
- ec2584d22898b44e8886bf0f55577d0dca0b156b Bump node expiry to May 2027, add new `-upgrade2027activationtime`
- b262ce3c2b9aea60a08ec67f692c5a46ac3ae1a6 RPC: Return permitbaremultisig and maxdatacarriersize in getmempoolinfo
- 970df501b951062f8862635de60ac78062627a7e Add getblockchaininfo.upgrade_status support and tests
- b5cd19f6ff7dd97e0817461305235627f930bf19 Format upgrade_status.name for external use

#### Features in internal development: support for UTXO commitments

None

### Data directory changes

None

#### Performance optimizations

- 8626f7e68f049da4f96d67ce31aac5bedf21c8fb UniValue: Use std::variant, using less memory
- 51993a4b548318a4d65c740e67d8622c01b00536 Optimize INV TX relay, reduce inefficiency/redundant sorting
- c06d873eed24932434c48df44060c841ef2d5444 Use std::move in unserialization of std::set and std::map
- 3940f71d298529347599ea5429b230ca96ea5efc Reduce excess allocations & copying in `VerifyScript`, plus small refactor
- d8abd38128ad97e3f64e59e3fc4e8d55bcd8324c Added caching mechanism for `GetMedianTimePast()` to avoid wasteful recalculation
- 82a12a4d356dabeb4ef9a9e11da0bdb71c3a38be Optimization: make use of CTOR when searching for txn in a block
- 6e2ed5e591410479adad07149131ce2d4eff6029 blockstorage: Avoid computing block header hash twice in ReadBlockFromDisk
- aed917c6332e195872a95735292c20dd2e7aefe7 Optimization to the script VM to use `int64_t` native ints when possible, falling back to `BigInt`
- c5de915af07fb108ba289fcbef0e4bc76d784cf2 Reduce redundant copying in checksig codepaths
- c2fac45dde48a79fdb4f571f53d0b1fd4fe1065e Introduce BufferedReader and BufferedWriter to streams.h, and use it

#### GUI

- b44070fb881a236ce9ebb4a3db1967f2dc9664da Update chainTxData for main, test3, test4, and chipnet

#### Code quality

- 6d94eabf6ee3dba6641827e12c5b0e343d8bbd3e net: Refactor lifetime management of CNode, use smart_ptr; put in map.
- 8a2fe7456c6f751f3843406495959653383f3d7a Fix typo in a comment in policy/policy.h
- ac4f4638e70545a598f7cb5305c0fdf1a5633541 Trivial: Prevent UB in class CNoDestination
- 1deb85f1916ad8c45bac9ec1f087b8ede627671f Remove boost::variant dependency, use std::variant instead.
- 7ca3ed8a60fe32f5655d328cb6018d2c61301495 Guard against unsafe PSBT tx usage
- 543eaf36c87571f99692730c055ed639978b3c6f validation: Give `m_block_index` ownership of `CBlockIndex`s
- 0916766fe5fdd8503706c3c639a64650cb199e23 Set project to use the C++20 language standard
- 18aedd284e7f1e43bafa2d812a3825b70f73f73f C++20: Add some endian sanity checks to assumptions.h and compat/endian.h
- 37dc618c8b58090bcfafb36116865f4bc4369b5c C++20: Handle some C++20-related TODOs for script/bigint.cpp
- 7219224e0622ccb0ae9ae846708c44edcbfd22d1 trivial refactor: Remove spurious virtual from final ~CZMQNotificationInterface
- 2ae91fcdc33edeba62da2ef8010537d324748845 refactor: Declare g_zmq_notification_interface as unique_ptr
- 7eb0a47945b765eff3347b29b5c5ea6b9e62d027 Add lifetimebound to attributes for general-purpose usage
- edddffc43129be0fd1fddf07a4cc331fe092606f refactor: Move most CAutoFile methods into an implementation file
- 4348729ce988d11fbfd25b3348b38ef171629830 refactor: Move most CBufferedFile methods into streams.cpp
- 284ad9d8ea605ce6bfcd0033a3c3e56b56453cc8 Slightly modernize the code in blockstorage and add overflow checks
- c101497e7f7cef1af4ed8f16d0597305193dcb7f Add Assume(), Assert(), and NONFATAL_UNREACHABLE macros
- 5c259ee2eb01f89bdacfaf5b068b46ec31027038 Refactor Transaction Serialization in RPC and REST
- b30c1121c68eec248e5d9f19da65c32716f91b38 Check for arithmetic right shift on negative ints
- 2b79f05e2c8ab5747d8b57fb6bf8d7011087e860 Removed dependency on boost header: '<boost/range/adaptor/sliced.hpp>'
- fcb0a4bf3ab21d636c620ed66ed2e373ea191771 Added support for serialization of `std::span<uint8_t>`
- e7597ada8660e774387e512653d8cc2c63a18eee Miscellaneous fixups to the prevector class
- b2e756d2c70f45e4a365c80264ea68afaa236bd3 Make prevector class convertible to std::span
- a973bac8ab8e0ae23610ec50db28abf7d1f18dcd Remove duplicated typedefs for `valtype` and `stacktype`
- 9a7d3a8cb9772423c6ee8927694cab76855ed135 streams: Prefer `std::as_bytes` to `reinterpret_cast` + fix a -Wshadow warning
- abbe1272e6d2a03834ea723ff522d6f17b9abda8 scheduler: Add std::chrono support for scheduleFromNow
- e9f25d7596f1160ff4119977333f987da97dc74c Remove boost dependency: `<boost/range/iterator.hpp>`

#### Documentation updates

- 452bebe03b0581620a119ec15b37dc8f5e45dda9 [qa] Bump version to 28.0.2, rotate release notes
- f033024a58fd596eb86a8c3aa25ea8c10d2eb295 Update copyright year to 2025
- dbc5e77ce32a4e39f140cc1b65019181873af935 Bump version to 29.0.0

#### Build / general

- 0e0177f90636b429dc05ffbe276fc927430ef225 Upgrade gitian to use Ubuntu Jammy 22.04 in order to get `gcc-11` and/or `clang-14` for OSX
- 8de7a7a7af7a314c3179b8e6d03f6a98c884522a Remove Boost::Thread requirement from src/CMakeLists.txt
- 0435c42535f5465085f126c931527ebb0eb38a34 depends: Don't build boost::thread
- 9d69f72414ff866be62792045cf5e13f49683f33 gitian: Remove installation of ALL of boost in favor of the pieces we use

#### Build / Linux

- f209581ff8d1fd48c286c1951bfa0f3ef97069e7 Fix BCHN not compiling on latest Arch Linux

#### Build / Windows

None

#### Build / MacOSX

- d33eb30ea92a24fa04f0206df7c4e51dd8d800db gitian & depends: Switch macOS SDK to 13.3 (Ventura)
- b0a1f0a4434e11d565af58b6cf9d778225fc3b8a gitian: Switch macOS SDK to 14.5 (Sonoma) for better C++20 support + Upgrade Qt to 5.15.16

#### Tests / test framework

- f1d0d97f3998f74999f7d1f4012d1812223ca36e tests: Add fuzzing harness for bloom filter class CBloomFilter
- cad8a8d39855e9b131e7de3edba99521e4873fdb tests: Add fuzzing harness for rolling bloom filter class CRollingBloomFilter
- 59d2be4e19bbccbefcc5e1f670bb0697d8325612 qa: Fix dbcrash test
- a72c8aad7a27213752b072e3bd2c06805f65d0a9 trivial: Fix off-by-1-error in mempool_tests
- cffee23e63c613967efaf1d8665749e660dfeee6 Added a unit test to ensure serializing default constructed PSBT throws
- 1648147aa1d789d2d4714446c778ba5888bac73f Fix `wallet_tests` after change to the way `mapBlockIndex` works
- 97d96861ad08b75f9a9fa8e51248feee648d61be Fix failing `ninja check-functiona-upgrade-activated`
- aad3069073fce7730f50f2358b3797b63d265c4b Add unit tests for new BufferedReader and BufferedWriter
- 40d629c8d57afbe47f17be4905f58e9d8685775e unit tests: Update setup_common.h to allow printing of enums + optionals
- b633205fc28dd3200aebf4ede1cc123a89371662 logging: Add support for tearing down and reconstructing the logger
- 829cb8675da52d162e632a96d18fc99a891834f5 logging: Added unit tests for the new rate limiting facility

#### Benchmarks

- 2b3c85c941de3c702514088ce5ecd4dc34a462bb bench: add readwriteblock benchmark

#### Seeds / seeder software

- 692527a80b3bbf91291039dbfdcbffef3722d117 [qa] Update mainnet static seeds in preparation for v29.0.0 release
- 442a9ee83dd2929e40340557a899780550943d75 [qa] Update all testnet static seeds in preparation for v29.0.0 release
- 1672d1fe160093201aa21b07be558fb6951981b5 Fix makeseeds.py to no longer require MIN_BLOCKS = 540000

#### Maintainer tools

- 7a34b7597fb0ead9aad3dccddc44be44d087473b [backport] script: add script to generate example bitcoin.conf

#### Infrastructure

- 8188f5166040641dc9eb871e2bc3a36b823f5ff1 Update im_uname keys

#### Cleanup

- 0a675168a67e8286a07b623681e90745c194a154 trivial: Remove unused/aborting function from class CScript
- 327bb0ce0478cc6eb42f04bf62ce3c280afa3a02 Trivial: remove #inlcude `<cstdbool>` in seeder/dns.cpp
- 70f07e91197a3b1a78d82fbe15c2978171f81df0 Logging: Fix bad ABC backport which set the new fileout to unbuffered incorrectly

#### Continuous Integration (GitLab CI)

- c40e6332144a317054d3cf90c8820cafc73f057c Resolve mkdocs/jinja version incompatibility

#### Compatibility

- 172bb33e475064d97dab6899fcb92fcb17aa00e6 Fix for failure to compile UniValue using older GCC 8.3.0
- fe428e8c29a1fd2de30b87b0d68fda566c50b649 Fix gitian builder due to removed File.exists method in ruby
- c5c485439574216499049a456644b19cb6bbe18a Fix compile issue for older GCC with the MuHash class
- 74b0e7d76edf30f4b04318521634f209fe0a003c Fix for CoinStatsIndex compiler bug (gcc 8.3.0)
- 9b3da00b7d33128d77996e07a34f19e3fdbc4ad7 Fix for clang-8 compile issue
- 9770e27bbaab219390170014423a556b91b96723 Resolve CMP0116 deprecation warnings
- 1e1ac70a238f3669cdbcb1b08fb55aa42b3e26ad Fix failing uint256_tests if compiled with gcc 15.1.1
- f7fdfef88a33266cbdd93a50b4a2b228496abc66 Fixed many false warnings from newer GCC
- 499c4cd021882940b0e6e37a5e60620569c5281c Fixed more false warnings from newer GCC (2/N)

#### Backports

- f1d0d97f3998f74999f7d1f4012d1812223ca36e tests: Add fuzzing harness for bloom filter class CBloomFilter
- cad8a8d39855e9b131e7de3edba99521e4873fdb tests: Add fuzzing harness for rolling bloom filter class CRollingBloomFilter
- 7a34b7597fb0ead9aad3dccddc44be44d087473b [backport] script: add script to generate example bitcoin.conf
- fe384182c160ced664585564eceec33017e4bb68 Add `coinstatsindex`, and use it with `gettxoutsetinfo` RPC; also add `MuHash3072`
- f759c6dc5294b64ed82a88618a109277826260b6 [backport] addrman, refactor: introduce user-defined type for internal nId
- 543eaf36c87571f99692730c055ed639978b3c6f validation: Give `m_block_index` ownership of `CBlockIndex`s
- 7219224e0622ccb0ae9ae846708c44edcbfd22d1 trivial refactor: Remove spurious virtual from final ~CZMQNotificationInterface
- 2ae91fcdc33edeba62da2ef8010537d324748845 refactor: Declare g_zmq_notification_interface as unique_ptr
- 7eb0a47945b765eff3347b29b5c5ea6b9e62d027 Add lifetimebound to attributes for general-purpose usage
- c101497e7f7cef1af4ed8f16d0597305193dcb7f Add Assume(), Assert(), and NONFATAL_UNREACHABLE macros
- 6e2ed5e591410479adad07149131ce2d4eff6029 blockstorage: Avoid computing block header hash twice in ReadBlockFromDisk
- b262ce3c2b9aea60a08ec67f692c5a46ac3ae1a6 RPC: Return permitbaremultisig and maxdatacarriersize in getmempoolinfo
- 2b3c85c941de3c702514088ce5ecd4dc34a462bb bench: add readwriteblock benchmark
- c2fac45dde48a79fdb4f571f53d0b1fd4fe1065e Introduce BufferedReader and BufferedWriter to streams.h, and use it
- aad3069073fce7730f50f2358b3797b63d265c4b Add unit tests for new BufferedReader and BufferedWriter
- 48c80b45c6d1634780556a153209ac45475a2ea7 log: Add rate limiting to LogPrintf
- 40d629c8d57afbe47f17be4905f58e9d8685775e unit tests: Update setup_common.h to allow printing of enums + optionals
- 829cb8675da52d162e632a96d18fc99a891834f5 logging: Added unit tests for the new rate limiting facility
