#include "redgene.hpp"

int main(int argc, char** argv)
{
    redgene_validator rgene("test.json");
    if(rgene.is_valid())
        cout << "JSON IS ReDGene VALID :-)" << endl;
    else
        cout << "JSON IS NOT ReDGene VALID!" << endl;

    ofstream flatfile("ff1.txt", std::ofstream::trunc);
    prng_engine<uint_fast64_t> peng(MT19937_64, 1729);
    table_type tab("tab1", 100);
    column_type<uint_fast64_t>* col1_gen = new normal_int_column<>(peng, tab, "col1", 5);
    column_type<double>* col2_gen = new normal_real_column<>(peng, tab, "col2", 2, 3);
    column_type<string>* col3_gen = new normal_string_column<>(peng, tab, "col3", 0.3, 15, true);

    for(uint_fast64_t i = 0; i < tab.get_row_count(); i++)
    {
        flatfile << (*col1_gen)() << '|' << (*col2_gen)() 
            << '|' << (*col3_gen)() << endl;
    }

    delete col3_gen;
    delete col2_gen;
    delete col1_gen;
    
    if(flatfile.is_open())
    {
        flatfile.flush();
        flatfile.close();
    }
    return 0;
}