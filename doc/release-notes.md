# Release Notes for Bitcoin Cash Node version 28.0.2

Bitcoin Cash Node version 28.0.2 is now available from:

  <https://bitcoincashnode.org>

## Overview

This release of Bitcoin Cash Node (BCHN) is a patch release.

## Usage recommendations

Users who are running v28.0.1 or older are encouraged to upgrade to v28.0.2.

## Network changes

None

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

## Regressions

Bitcoin Cash Node 28.0.2 does not introduce any known regressions as compared to 28.0.1.

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

- The minimum macOS version is 13.3 (Ventura).
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

#### Security or consensus relevant fixes

None

#### Interfaces / RPC

None

#### Features in internal development: support for UTXO commitments

None

### Data directory changes

None

#### Performance optimizations

None

#### GUI

None

#### Code quality

None

#### Documentation updates

None

#### Build / general

None

#### Build / Linux

None

#### Build / Windows

None

#### Build / MacOSX

None

#### Tests / test framework

None

#### Benchmarks

None

#### Seeds / seeder software

None

#### Maintainer tools

None

#### Infrastructure

None

#### Cleanup

None

#### Continuous Integration (GitLab CI)

None

#### Compatibility

None

#### Backports

None
