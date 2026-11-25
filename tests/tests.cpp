#include "tcp_client.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

constexpr int NUM_CLIENTS = 4;
constexpr int REQUESTS_PER_CLIENT = 250000;

constexpr uint64_t TOTAL_REQUESTS =
    static_cast<uint64_t>(NUM_CLIENTS) *
    REQUESTS_PER_CLIENT;

std::atomic<uint64_t> requests_sent{0};
std::atomic<uint64_t> new_sent{0};
std::atomic<uint64_t> cancel_sent{0};
std::atomic<uint64_t> modify_sent{0};
std::atomic<uint64_t> pnl_sent{0};

struct ActiveOrder {
    uint64_t id;
    std::string symbol;
};

void stress_client(int client_id, uint64_t seed)
{
    try {
        TestClient client("127.0.0.1", 8080);

        client.connect();

        std::cout
            << "[Client " << client_id
            << "] connected\n";

        std::atomic<bool> receiving{true};
        std::thread receiver([&](){
            while (receiving){
                try{
                    std::string response = client.receive();
                }
                catch(...){
                    break;
                }
            }     
        });

        std::mt19937_64 rng(seed);

        // Request distribution:
        // 70% NEW
        // 15% CANCEL
        // 10% MODIFY
        //  5% PNL

        std::uniform_int_distribution<int> request_dist(0, 99);
        std::uniform_int_distribution<int> side_dist(0, 1);
        std::uniform_int_distribution<int> qty_dist(1, 100);
        std::uniform_int_distribution<int> price_dist(1, 100);
        std::uniform_int_distribution<int> symbol_dist(0, 2);

        const char* symbols[] = {
            "TEST",
            "TEST2",
            "TEST3"
        };

        /*
         * Give every client a unique order-ID range.
         *
         * Client 1: 1       - 250000
         * Client 2: 250001  - 500000
         * Client 3: 500001  - 750000
         * Client 4: 750001  - 1000000
         */
        const uint64_t first_id =
            static_cast<uint64_t>(client_id - 1) *
            REQUESTS_PER_CLIENT + 1;

        std::vector<ActiveOrder> active_orders;
        active_orders.reserve(REQUESTS_PER_CLIENT);

        for (int i = 0; i < REQUESTS_PER_CLIENT; ++i) {

            int request_type = request_dist(rng);

            /*
             * If there are no existing orders, force NEW.
             */
            if (active_orders.empty()) {
                request_type = 0;
            }

            /*
             * ============================
             * NEW
             * ============================
             */
            if (request_type < 70) {

                const uint64_t order_id =
                    first_id + i;

                const char* side =
                    side_dist(rng) == 0
                        ? "BUY"
                        : "SELL";

                const char* symbol =
                    symbols[symbol_dist(rng)];

                const int qty =
                    qty_dist(rng);

                const int price =
                    price_dist(rng);

                std::string message =
                    "NEW," +
                    std::to_string(order_id) +
                    "," +
                    side +
                    "," +
                    symbol +
                    "," +
                    std::to_string(qty) +
                    "," +
                    std::to_string(price) +
                    "\n";

                client.send(message);

                active_orders.push_back({
                    order_id,
                    symbol
                });

                new_sent.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }

            /*
             * ============================
             * CANCEL
             * ============================
             */
            else if (request_type < 85) {

                std::uniform_int_distribution<size_t>
                    index_dist(
                        0,
                        active_orders.size() - 1
                    );

                const size_t index =
                    index_dist(rng);

                const ActiveOrder& order =
                    active_orders[index];

                std::string message =
                    "CANCEL," +
                    std::to_string(order.id) +
                    "," +
                    order.symbol +
                    "\n";

                client.send(message);

                cancel_sent.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }

            // /*
            //  * ============================
            //  * MODIFY
            //  * ============================
            //  */
            else if (request_type < 95) {

                std::uniform_int_distribution<size_t>
                    index_dist(
                        0,
                        active_orders.size() - 1
                    );

                const size_t index =
                    index_dist(rng);

                const ActiveOrder& order =
                    active_orders[index];

                const int new_qty =
                    qty_dist(rng);

                const int new_price =
                    price_dist(rng);

                std::string message =
                    "MODIFY," +
                    std::to_string(order.id) +
                    "," +
                    order.symbol +
                    "," +
                    std::to_string(new_qty) +
                    "," +
                    std::to_string(new_price) +
                    "\n";

                client.send(message);

                modify_sent.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }

            /*
             * ============================
             * PNL
             * ============================
             */
            else {

                const char* symbol =
                    symbols[symbol_dist(rng)];

                std::string message =
                    "PNL," +
                    std::string(symbol) +
                    "\n";

                client.send(message);

                pnl_sent.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }

            requests_sent.fetch_add(
                1,
                std::memory_order_relaxed
            );

            /*
             * Don't print every request.
             * Printing would destroy the benchmark.
             */
            if ((i + 1) % 50000 == 0) {

                std::cout
                    << "[Client "
                    << client_id
                    << "] "
                    << (i + 1)
                    << " / "
                    << REQUESTS_PER_CLIENT
                    << '\n';
            }
        }

        /*
         * IMPORTANT:
         *
         * Keep the connection alive while the exchange
         * processes pending orders/reports.
         *
         * This prevents the dispatcher from attempting
         * to write to a client queue that has already
         * disappeared.
         */
        std::cout
            << "[Client " << client_id
            << "] generation complete; waiting...\n";

        std::this_thread::sleep_for(
            std::chrono::seconds(10)
        );
        receiving = false;
        client.close();
        if(receiver.joinable()){
            receiver.join();
        }
        std::cout
            << "[Client " << client_id
            << "] disconnected\n";
    }
    catch (const std::exception& e) {

        std::cerr
            << "[Client " << client_id
            << "] ERROR: "
            << e.what()
            << '\n';
    }

}

int main()
{
    std::cout
        << "\n========================================\n"
        << "       1M MIXED WORKLOAD STRESS TEST\n"
        << "========================================\n";

    std::cout
        << "Clients:              "
        << NUM_CLIENTS
        << '\n';

    std::cout
        << "Requests / client:    "
        << REQUESTS_PER_CLIENT
        << '\n';

    std::cout
        << "Total requests:       "
        << TOTAL_REQUESTS
        << "\n\n";

    const auto start =
        std::chrono::steady_clock::now();

    std::vector<std::thread> clients;
    clients.reserve(NUM_CLIENTS);

    /*
     * Fixed seeds make the stress test reproducible.
     */
    for (int i = 0; i < NUM_CLIENTS; ++i) {

        clients.emplace_back(
            stress_client,
            i + 1,
            12345ULL + i
        );
    }

    for (auto& thread : clients) {
        thread.join();
    }

    const auto end =
        std::chrono::steady_clock::now();

    const double elapsed =
        std::chrono::duration<double>(
            end - start
        ).count();

    const uint64_t sent =
        requests_sent.load(
            std::memory_order_relaxed
        );

    const uint64_t new_count =
        new_sent.load(
            std::memory_order_relaxed
        );

    const uint64_t cancel_count =
        cancel_sent.load(
            std::memory_order_relaxed
        );

    const uint64_t modify_count =
        modify_sent.load(
            std::memory_order_relaxed
        );

    const uint64_t pnl_count =
        pnl_sent.load(
            std::memory_order_relaxed
        );

    std::cout
        << "\n========================================\n"
        << "             TEST COMPLETE\n"
        << "========================================\n";

    std::cout
        << "Requests sent:        "
        << sent
        << '\n';

    std::cout
        << "Expected:             "
        << TOTAL_REQUESTS
        << '\n';

    std::cout
        << "\nWorkload breakdown:\n";

    std::cout
        << "NEW:                  "
        << new_count
        << '\n';

    std::cout
        << "CANCEL:               "
        << cancel_count
        << '\n';

    std::cout
        << "MODIFY:               "
        << modify_count
        << '\n';

    std::cout
        << "PNL:                  "
        << pnl_count
        << '\n';

    std::cout
        << "\nGeneration time:      "
        << elapsed
        << " sec\n";

    std::cout
        << "Generation throughput:"
        << static_cast<double>(sent) / elapsed
        << " requests/sec\n";

    std::cout
        << "========================================\n";

    return 0;
}