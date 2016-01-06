// Team MicroSmeta~
// Dimitrije Erdeljan & Nikola Jovanovic
// AngryMobs Hackathon
// Petnica Science Center IOI preparation camp - June 2015

#include <iostream>
#include <cstdio>
#include <string>
#include <queue>
#define par pair<int, int>
#include "communicator.h"

using namespace std;
using namespace Sushi;

// vars

bool am_booming, goBoom;
bool beginning;

const int N = 64;
const int CAP_CAP = 9;
bool block[N][N];
int neutral[N][N], hostile[N][N];
int curr_tick;

const int DI[] = { 0, -1, 0, 1 }, DJ[] = { 1, 0, -1, 0 };

bool ass[N][N];

struct unit
{
    int i, j;
    int mode;
    int ti, tj;
    bool assigned;
};

vector<unit> units;
bool gameInProgress;
int my_base_i, my_base_j, opp_base_i, opp_base_j;

bool neutrals_depleted;

int dist[N][N][N][N];
bool mark[N][N];

const int NONE = 0, HARVEST = 1, ATTACK = 2, DEFEND = 3, SUICIDE = 4, FORCE = 5, BOOM = 6, SHOWOFF = 7, DEBUG_1 = 100, DEBUG_2 = 101;

const int BOOM_R = 5;
bool boomd[N][N];
int from[N][N];

int showingOff; //0-no 1-assembling 2-showing off
bool moved;
int ticksLeft;

// atomic functions

void add_unit(int i, int j)
{
    units.push_back({i, j, 0});
}

int sgnx(int x) { return x < 0 ? -1 : 1; }

int absx(int x) { return x < 0 ? -x : x; }

bool fuckable(int i, int j)
{
    if(!block[i][j] && (i != opp_base_i || j != opp_base_j))
        return true;
    return false;
}

int get_hostile(int i, int j) { return (i < 0 || j < 0 || i >= N || j >= N) ? 0 : hostile[i][j]; }

// total number of opponent's units
int total_opp_units()
{
    int res = 0;
    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++)
            res += hostile[i][j];
    return res;
}

// each soldier has it's own role: harvest, attack, defend, suicide, force, boom, showoff
void change_role(unit &u, int to); // jbg

// total number of neutral units
int total_neutral_units()
{
    int res = 0;
    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++)
            res += neutral[i][j];
    return res;
}

// preprocessing

void bfsa(int si, int sj)
{
    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++)
            { mark[i][j] = false; dist[si][sj][i][j] = 1 << 30; }
    dist[si][sj][si][sj] = 0;
    if(block[si][sj]) return;

    queue<int> qi, qj;
    qi.push(si); qj.push(sj);

    while(!qi.empty())
    {
        int i = qi.front(); qi.pop();
        int j = qj.front(); qj.pop();

        if(mark[i][j]) continue;
        mark[i][j] = true;

        for(int d = 0; d < 4; d++)
        {
            int ii = i + DI[d], jj = j + DJ[d];
            if(ii < 0 || jj < 0 || ii >= N || jj >= N) continue;
            if(dist[si][sj][ii][jj] <= dist[si][sj][i][j] + 1) continue;
            if(block[ii][jj]) continue;
            dist[si][sj][ii][jj] = dist[si][sj][i][j] + 1;
            qi.push(ii); qj.push(jj);
        }
    }
}

void preprocess()
{
    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++)
            bfsa(i, j);
}

// main logic

bool has_hostile(int i, int j, int r)
{
    for(int ii = max(i - r, 0); ii < min(N, i + r + 1); ii++)
        for(int jj = max(j - r, 0); jj < min(N, j + r + 1); jj++)
            if(hostile[ii][jj]) return true;
    return false;
}

bool compare_loc(int i, int j, int ii, int jj, int di, int dj)
{
    if(dist[i][j][di][dj] != dist[ii][jj][di][dj]) return dist[i][j][di][dj] < dist[ii][jj][di][dj];
    for(int r = 1; r <= 3; r++)
    {
        bool a = has_hostile(i, j, r);
        bool aa = has_hostile(ii, jj, r);
        if(a && aa) return beginning || (rand()&1); // rand?
        if(a) return false;
        if(aa) return true;
    }
    return beginning || (rand()&1); // rand?
}

Move move_to(int i, int j, int di, int dj)
{
    if(i == di && j == dj)
        return {i, j, -1, -1};

    int res = -1;
    for(int d = 0; d < 4; d++)
    {
        int ii = i + DI[d], jj = j + DJ[d];
        if(ii >= 0 && ii < N && jj >= 0 && jj < N && !block[ii][jj])
        {
            if(res == -1 || compare_loc(ii, jj, i + DI[res], j + DJ[res], di, dj))
                res = d;
        }
    }
    return {i, j, 1, res};
}

Move move_to_forced(int i, int j, int di, int dj)
{
    if(i == di && j == dj)
        return {i, j, -1, -1};

    int res = -1;
    for(int d = 0; d < 4; d++)
    {
        int ii = i + DI[d], jj = j + DJ[d];
        if(ii >= 0 && ii < N && jj >= 0 && jj < N && !block[ii][jj])
        {
            if(dist[i][j][di][dj] == dist[ii][jj][di][dj] + 1)
                return {i, j, 1, d};
        }
    }
}

void boombfs(int si, int sj)
{
    int n0 = max(si - BOOM_R, 0), n1 = min(si + BOOM_R + 1, N);
    int m0 = max(sj - BOOM_R, 0), m1 = min(sj + BOOM_R + 1, N);

    for(int i = n0; i < n1; i++)
        for(int j = m0; j < m1; j++)
            { boomd[i][j] = 1 << 30; mark[i][j] = false; }

    queue<int> qi, qj;
    qi.push(si); qj.push(sj);
    boomd[si][sj] = 0;

    while(!qi.empty())
    {
        int i = qi.front(); qi.pop();
        int j = qj.front(); qj.pop();

        for(int d = 0; d < 4; d++)
        {
            int ii = i + DI[d], jj = j + DJ[d];
            if(ii < n0 || ii >= n1) continue;
            if(jj < m0 || jj >= m1) continue;
            if(block[ii][jj]) continue;
            if(neutral[ii][jj]) continue;
            if(boomd[ii][jj] <= boomd[i][j] + 1) continue;
            boomd[ii][jj] = boomd[i][j] + 1;
            qi.push(ii); qj.push(jj);
            from[ii][jj] = (i == si && j == sj) ? d : from[i][j];
        }
    }
}

Move move_to_boom(int si, int sj, int di, int dj)
{
    int n0 = max(si - BOOM_R, 0), n1 = min(si + BOOM_R + 1, N);
    int m0 = max(sj - BOOM_R, 0), m1 = min(sj + BOOM_R + 1, N);
    boombfs(si, sj);
    return {si, sj, 1, from[di][dj]};
    for(int d = 0; d < 4; d++)
    {
        int ii = si + DI[d], jj = sj + DJ[d];
        if(ii >= n0 && ii < n1 && jj >= m0 && jj < m1 && !block[ii][jj])
        {
            if(boomd[di][dj] == boomd[di][dj] + 1)
                return {si, sj, 1, d};
        }
    }
}

void get_bombs(int si, int sj)
{
    int n0 = max(si - BOOM_R, 0), n1 = min(si + BOOM_R + 1, N);
    int m0 = max(sj - BOOM_R, 0), m1 = min(sj + BOOM_R + 1, N);
    boombfs(si, sj);

    vector<pair<int,int>> candidates;
    for(int i = n0; i < n1; i++)
        for(int j = m0; j < m1; j++)
            if(boomd[i][j] < 1000 && neutral[i][j]) candidates.push_back({i, j});

    sort(candidates.begin(), candidates.end(), [](pair<int,int> a, pair<int,int> b) {
         return neutral[a.first][a.second] > neutral[b.first][b.second];
         });

    int i = 0;
    for(unit &u : units)
    {
        change_role(u, BOOM);
        u.ti = candidates[i].first;
        u.tj = candidates[i].second;
        if(i < candidates.size() - 1) i++;
    }
    am_booming = true;
}

// where to move depending on your role

Move move_defend(unit &u)
{
    if(u.i == my_base_i && get_hostile(u.i, u.j + sgnx(u.j - my_base_j))) return {-1,-1, -1, 1};
    if(u.j == my_base_j && get_hostile(u.i + sgnx(u.i - my_base_i), u.j)) return {-1,-1, -1, 1};
    return move_to(u.i, u.j, my_base_i, my_base_j);
}

Move move_attack(unit &u)
{
    return move_to(u.i, u.j, opp_base_i, opp_base_j);
}

Move move_suicide(unit &u)
{
    return move_to_forced(u.i, u.j, opp_base_i, opp_base_j);
}

Move move_harvest(unit &u)
{
    if(u.assigned && neutral[u.ti][u.tj] == 0)
    {
        //unassign
        u.assigned = false;
        ass[u.ti][u.tj] = false;
    }

    if(!u.assigned)
    {
        //assign
        int closestNeutral = N*N, ni, nj;
        for(int i = 0; i < N; i++)
            for(int j = 0; j < N; j++)
            {
                if(neutral[i][j] > 0 && (beginning || !ass[i][j]) && dist[u.i][u.j][i][j] < closestNeutral)
                    {
                        closestNeutral = dist[u.i][u.j][i][j];
                        ni = i, nj = j;
                    }
            }
        ass[ni][nj] = true;
        u.ti = ni, u.tj = nj;
        u.assigned = true;
    }

    Move x = move_to(u.i, u.j, u.ti, u.tj);
    if(beginning && ((int)(units.size()) + neutral[u.i + DI[x.dir]][u.j + DJ[x.dir]] > 8))
    {
        beginning = false;
        goBoom = true;
        return {-1,-1,-1,-1};
    }
    return move_to(u.i, u.j, u.ti, u.tj);
}
Move move_forced(unit &u)
{
    return move_to_forced(u.i, u.j, opp_base_i, opp_base_j);
}

Move move_boom(unit &u)
{
    return move_to_boom(u.i, u.j, u.ti, u.tj);
}

Move move_showoff(unit &u)
{
    if(showingOff == 1)
    {
        if(u.i == u.ti && u.j == u.tj)
             return {-1,-1,-1,-1};
        else
        { moved = true; return move_to(u.i, u.j, u.ti, u.tj);}
    }
    else if(showingOff == 2)
    {
        if(ticksLeft%2 == 1)
            return move_to(u.i, u.j, u.i, u.j-1);
        else
            return move_to(u.i, u.j, u.i, u.j+1);

    }


}

// choosing a move

Move make_move(unit &u)
{
    if(u.mode == HARVEST) return move_harvest(u);
    else if(u.mode == ATTACK) return move_attack(u);
    else if(u.mode == DEFEND) return move_defend(u);
    else if(u.mode == SUICIDE) return move_suicide(u);
    else if(u.mode == FORCE) return move_forced(u);
    else if(u.mode == BOOM) return move_boom(u);
    else if(u.mode == SHOWOFF) return move_showoff(u);
    else return move_to(u.i, u.j, my_base_i, my_base_j);
}

// changing the role

void change_role(unit &u, int to)
{
    if(to == SHOWOFF)
    {
        u.mode = to;
        u.ti = u.i;
        u.tj = u.j;
        return;
    }
    if(to != HARVEST) beginning = false;
    if(u.mode == FORCE) return; // FUCK YOU I WONT DO WHAT YOU TELL ME
    if(to == HARVEST && neutrals_depleted) { u.mode = ATTACK; return; } // empty table
    if(u.mode == HARVEST && to != HARVEST)
    {
        u.assigned = false;
        ass[u.ti][u.tj] = false;
    }
    u.mode = to;
}

// make your next move

void makeMove(Communicator &comm)
{
    vector<Move> subs;

    int n_my = units.size(), n_his = total_opp_units();

    if(!neutrals_depleted && total_neutral_units() == 0) neutrals_depleted = true;

    if(!showingOff && n_his == 0 && n_my >= 5)
    {
        //SHOW OFF
        showingOff = 1;
        for(auto &u : units)
        {
            change_role(u, SHOWOFF);
        }
        int i = opp_base_i;
        int j = opp_base_j;
        // left
        if(fuckable(i, j-1) && fuckable(i, j-2) && fuckable(i, j-3) && fuckable(i-1, j-4) && fuckable(i+1, j-4))
        {
            units[0].ti = i, units[0].tj = j-1;
            units[1].ti = i, units[1].tj = j-2;
            units[2].ti = i, units[2].tj = j-3;
            units[3].ti = i-1, units[3].tj = j-4;
            units[4].ti = i+1, units[4].tj = j-4;
        }
        else{
        // force
        for(auto &u : units)
        {
            change_role(u, FORCE);
        }
       }
    }
    else if(!showingOff)
    {
        if(n_my < CAP_CAP)
        {
            for(auto &u : units)
                change_role(u, HARVEST);
        }
        else if(n_my > n_his)
        {
            for(auto &u : units)
                    change_role(u, ATTACK);
        }
        else
        {
            int n_h = 0;
            for(auto &u : units)
                if(u.mode == HARVEST) n_h++;
                else change_role(u, SUICIDE);

            for(auto &u : units)
                if(n_h < CAP_CAP - 1 && u.mode != HARVEST) { change_role(u, HARVEST); n_h++; }
                else if(n_h >= CAP_CAP && u.mode == HARVEST) { change_role(u, SUICIDE); n_h--; }
        }

            // DEFEND MATCH
            vector<par> oppCoords;
            for(int i = 0; i < N; i++)
                for(int j = 0; j < N; j++)
                    for(int k=1; k <= hostile[i][j]; k++)
                        oppCoords.push_back(par(i, j));

           // sort by dist[si][sj] oppcorrds
           sort(units.begin(), units.end(), [](unit a, unit b) { return dist[a.i][a.j][my_base_i][my_base_j] < dist[b.i][b.j][my_base_i][my_base_j]; });
           sort(oppCoords.begin(), oppCoords.end(), [](par a, par b) { return dist[a.first][a.second][my_base_i][my_base_j] < dist[b.first][b.second][my_base_i][my_base_j]; });

                  int my_sz = (int)units.size();
           int his_sz = (int)oppCoords.size();
           int sz = min(my_sz, his_sz);
           for(int i=0; i<sz; i++)
           {
               if(dist[oppCoords[i].first][oppCoords[i].second][my_base_i][my_base_j]
                  <= dist[units[i].i][units[i].j][my_base_i][my_base_j] + 1)
                    change_role(units[i], DEFEND);
           }

        // rush
            int hisBest = N*N;

            for(int it=0; it<his_sz; it++)
            {
               if(dist[oppCoords[it].first][oppCoords[it].second][opp_base_i][opp_base_j] < hisBest)
               {
                   hisBest = dist[oppCoords[it].first][oppCoords[it].second][opp_base_i][opp_base_j];
               }
            }

            for(int it=0; it<my_sz; it++)
            {
               if(units[it].mode != DEFEND && dist[units[it].i][units[it].j][opp_base_i][opp_base_j] <= 1 + hisBest)
               {
                  change_role(units[it], FORCE);
               }
            }

            // end rush

        if(curr_tick > 1000 - dist[my_base_i][my_base_j][opp_base_i][opp_base_j] - 42)
            for(auto &u : units)
                change_role(u, FORCE);

        if(false && goBoom) get_bombs(units[0].i, units[0].j);
        if(am_booming)
        {
            bool ok = true;
            for(auto &u : units)
                if(absx(u.i - u.ti) + absx(u.j - u.tj) > 1) ok = false;

            if(ok)
            {
                am_booming = false;
                for(auto &u : units)
                    if(u.mode == BOOM) { change_role(u, HARVEST); u.assigned = true;}
            }
        }
    }


    moved = false;

    // make moves
    for(auto &u : units)
    {
        Move x = make_move(u);
        if(x.n == -1) continue;
        subs.push_back(x);

        u.i += DI[x.dir]; u.j += DJ[x.dir];
    }

    if(showingOff==1 && moved == false) {showingOff = 2; ticksLeft = 50;}
    if(showingOff==2)
        ticksLeft--;
    if(showingOff==2 && ticksLeft==0)
    {
        for(auto &u : units)
                change_role(u, FORCE);
    }

    comm.sendMoves(subs);
}

// on death
void on_death(unit &u)
{
    beginning = false;
    if(u.mode == HARVEST)
    {
        u.assigned = false;
        ass[u.ti][u.tj] = false;
    }
    return;
}

// read and apply your opponent's move
void takeOpponentsMove(Communicator &comm)
{
    vector<Move> moves = comm.readMoves();

    int n_his = total_opp_units();
    bool can_cap = units.size() < CAP_CAP;

    for(auto &m : moves)
    {
        hostile[m.i][m.j] -= m.n;
        m.i += DI[m.dir]; m.j += DJ[m.dir];
        hostile[m.i][m.j] += m.n;
    }

        int kill = 0;
        for(int i = 0; i < (int)(units.size()); i++)
            if(hostile[units[i].i][units[i].j])
            {
                hostile[units[i].i][units[i].j]--;
                kill++;
                on_death(units[i]);
            }
            else units[i - kill] = units[i];
    units.resize(units.size() - kill);

    if(n_his < CAP_CAP)
    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++)
            if(hostile[i][j] && neutral[i][j]) { hostile[i][j] += neutral[i][j]; neutral[i][j] = 0; }

        if(can_cap)
    {
        int nn = units.size();
        for(int i = 0; i < nn; i++){
            while(neutral[units[i].i][units[i].j] > 0)
            {
                neutral[units[i].i][units[i].j]--;
                add_unit(units[i].i, units[i].j);
            }}}
}


int main() {  try{
    beginning = true;
        srand(time(NULL));

    Communicator comm;
    comm.startGame("microSmeta~");

    //freopen("out.txt", "w", stdout);
    //freopen("err.txt", "w", stderr);

    int n, m, k = 0;
        string s = comm.readBoard(n, m);
        for(int i = 0; i < n; i++)
        for(int j = 0; j < m; j++)
        {
            block[i][j] = s[k] == '#';
            neutral[i][j] = (s[k] >= 'a' && s[k] <= 'z') ? (s[k] - 'a' + 1) : 0;
            if(s[k] == '0') { my_base_i = i; my_base_j = j; add_unit(i, j); }
            if(s[k] == '1') { opp_base_i = i; opp_base_j = j; hostile[i][j] = 1; }
            k++;
        }

    preprocess();

    gameInProgress = true;
        while (gameInProgress) {
                curr_tick++;
                makeMove(comm);
                takeOpponentsMove(comm);
        }
}

catch(const char *err)
{
    cerr << "ERROR: \'" << err << "\'\n";
}
catch(sf::Socket::Status status)
{
    if(status == sf::Socket::Disconnected) cerr << "DISCONNECTED\n";
    if(status == sf::Socket::NotReady) cerr << "NOTERADY\n";
    if(status == sf::Socket::Error) cerr << "ERR\n";
}
return 0;
}
