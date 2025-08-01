// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <streams.h>

#include <array>
#include <cstdio>

CAutoFile::CAutoFile(FILE *filenew, int nTypeIn, int nVersionIn) : nType(nTypeIn), nVersion(nVersionIn), file(filenew) {}

CAutoFile::CAutoFile(CAutoFile &&o) : nType(o.nType), nVersion(o.nVersion), file(o.file) { o.setNull(); }

CAutoFile &CAutoFile::operator=(CAutoFile &&o) {
    if (this != &o) {
        if (file != o.file) fclose(); // close our managed file if we have one and it's not same as o's
        file = o.file;
        nType = o.nType;
        nVersion = o.nVersion;
        o.setNull(); // null out moved-from object to complete the transfer
    }
    return *this;
}

CAutoFile::~CAutoFile() { fclose(); }

void CAutoFile::setNull() {
    nType = nVersion = 0;
    file = nullptr;
}

void CAutoFile::fclose() {
    if (file) {
        std::fclose(file);
        file = nullptr;
    }
}

FILE *CAutoFile::release() {
    FILE *ret = file;
    file = nullptr;
    return ret;
}

void CAutoFile::read(std::span<std::byte> buf) {
    if (!file) {
        throw std::ios_base::failure("CAutoFile::read: file handle is nullptr");
    }
    if (std::fread(buf.data(), 1, buf.size(), file) != buf.size()) {
        throw std::ios_base::failure(std::feof(file) ? "CAutoFile::read: end of file"
                                                     : "CAutoFile::read: fread failed");
    }
}

void CAutoFile::ignore(size_t nSize) {
    if (!file) {
        throw std::ios_base::failure("CAutoFile::ignore: file handle is nullptr");
    }
    std::array<std::byte, 4096> buf;
    while (nSize > 0) {
        const size_t nNow = std::min(nSize, buf.size());
        if (std::fread(buf.data(), 1, nNow, file) != nNow) {
            throw std::ios_base::failure(std::feof(file) ? "CAutoFile::ignore: end of file"
                                                         : "CAutoFile::read: fread failed");
        }
        nSize -= nNow;
    }
}

void CAutoFile::write(std::span<const std::byte> buf) {
    if (!file) {
        throw std::ios_base::failure("CAutoFile::write: file handle is nullptr");
    }
    if (std::fwrite(buf.data(), 1, buf.size(), file) != buf.size()) {
        throw std::ios_base::failure("CAutoFile::write: write failed");
    }
}
