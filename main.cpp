#define TEST 0
#define DEBUG 1


#if TESTSERVER

#include "test/testUtil.hpp"

using namespace cell;

int main() {
    TestP2PServer();
    return 0;
}

#include "test/testUtil.hpp"

using namespace cell;

int main() {

    TestP2PClient();
    return 0;

}

#endif


#if DEBUG

#include "src/p2pClient.hpp"
#include "src/p2pServer.hpp"
using namespace cell;
int main()
{
    std::thread([]{
        P2PServer server;
        server.Start();
    }).detach();

    P2PClient client;
    client.Start();
}

#endif