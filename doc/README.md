# Vuhn Bitcoin Cash Node Setup

Vuhn Bitcoin Cash Node is a node and wallet implementation for the Bitcoin Cash network.
It downloads and, by default, stores the entire history of Bitcoin Cash
transactions, which requires a few hundred gigabytes of disk space. Depending on
the speed of your computer and network connection, the synchronization process
can take anywhere from a few hours to a day or more.

## Running

The following are some helpful notes on how to run Vuhn Bitcoin Cash Node on your
native platform.

### Unix

Unpack the files into a directory and run:

- `bin/bitcoin-qt` (GUI) or
- `bin/bitcoind` (headless)

### Windows

Unpack the files into a directory, and then run `bitcoin-qt.exe`.

### macOS

Drag `vuhn-bitcoin-cash-node` to your applications folder, and then run `vuhn-bitcoin-cash-node`.

## Help

- Open an issue at [https://github.com/vu-hn/Vuhn-Bitcoin-Cash-Node/issues](https://github.com/vu-hn/Vuhn-Bitcoin-Cash-Node/issues)

## License

Distribution is done under the [MIT software license](/COPYING). This product includes software developed by the
OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/), cryptographic software written by Eric Young
([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
