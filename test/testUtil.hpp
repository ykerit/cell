//
// Created by some yuan on 2019/7/30.
//

#ifndef CELL_TESTUTIL_HPP
#define CELL_TESTUTIL_HPP

#include "../src/util.hpp"
#include "../src/p2pServer.hpp"
#include "../src/p2pClient.hpp"

namespace cell {
    using namespace util;

    void TestGenerateFileList() {
        Dict dict;
        GenerateFileList(dict);
        for (auto &e : dict) {
            std::cout << e.first << " " << e.second.first << " " << e.second.second << std::endl;
        }
    }

    void TestP2PServer() {
        P2PServer app;
        app.Start();
    }

    void TestP2PClient() {
        P2PClient app;
    }

    void TestGetAddr(std::vector<std::string> &addrList) {
        struct ifaddrs *ifaddrs = nullptr;
        uint32_t netAddr, endHost;
        ::getifaddrs(&ifaddrs);
        while (ifaddrs) {
            if (ifaddrs->ifa_addr->sa_family == AF_INET && strcmp(ifaddrs->ifa_name, "wifi0") == 0) {
                auto *addr = (struct sockaddr_in *) ifaddrs->ifa_addr;
                auto *mask = (struct sockaddr_in *) ifaddrs->ifa_netmask;

                netAddr = ntohl(addr->sin_addr.s_addr & mask->sin_addr.s_addr);
                endHost = ntohl(~mask->sin_addr.s_addr);
                for (size_t i = 0; i < endHost; ++i) {
                    struct in_addr tmp{};
                    tmp.s_addr = htonl(netAddr + i);
                    addrList.emplace_back(inet_ntoa(tmp));
                }
            }
            ifaddrs = ifaddrs->ifa_next;
        }
    }

    void TestParsingRange() {
        int64_t a, b;
        std::string test = "bytes=0-35";
        P2PServer::parsingRange(test, a, b);
        std::cout << a << " " << b << std::endl;
    }

    void TestDownloadFolder() {
        std::ofstream file("../Download/test.txt");
        if (!file.is_open()) {
            std::cout << "can't open" << std::endl;
        } else {
            file << "funk you";
            file.close();
        }

    }

    void getChunk(int64_t fileSize, P2PClient::Chunk &chunks) {

        const int64_t CHUNK = 100 * 1024 * 1024;
        int64_t start = 0;

        int count;
        if (fileSize % CHUNK == 0) {
            count = fileSize / CHUNK;
        } else {
            count = fileSize / CHUNK + 1;
        }

        std::cout << count << std::endl;

        for (int i = 0; i < count; ++i) {
            if (i == count - 1) {
                // the last chunk
                chunks.emplace_back(std::make_pair(i * CHUNK, fileSize - 1));
            } else {
                chunks.emplace_back(std::make_pair(i * CHUNK, (i + 1) * CHUNK - 1));
            }
        }
    }

    void TestChunk() {
//        int64_t size = 1048576000;
        int64_t size = 120;
        P2PClient::Chunk chunks;
        getChunk(size, chunks);
        for (auto &e : chunks) {
            std::clog << "chunks: " << e.first << "-" << e.second << std::endl;
        }
    }
}

#endif //CELL_TESTUTIL_HPP
