//
// Created by some yuan on 2019/7/30.
//

#ifndef CELL_P2PSERVER_HPP
#define CELL_P2PSERVER_HPP

#include "../lib/httplib.h"
#include "util.hpp"
#include <fstream>
#include <sstream>

namespace cell {
    using namespace util;

    class P2PServer {
    public:
        P2PServer() = default;

        P2PServer(const P2PServer &) = delete;

        P2PServer &operator=(const P2PServer &) = delete;

        void Start() {
            server.Get("/auth", authentication);
            server.Get("/list", getFileList);
            server.Get("/list/(.*)", downloadFile);
            server.listen("0.0.0.0", 9000);
        }

    private:
        static void authentication(const httplib::Request &req, httplib::Response &rsp) {
            std::cout << "pair" << std::endl;
            rsp.status = 200;
        }

        static void getFileList(const httplib::Request &req, httplib::Response &rsp) {
            dict_.clear();
            GenerateFileList(dict_);
            rsp.status = 200;
            for (auto &e : dict_) {
                rsp.body += e.first + "\n";
            }
        }

        static void downloadFile(const httplib::Request &req, httplib::Response &rsp) {
            std::string filename = req.matches[1];

            if (filename.size() == 0)
                return;
            auto iter = dict_.find(filename);
            if (iter == dict_.end()) {
                rsp.status = 400;
                rsp.set_content("no file in folders", "text/plain");
                return;
            }

            if (req.method == "HEAD")
            {
                rsp.status = 200;
                rsp.set_header("Content-Length", std::to_string(dict_[filename].second).c_str());
            } else if (req.method == "GET")
            {

                if (!req.has_header("Range"))
                {
                    rsp.status = 400;
                    rsp.set_content("no range", "text/plain");
                    return;
                }
                std::string range = req.get_header_value("Range", 0);
                int64_t start, end;
                if (parsingRange(range, start, end))
                {
                    if (start < 0 || end > dict_[filename].second || start >= end)
                    {
                        rsp.status = 400;
                        return;
                    }
                }

                std::clog << "range" << start << " " << end << std::endl;

                int64_t rangeSize = end - start + 1;

                std::cout << "size: " << rangeSize << std::endl;

                std::ifstream file(dict_[filename].first, std::ios_base::binary);
                if (!file.is_open()) {
                    rsp.status = 400;
                    rsp.set_content("file is not open", "text/plain");
                    return;
                }

                file.seekg(start);
                rsp.body.resize(rangeSize);
                file.read(&rsp.body[0], rangeSize);
                rsp.status = 206;
                rsp.set_content(rsp.body, "text/plain");

                file.close();
            }

        }

    public:
        static bool parsingRange(std::string &range, int64_t &start, int64_t &end) {
            size_t start_ = range.find("=");
            size_t end_ = range.find("-");
            if (start_ != std::string::npos && end_ != std::string::npos) {
                std::stringstream s;
                s << range.substr(start_ + 1, end_ - start_ + 1);
                s >> start;
                std::stringstream b;
                b << range.substr(end_+1);
                b >> end;
            } else
            {
                return false;
            }
            return true;
        }

    private:
        httplib::Server server;
        static util::Dict dict_;
    };

    util::Dict P2PServer::dict_;
}

#endif //CELL_P2PSERVER_HPP
