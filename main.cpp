#include "prob_dist.hpp"

using namespace redgene;

int main(int argc, char** argv)
{
    ofstream flatfile("ff1.txt", std::ofstream::trunc);
    prng_engine<uint_fast64_t> peng(MT19937_64, 100);
    prob_dist_base<uint_fast64_t>* uid = new uniform_int_dist_engine<uint_fast64_t, uint_fast64_t>(peng, 0, 10);
    prob_dist_base<float>* urd = new uniform_real_dist_engine<uint_fast64_t, float>(peng, 3, 8);
    for(uint_fast32_t i = 0; i < 100000; i++)
        flatfile << (*uid)() << '|' << (*urd)() << '\n';
    if(flatfile.is_open())
    {
        flatfile.flush();
        flatfile.close();
    }
    delete urd;
    delete uid;
    return 0;
}