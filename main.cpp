#include "redgene.hpp"

int main(int argc, char** argv)
{
    ofstream flatfile("ff1.txt", std::ofstream::trunc);
    prng_engine<uint_fast64_t> peng(MT19937_64, 101);
    //prob_dist_base<uint_fast64_t>* uid = new uniform_int_dist_engine<uint_fast64_t, uint_fast64_t>(peng, 0, 10);
    /*prob_dist_base<float>* urd = new uniform_real_dist_engine<uint_fast64_t, float>(peng, 3, 8);
    for(uint_fast32_t i = 0; i < 100000; ++i)
        flatfile << (*uid)() << '|' << (*urd)() << '\n';
    if(flatfile.is_open())
    {
        flatfile.flush();
        flatfile.close();
    }
    delete urd;
    delete uid;

    for(uint_fast16_t i = 0 ; i < 200; ++i)
    {
        uint_fast64_t key = (*uid)();
        rand_str_generator<> str_gen(15, false);
        cout << key << ":" << str_gen(key) << endl;
    }

    delete uid;*/

    prob_dist_base<uint_fast64_t>* zipf = new zipf_distribution<>(peng, 300, 1.5);
    array<uint_fast64_t, 1000> hist;
    hist.fill(0);
    for (int i = 0; i < 10000; i++)
    {
        ++hist[(*zipf)() - 1];
    }

    for(int i  = 0; i < 300; i++)
		cout << i+1 << " : " << hist[i] << endl;

    return 0;
}