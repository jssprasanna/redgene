#include "redgene.hpp"

using namespace redgene;

int main(int argc, char** argv)
{
    redgene_validator rg_validator("test.json");
    //redgene_engine rg_engine(rg_validator);
    //rg_engine.generate();
    if(rg_validator.is_valid())
        cout << "VALID!" << endl;
    else
        cout << "INVALID!!!" << endl;
    
    return 0;
}