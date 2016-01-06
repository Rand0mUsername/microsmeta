#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <sfml/Network.hpp>
#include <string>
#include <cstdlib>
#include <vector>

namespace Sushi
{
    struct Move
    {
        int i, j, n, dir;
    } ;

    class Communicator
    {
        public:
            // Communicator(string ip, int port), default values are localhost and 58351
            Communicator(std::string ip = "localhost", int port = PORT);
            ~Communicator();

            // startGame(name)
            void startGame(std::string);

            // readBoard(n, m) -> rows concatenated
            // block: '#'
            // base: '0' / '1'
            // empty: '.'
            // units: 'a' = 1, 'b' = 2, 'c' = 3, ...

            std::string readBoard(int&, int&);

            std::vector<Move> readMoves();
            void sendMoves(std::vector<Move>);

        private:
            static const int PORT = 58351;
            static const int BUFLEN = 1 << 15;

            char buffer[BUFLEN];
            int n;
            sf::TcpSocket socket;

            void writeByte(char);
            void writeInt(int);
            void flush();

            char in_buffer[BUFLEN];
            int in_i, in_n;
            char readByte();
            int readInt();
    };

}

#endif // COMMUNICATOR_H
