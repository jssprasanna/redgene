#include "redgene.hpp"

using namespace redgene;

int main(int argc, char** argv)
{
    redgene_validator rg_validator("test.json");
    redgene_engine rg_engine(rg_validator);
    rg_engine.generate();

    return 0;
}