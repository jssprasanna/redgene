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

    return EXIT_SUCCESS;
}
