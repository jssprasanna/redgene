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

    cout << (rg_validator.is_valid() ? "VALID" : "INVALID!") << endl;
    
    if(rg_validator.is_valid())
    {
        redgene_engine rg_engine(rg_validator);
        rg_engine.generate(); 
        cout << "Data Generated Successfully." << endl;
    } 
    
    return EXIT_SUCCESS;
}
