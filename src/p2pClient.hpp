//
// Created by some yuan on 2019/7/31.
//

#ifndef CELL_P2PCLIENT_HPP
#define CELL_P2PCLIENT_HPP

#include "../lib/httplib.h"
#include <iostream>
#include <ifaddrs.h>
#include <future>
#include <boost/algorithm/string.hpp>
#include "threadpool.hpp"
#include <fstream>
#include <sstream>

namespace cell {
#define EXIT 0

    class P2PClient {
    public:
        P2PClient(int64_t port = 9000) : port_(port) {}

        void Start()
        {
            UI();
        }

        using Chunk = std::vector<std::pair<int64_t, int64_t >>;
    private:
        void UI() {
            int option = -1;
            while (option != EXIT) {
                menu(option);
                switch (option) {
                    case 1:
                        searchHost();
                        break;
                    case 2:
                        matchHost();
                        break;
                    case 3:
                        downloadFile();
                        break;
                    default:
                        break;
                }
            }
        }

        void menu(int &option) {
            std::cout << "<<<<<<<<<<<<<<<" << std::endl;
            std::cout << "1. 搜索附近主机" << std::endl;
            std::cout << "2. 配对附近主机" << std::endl;
            std::cout << "3. 下载主机共享文件" << std::endl;
            std::cout << "0. 退出" << std::endl;
            std::cout << "<<<<<<<<<<<<<<<" << std::endl;
            std::cout << "请输入选择：" << std::endl;
            std::cin >> option;
        }

        void searchHost() {
            std::vector<std::string> addrList;
            getAllAddr(addrList);

            std::vector<std::future<void>> futures;
            for (auto &e : addrList) {
                futures.emplace_back(std::async(&P2PClient::getPairStatus, this, std::ref(e)));
            }
            for (auto &e : futures)
                e.get();
            std::cout << "附近主机数：" << hostIp_.size() << std::endl;
        }

        void matchHost() {
            int choose;
            showHostList();
            std::cout << "请选择主机：\n";
            std::cin >> choose;
            if (choose < 1 && choose > hostIp_.size()) {
                std::cout << "choose warning" << std::endl;
                return;
            }
            ip_ = hostIp_[choose - 1];
            httplib::Client cli(ip_.c_str(), port_);
            auto res = cli.Get("/list");
            if (res && res->status == 200) {
                parsingBody(res->body);
            }
        }

        void parsingBody(std::string &body) {
            fileList_.clear();
            std::vector<std::string> fileList;
            boost::split(fileList, body, boost::is_any_of("\n"), boost::token_compress_on);
            for (auto &e : fileList) {
                if (!e.empty()) {
                    fileList_.emplace_back(e);
                }
            }
        }

        void downloadFile() {
            std::cout << "===============" << std::endl;
            int i = 1;
            for (auto &e : fileList_) {
                std::cout << i << ". " << e << std::endl;
                ++i;
            }
            std::cout << "===============" << std::endl;

            std::cout << "请选择想要下载的文件\n";
            int choose;
            std::cin >> choose;
            multiDownload(fileList_[choose - 1]);
        }

        void multiDownload(std::string& filename) {
            std::vector<std::future<bool>> futures;

            int64_t fileSize = 0;

            if (!getFileSize(filename, fileSize) || fileSize == -1)
                return;

            Chunk chunks;
            getChunk(fileSize, chunks);
            ThreadPool pool;
            for (auto &e : chunks) {
                futures.emplace_back(pool.execute(&P2PClient::downloadTask, this, std::ref(filename), std::ref(e)));
            }

            for (auto&& e : futures)
            {
                e.get();
            }
        }

        bool getFileSize(std::string& filename, int64_t& fileSize)
        {
            httplib::Client cli(ip_.c_str(), port_);
            auto res = cli.Head(("/list/" + filename).c_str());
            if (res == nullptr && res->status != 200)
                return false;
            if (res->has_header("Content-Length"))
            {
                std::stringstream s;
                s << res->get_header_value("Content-Length");
                s >> fileSize;
                return true;
            } else
            {
                std::cerr << "unable to download this file, size is not clear!" << std::endl;
                fileSize = -1;
            }
            return false;
        }

        bool downloadTask(std::string &filename, std::pair<int64_t, int64_t> &chunk) {
            httplib::Client cli(ip_.c_str(), port_);
            auto res = cli.Get(("/list/" + filename).c_str(),
                               {httplib::make_range_header(chunk.first, chunk.second)});
            if (res && res->status == 206) {
                std::ofstream file("../Download/" + filename);
                if (!file.is_open())
                    throw "is not open";
                file.seekp(chunk.first);
                file.write(res->body.c_str(), chunk.second - chunk.first + 1);
                std::clog << "download success" << chunk.first << " " << chunk.second << "\n";
                return true;
            } else
            {
                std::cerr << "download failed" << std::endl;
            }
            return false;
        }

        void getChunk(int64_t fileSize, Chunk &chunks) {

            const int64_t CHUNK = 100 * 1024 * 1024;
            int64_t start = 0;

            int count;
            if (fileSize % CHUNK == 0) {
                count = fileSize / CHUNK;
            } else {
                count = fileSize / CHUNK + 1;
            }

            for (int i = 0; i < count; ++i) {
                if (i == count - 1) {
                    // the last chunk
                    chunks.emplace_back(std::make_pair(i * CHUNK, fileSize - 1));
                } else {
                    chunks.emplace_back(std::make_pair(i * CHUNK, (i + 1) * CHUNK - 1));
                }
            }
        }

        void getPairStatus(std::string &ip) {
            httplib::Client cli(ip.c_str(), port_);
            auto res = cli.Get("/auth");
            if (res && res->status == 200) {
                hostIp_.emplace_back(ip);
                std::clog << "auth success" << std::endl;
            } else {
                std::clog << "auth failed" << std::endl;
            }
        }

        void showHostList() {
            int i = 1;
            std::cout << "===============" << std::endl;
            for (auto &e : hostIp_) {
                std::cout << i << ". " << e << std::endl;
                ++i;
            }
            std::cout << "===============" << std::endl;
        }

        // 获取局域网中所有ipv4
        void getAllAddr(std::vector<std::string> &addrList) {
            struct ifaddrs *ifaddrs = nullptr;
            uint32_t netAddr, endHost;
            ::getifaddrs(&ifaddrs);
            while (ifaddrs) {
                if (ifaddrs->ifa_addr->sa_family == AF_INET && strcmp(ifaddrs->ifa_name, "wifi0") == 0) {
                    auto *addr = (struct sockaddr_in *) ifaddrs->ifa_addr;
                    auto *mask = (struct sockaddr_in *) ifaddrs->ifa_netmask;

                    netAddr = ntohl(addr->sin_addr.s_addr & mask->sin_addr.s_addr);
                    endHost = ntohl(~mask->sin_addr.s_addr);
                    for (size_t i = 1; i < endHost-1; ++i) {
                        struct in_addr tmp{};
                        tmp.s_addr = htonl(netAddr + i);
                        addrList.emplace_back(inet_ntoa(tmp));
                    }
                }
                ifaddrs = ifaddrs->ifa_next;
            }
        }

    private:
        std::vector<std::string> hostIp_;
        std::vector<std::string> fileList_;
        std::string ip_;
        int64_t port_;
    };
}

#endif //CELL_P2PCLIENT_HPP
