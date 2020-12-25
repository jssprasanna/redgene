#include "redgene.hpp"

using namespace redgene;

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        cout << "Insufficient arguments; redgene <schema_template.json>" << endl;
        return EXIT_SUCCESS;
    }
    redgene_validator rg_validator(argv[1]);
    
    redgene_engine rg_engine(rg_validator);
    rg_engine.generate(); 

    cout << "Data Generated Successfully." << endl;

/*     prng_engine<uint_fast64_t> prng(random_engines::MT19937_64);
    set_distribution<> set_dist(prng, 10, 100);
    for(uint_fast64_t i = 0; i < 10; i++)
    {
        cout << set_dist() << endl;
    } */
    return EXIT_SUCCESS;
}
