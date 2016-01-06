#include "Communicator.h"
#include <iostream>

using namespace std;

namespace Sushi
{
    Communicator::Communicator(std::string ip, int port)
    {
        sf::Socket::Status status = socket.connect(ip, port);
        if(status != sf::Socket::Done) throw status;

        n = 0;
        in_i = in_n = 0;
    }

    Communicator::~Communicator()
    {
        // ...
    }

    void Communicator::startGame(std::string name)
    {
        writeByte(name.length());
        for(char c : name)
            writeByte(c);
        flush();
    }

    std::string Communicator::readBoard(int &n, int &m)
    {
        n = readByte(); m = readByte();
        std::string res = "";
        for(int i = 0; i < n; i++)
            for(int j = 0; j < m; j++)
            {
                char c = readByte();
                if(c == -3) res += '#';
                else if(c == 0) res += '.';
                else if(c > 0) res += 'a' + (c - 1);
                else if(c == -2) res += '1';
                else res += '0';
            }
        return res;
    }

    std::vector<Move> Communicator::readMoves()
    {
        std::vector<Move> res;
        int n = readInt();
        for(int i = 0; i < n; i++)
        {
            Move x;
            x.i = readByte();
            x.j = readByte();
            x.n = readInt();
            x.dir = readByte();
            res.push_back(x);
        }
        return res;
    }

    void Communicator::sendMoves(std::vector<Move> a)
    {
        writeInt(a.size());
        for(Move i : a)
        {
            cout<<i.i<<" "<<i.j<<" "<<i.n<<" "<<i.dir<<endl;
            writeByte(i.i);
            writeByte(i.j);
            writeInt(i.n);
            writeByte(i.dir);
        }
        cout<<"--"<<endl;
        flush();
    }

    void Communicator::writeByte(char c)
    {
        buffer[n++] = c;
        if(n == BUFLEN) throw "TCP buffer overflow";
    }

    void Communicator::writeInt(int x)
    {
        writeByte(x >> 8);
        writeByte(x & 0xFF);
    }

    void Communicator::flush()
    {
        if(socket.send(buffer, n) != sf::Socket::Done) throw "TCP sending error";
        n = 0;
    }

    char Communicator::readByte()
    {
        if(in_i >= in_n)
        {
            in_i = 0;
            if(socket.receive(in_buffer, BUFLEN, (size_t&)in_n) != sf::Socket::Done) throw "TCP read error";
        }
        return in_buffer[in_i++];
    }

    int Communicator::readInt()
    {
        int res = readByte();
        res <<= 8;
        res |= readByte();
        return res;
    }
}
